#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <U8g2lib.h>

// STM32F103C8T6 + LILUDIN 0.91" 128x32 blue/white OLED:
// I2C1: SDA=PB7, SCL=PB6, OLED address 0x3C.
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN PB7
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN PB6
#endif

#ifndef OLED_ADDR_7BIT
#define OLED_ADDR_7BIT 0x3C
#endif

#ifndef ADXL345_ADDR_7BIT
#define ADXL345_ADDR_7BIT 0x53
#endif

#ifndef LED_BUILTIN
static constexpr uint8_t LED_PIN = PC13;
#else
static constexpr uint8_t LED_PIN = LED_BUILTIN;
#endif

// Most STM32F103C8T6 boards have an active-low LED on PC13.
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 1
#endif

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
static constexpr int16_t DRAW_X_OFFSET = 0;
static constexpr int16_t DRAW_Y_OFFSET = 0;

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Values are in m/s^2. Raise TRIGGER_STEP if the LED reacts too often.
static constexpr float TRIGGER_STEP = 0.25f;
static constexpr float RESET_DROP = 0.15f;
static constexpr uint32_t SAMPLE_PERIOD_MS = 50;
static constexpr uint32_t LED_PULSE_MS = 80;
static constexpr uint32_t SCREEN_PERIOD_MS = 100;

float xNow = 0.0f;
float xMax = -1000.0f;
bool readyForNextPeak = true;
uint32_t ledOffAtMs = 0;
uint32_t lastSampleMs = 0;
uint32_t lastScreenMs = 0;
bool adxlOk = false;

static void writeLed(bool on) {
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? !on : on);
}

static void startPulse() {
  writeLed(true);
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

  display.setCursor(DRAW_X_OFFSET + 64, DRAW_Y_OFFSET + 20);
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

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED_PIN, OUTPUT);
  writeLed(false);

#if defined(ARDUINO_ARCH_STM32)
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
#endif
  Wire.begin();
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
    writeLed(false);
    ledOffAtMs = 0;
  }

  if (adxlOk && now - lastSampleMs >= SAMPLE_PERIOD_MS) {
    lastSampleMs = now;

    sensors_event_t event;
    accel.getEvent(&event);
    xNow = event.acceleration.x;

    if (readyForNextPeak && xNow > xMax + TRIGGER_STEP) {
      xMax = xNow;
      readyForNextPeak = false;
      startPulse();
      Serial.print("New X max: ");
      Serial.println(xMax, 3);
    }

    if (xNow < xMax - RESET_DROP) {
      readyForNextPeak = true;
    }
  }

  if (now - lastScreenMs >= SCREEN_PERIOD_MS) {
    lastScreenMs = now;
    drawStatus(adxlOk ? "ADXL345 X" : "No ADXL");
  }
}
