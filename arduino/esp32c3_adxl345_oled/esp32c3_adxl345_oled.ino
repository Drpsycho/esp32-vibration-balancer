#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <U8g2lib.h>

// ESP32-C3 + small OLED from the message:
// I2C display: SDA=5, SCL=6, address 0x3C, visible offset X=28, Y=0.
static constexpr uint8_t I2C_SDA_PIN = 5;
static constexpr uint8_t I2C_SCL_PIN = 6;
static constexpr uint8_t OLED_ADDR_7BIT = 0x3C;
static constexpr uint8_t ADXL345_ADDR_7BIT = 0x53; // Often 0x53. SDO high makes it 0x1D.

// Use the built-in LED if the selected ESP32-C3 board defines it.
// Otherwise set this to the GPIO where the external LED is connected.
#ifndef LED_BUILTIN
static constexpr uint8_t LED_PIN = 8;
#else
static constexpr uint8_t LED_PIN = LED_BUILTIN;
#endif

// Display selection. Keep DISPLAY_72X40 enabled for the ESP32-C3 OLED module.
// It uses a 128x64 SSD1306 controller, but the visible 72x40 window starts at X=28.
// Set DISPLAY_72X40 to 0 for a classic 0.91" 128x32 I2C OLED.
#define DISPLAY_72X40 1

#if DISPLAY_72X40
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
static constexpr int16_t DRAW_X_OFFSET = 28;
static constexpr int16_t DRAW_Y_OFFSET = 0;
static constexpr int16_t VISIBLE_WIDTH = 72;
static constexpr int16_t VISIBLE_HEIGHT = 40;
#else
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
static constexpr int16_t DRAW_X_OFFSET = 0;
static constexpr int16_t DRAW_Y_OFFSET = 0;
static constexpr int16_t VISIBLE_WIDTH = 128;
static constexpr int16_t VISIBLE_HEIGHT = 32;
#endif

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Values are in m/s^2. Raise PEAK_MIN_RISE if the LED reacts to noise.
static constexpr float PEAK_MIN_RISE = 0.25f;
static constexpr float PEAK_DROP = 0.15f;
static constexpr uint32_t SAMPLE_PERIOD_MS = 50;
static constexpr uint32_t LED_PULSE_MS = 80;
static constexpr uint32_t SCREEN_PERIOD_MS = 100;

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
  digitalWrite(LED_PIN, HIGH);
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
  if (!haveSample) {
    haveSample = true;
    xValley = value;
    xPeakCandidate = value;
    xMax = value;
    return;
  }

  if (!risingToPeak) {
    if (value < xValley) {
      xValley = value;
    }

    if (value >= xValley + PEAK_MIN_RISE) {
      risingToPeak = true;
      xPeakCandidate = value;
    }
    return;
  }

  if (value > xPeakCandidate) {
    xPeakCandidate = value;
  }

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
  digitalWrite(LED_PIN, LOW);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
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
    digitalWrite(LED_PIN, LOW);
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
