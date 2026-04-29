#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_INA219.h>

// --- OLED pins (unchanged from working bring-up) ---
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   16
#define OLED_CS   5
#define OLED_RST  17

// Panel is SSD1306. Flip to 1 if your panel is actually SH1106.
#define OLED_IS_SH1106 0
// Software SPI matches the proven bring-up; switch to 0 for hardware SPI.
#define OLED_USE_SW_SPI 1

#if OLED_IS_SH1106 && OLED_USE_SW_SPI
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST);
#elif OLED_IS_SH1106
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI display(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);
#elif OLED_USE_SW_SPI
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST);
#else
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI display(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);
#endif

// --- INA219 ---
#define INA_SDA 21
#define INA_SCL 22
Adafruit_INA219 ina219;

const unsigned long REFRESH_MS = 500;
unsigned long lastRefreshMs = 0;
bool inaReady = false;

void drawReadings(float busV, float shuntmV, float current_mA, float power_mW) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 10, "Power Monitor");
  display.drawHLine(0, 12, 128);

  char line[24];
  display.setFont(u8g2_font_logisoso16_tf);
  snprintf(line, sizeof(line), "%5.2f V", busV);
  display.drawStr(0, 32, line);
  snprintf(line, sizeof(line), "%6.1f mA", current_mA);
  display.drawStr(0, 50, line);

  display.setFont(u8g2_font_6x12_tf);
  snprintf(line, sizeof(line), "P %6.1f mW", power_mW);
  display.drawStr(0, 63, line);

  // Suppress unused-parameter warning while keeping shunt available for future use.
  (void)shuntmV;

  display.sendBuffer();
}

void drawError(const char *msg) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 12, "INA219 ERROR");
  display.drawStr(0, 30, msg);
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Booting INA219 power monitor...");
  Serial.printf("OLED driver: %s, bus: %s\n",
                OLED_IS_SH1106 ? "SH1106" : "SSD1306",
                OLED_USE_SW_SPI ? "Software SPI" : "Hardware SPI");

#if !OLED_USE_SW_SPI
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
#endif

  Wire.begin(INA_SDA, INA_SCL);
  inaReady = ina219.begin();
  if (inaReady) {
    // 12V rail with up to ~2A loads. Resolution ~0.8mA, 4mV bus.
    ina219.setCalibration_32V_2A();
    Serial.println("INA219 initialized (32V / 2A range).");
  } else {
    Serial.println("INA219 not found on I2C bus (SDA=21, SCL=22).");
  }

  display.begin();
  display.setContrast(255);
  display.setPowerSave(0);

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 12, "Power Monitor");
  display.drawStr(0, 30, inaReady ? "INA219 OK" : "INA219 NOT FOUND");
  display.drawStr(0, 48, "Range: 32V / 2A");
  display.sendBuffer();
  delay(800);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastRefreshMs < REFRESH_MS) {
    return;
  }
  lastRefreshMs = now;

  if (!inaReady) {
    drawError("Check I2C wiring");
    return;
  }

  const float shuntmV    = ina219.getShuntVoltage_mV();
  const float busV       = ina219.getBusVoltage_V();
  const float current_mA = ina219.getCurrent_mA();
  const float power_mW   = ina219.getPower_mW();

  Serial.printf("Vbus=%.2fV  Vshunt=%.2fmV  I=%.2fmA  P=%.2fmW\n",
                busV, shuntmV, current_mA, power_mW);

  drawReadings(busV, shuntmV, current_mA, power_mW);
}
