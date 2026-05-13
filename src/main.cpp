#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <U8g2lib.h>

// Пины I2C по умолчанию:
// ESP32-C3: SDA=5, SCL=6.
// STM32F103C8T6: SDA=PB7, SCL=PB6.
#ifndef I2C_SDA_PIN
#if defined(ARDUINO_ARCH_STM32)
#define I2C_SDA_PIN PB7
#else
#define I2C_SDA_PIN 5
#endif
#endif

#ifndef I2C_SCL_PIN
#if defined(ARDUINO_ARCH_STM32)
#define I2C_SCL_PIN PB6
#else
#define I2C_SCL_PIN 6
#endif
#endif

#ifndef OLED_ADDR_7BIT
#define OLED_ADDR_7BIT 0x3C
#endif

#ifndef ADXL345_ADDR_7BIT
#define ADXL345_ADDR_7BIT 0x53
#endif

// Берем встроенный светодиод, если он описан в выбранной плате.
// Иначе используем внешний светодиод на GPIO 8.
#ifndef LED_BUILTIN
static constexpr uint8_t LED_PIN = 8;
#else
static constexpr uint8_t LED_PIN = LED_BUILTIN;
#endif

// На STM32F103C8T6 встроенный светодиод на PC13 обычно включается уровнем LOW.
#ifndef LED_ACTIVE_LOW
#if defined(ARDUINO_ARCH_STM32)
#define LED_ACTIVE_LOW 1
#else
#define LED_ACTIVE_LOW 0
#endif
#endif

// Выбор OLED. DISPLAY_72X40=1 нужен для маленького ESP32-C3 OLED 72x40.
// Для обычного SSD1306 0.91" 128x32 ставим DISPLAY_72X40=0.
#ifndef DISPLAY_72X40
#define DISPLAY_72X40 1
#endif

#if DISPLAY_72X40
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
static constexpr int16_t DRAW_X_OFFSET = 28;
static constexpr int16_t DRAW_Y_OFFSET = 0;
#else
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
static constexpr int16_t DRAW_X_OFFSET = 0;
static constexpr int16_t DRAW_Y_OFFSET = 0;
#endif

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Значения в m/s^2. Если светодиод реагирует на шум, увеличьте PEAK_MIN_RISE.
static constexpr float PEAK_MIN_RISE = 0.25f;
static constexpr float PEAK_DROP = 0.15f;
static constexpr uint32_t SAMPLE_PERIOD_MS = 50;
static constexpr uint32_t LED_PULSE_MS = 80;
static constexpr uint32_t SCREEN_PERIOD_MS = 100;

// Состояние простого детектора пиков по оси X.
float xNow = 0.0f;
float xMax = 0.0f;
float xValley = 0.0f;
float xPeakCandidate = 0.0f;
bool haveSample = false;
bool risingToPeak = false;
uint32_t ledOffAtMs = 0;
uint32_t lastSampleMs = 0;
uint32_t lastScreenMs = 0;
bool adxlOk = false;

static void startPulse() {
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? LOW : HIGH);
  ledOffAtMs = millis() + LED_PULSE_MS;
}

static void drawStatus(const char *status) {
  display.clearBuffer();
  display.setFont(u8g2_font_5x7_tf);
  display.setCursor(DRAW_X_OFFSET, DRAW_Y_OFFSET + 7);
  display.print(status);

  display.setFont(u8g2_font_6x12_tf);
  display.setCursor(DRAW_X_OFFSET, DRAW_Y_OFFSET + 20);
  display.print("X ");
  display.print(xNow, 2);

#if DISPLAY_72X40
  display.setCursor(DRAW_X_OFFSET, DRAW_Y_OFFSET + 34);
#else
  display.setCursor(DRAW_X_OFFSET + 64, DRAW_Y_OFFSET + 20);
#endif
  display.print("Max ");
  display.print(xMax, 2);
  display.sendBuffer();
}

static void scanI2cToSerial() {
  Serial.println("I2C scan:");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  found 0x");
      if (addr < 16) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);
    }
  }
}

static void updatePeakDetector(float value) {
  // Первый замер только инициализирует базовые уровни, пика еще нет.
  if (!haveSample) {
    haveSample = true;
    xValley = value;
    xPeakCandidate = value;
    xMax = value;
    return;
  }

  if (!risingToPeak) {
    // Пока не растем к пику, запоминаем самую низкую точку.
    if (value < xValley) {
      xValley = value;
    }

    // Рост от впадины больше порога считаем началом нового пика.
    if (value >= xValley + PEAK_MIN_RISE) {
      risingToPeak = true;
      xPeakCandidate = value;
    }
    return;
  }

  // На подъеме обновляем кандидата в пик.
  if (value > xPeakCandidate) {
    xPeakCandidate = value;
  }

  // Когда значение достаточно упало от кандидата, фиксируем пик.
  if (value <= xPeakCandidate - PEAK_DROP) {
    xMax = xPeakCandidate;
    risingToPeak = false;
    xValley = value;
    xPeakCandidate = value;

    startPulse();
    Serial.print("X peak max: ");
    Serial.println(xMax, 3);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);

#if defined(ARDUINO_ARCH_STM32)
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
  Wire.begin();
#else
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#endif
  Wire.setClock(400000);

  display.setI2CAddress(OLED_ADDR_7BIT << 1);
  display.begin();
  drawStatus("Starting");

  adxlOk = accel.begin(ADXL345_ADDR_7BIT);
  if (adxlOk) {
    accel.setRange(ADXL345_RANGE_16_G);
    accel.setDataRate(ADXL345_DATARATE_100_HZ);
    drawStatus("ADXL OK");
  } else {
    drawStatus("No ADXL");
    Serial.println("ADXL345 not found. Check VCC/GND/SDA/SCL and address 0x53 or 0x1D.");
    scanI2cToSerial();
  }
}

void loop() {
  const uint32_t now = millis();

  if (ledOffAtMs != 0 && static_cast<int32_t>(now - ledOffAtMs) >= 0) {
    digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
    ledOffAtMs = 0;
  }

  if (adxlOk && now - lastSampleMs >= SAMPLE_PERIOD_MS) {
    lastSampleMs = now;

    sensors_event_t event;
    accel.getEvent(&event);
    xNow = event.acceleration.x;
    updatePeakDetector(xNow);
  }

  if (now - lastScreenMs >= SCREEN_PERIOD_MS) {
    lastScreenMs = now;
    drawStatus(adxlOk ? "ADXL345 X" : "No ADXL");
  }
}
