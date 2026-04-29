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
bool warnedRail = false;

void drawReadings(float busV, float shuntmV, float loadV, float current_mA, float power_mW, bool reverseFlow, bool suspectWiring, bool shuntRailed) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 10, "INA219 Monitor");
  display.drawHLine(0, 12, 128);

  char line[24];
  display.setFont(u8g2_font_6x12_tf);
  snprintf(line, sizeof(line), "Bus  %5.2f V", busV);
  display.drawStr(0, 24, line);
  snprintf(line, sizeof(line), "Load %5.2f V", loadV);
  display.drawStr(0, 35, line);
  snprintf(line, sizeof(line), "I %7.1f mA", current_mA);
  display.drawStr(0, 46, line);
  snprintf(line, sizeof(line), "P %7.1f mW", power_mW);
  display.drawStr(0, 57, line);

  if (shuntRailed) {
    display.drawStr(0, 64, "SHUNT RAIL");
  } else if (reverseFlow) {
    display.drawStr(0, 64, "DIR:REV");
  } else if (suspectWiring) {
    display.drawStr(0, 64, "CHK GND/VIN");
  }

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
  Wire.setClock(100000);
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
  const float loadV      = busV + (shuntmV / 1000.0f);
  const bool shuntRailed = fabsf(shuntmV) >= 319.0f;
  const float power_mW   = shuntRailed ? 0.0f : (loadV * current_mA);
  const bool reverseFlow = current_mA < -5.0f;
  const bool suspectWiring = (busV < 2.0f && fabsf(shuntmV) > 20.0f) || shuntRailed;

  Serial.printf("Vbus=%.2fV  Vload=%.2fV  Vshunt=%.2fmV  I=%.2fmA  P=%.2fmW",
                busV, loadV, shuntmV, current_mA, power_mW);
  if (shuntRailed) {
    Serial.print("  [FAULT: shunt input railed +/-320mV; input saturated/floating]");
  } else if (reverseFlow) {
    Serial.print("  [WARN: reverse current; swap VIN+ and VIN-]");
  } else if (suspectWiring) {
    Serial.print("  [WARN: low bus + high shunt; check common GND and load path]");
  }
  Serial.println();

  if (shuntRailed && !warnedRail) {
    warnedRail = true;
    Serial.println("DIAG: INA219 shunt channel is saturated.");
    Serial.println("DIAG: Common causes:");
    Serial.println("  1) VIN+ and VIN- not truly in series with the load.");
    Serial.println("  2) Load negative is not returned to 12V supply negative.");
    Serial.println("  3) 12V supply negative not tied to ESP32/INA219 GND.");
    Serial.println("  4) VIN- node floating (no real load connected).");
    Serial.println("DIAG: Use a multimeter now:");
    Serial.println("  - Measure VIN+ to GND (should be ~12V)");
    Serial.println("  - Measure VIN- to GND (should be near VIN+ when idle)");
    Serial.println("  - Measure VIN+ to VIN- (should be near 0mV at light/no load)");
  }

  drawReadings(busV, shuntmV, loadV, current_mA, power_mW, reverseFlow, suspectWiring, shuntRailed);
}
