#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_INA238.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

// --- I2C / sensors ---
#define INA_SDA 21
#define INA_SCL 22

Adafruit_INA238 ina238;            // default address 0x40
Adafruit_INA219 ina219(0x41);      // A0 jumpered for 0x41

bool ina238Ready = false;
bool ina219Ready = false;

const unsigned long REFRESH_MS = 500;
unsigned long lastRefreshMs = 0;
const unsigned long UPLOAD_MS = 3000;
unsigned long lastUploadMs = 0;

const char* WIFI_SSID = "Airtel_vyom";
const char* WIFI_PASSWORD = "Vyom123@";
const char* UPLOAD_URL = "http://15.207.18.221:5000/ingest";
const char* DEVICE_ID = "firebeetle32-dual-ina";

// INA238 hardware shunt on board (e.g. Adafruit INA238 breakout uses R015 = 15 mOhm).
constexpr float INA238_SHUNT_OHMS       = 0.015f;
// Max expected current (A) for SHUNT_CAL scaling only; raise if your MPPT can exceed this.
constexpr float INA238_MAX_EXPECTED_AMPS = 10.0f;

struct SensorReading {
  bool ready;
  float busV;
  float current_mA;
  float power_mW;
  float shuntmV;
  bool reverse;
  bool railed;
  bool noLoad;
};

static void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.printf("Connecting WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connect timeout, will retry.");
  }
}

static String buildPayload(const SensorReading& r238, const SensorReading& r219) {
  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"uptime_ms\":" + String(millis()) + ",";
  payload += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"ina238\":{";
  payload += "\"ready\":" + String(r238.ready ? "true" : "false") + ",";
  payload += "\"bus_v\":" + String(r238.busV, 3) + ",";
  payload += "\"current_ma\":" + String(r238.current_mA, 3) + ",";
  payload += "\"power_mw\":" + String(r238.power_mW, 3) + ",";
  payload += "\"shunt_mv\":" + String(r238.shuntmV, 3);
  payload += "},";
  payload += "\"ina219\":{";
  payload += "\"ready\":" + String(r219.ready ? "true" : "false") + ",";
  payload += "\"bus_v\":" + String(r219.busV, 3) + ",";
  payload += "\"current_ma\":" + String(r219.current_mA, 3) + ",";
  payload += "\"power_mw\":" + String(r219.power_mW, 3) + ",";
  payload += "\"shunt_mv\":" + String(r219.shuntmV, 3);
  payload += "}";
  payload += "}";
  return payload;
}

static void uploadReading(const SensorReading& r238, const SensorReading& r219) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
  }

  HTTPClient http;
  http.begin(UPLOAD_URL);
  http.addHeader("Content-Type", "application/json");

  const String payload = buildPayload(r238, r219);
  const int responseCode = http.POST(payload);
  if (responseCode > 0) {
    Serial.printf("Upload OK, HTTP %d\n", responseCode);
  } else {
    Serial.printf("Upload failed: %s\n", http.errorToString(responseCode).c_str());
  }
  http.end();
}

static void drawColumn(int x, const char* title, const SensorReading& r) {
  display.drawStr(x + 14, 22, title);

  if (!r.ready) {
    display.drawStr(x, 38, "NO I2C");
    return;
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%5.2f V", r.busV);
  display.drawStr(x, 36, buf);

  if (fabsf(r.current_mA) >= 1000.0f) {
    snprintf(buf, sizeof(buf), "%5.2f A", r.current_mA / 1000.0f);
  } else {
    snprintf(buf, sizeof(buf), "%5.0f mA", r.current_mA);
  }
  display.drawStr(x, 47, buf);

  if (fabsf(r.power_mW) >= 1000.0f) {
    snprintf(buf, sizeof(buf), "%5.2f W", r.power_mW / 1000.0f);
  } else {
    snprintf(buf, sizeof(buf), "%5.0f mW", r.power_mW);
  }
  display.drawStr(x, 58, buf);

  const char* status = "OK";
  if (r.railed) status = "RAIL";
  else if (r.reverse) status = "REV";
  else if (r.noLoad) status = "NOLOAD";
  display.drawStr(x, 64, status);
}

static void drawDual(const SensorReading& a, const SensorReading& b) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(16, 10, "Dual Power Monitor");
  display.drawHLine(0, 12, 128);
  display.drawHLine(0, 24, 128);
  display.drawVLine(64, 12, 52);
  drawColumn(0,  "INA238", a);
  drawColumn(66, "INA219", b);
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Booting dual power monitor (INA238 + INA219)...");
  Serial.printf("OLED driver: %s, bus: %s\n",
                OLED_IS_SH1106 ? "SH1106" : "SSD1306",
                OLED_USE_SW_SPI ? "Software SPI" : "Hardware SPI");

#if !OLED_USE_SW_SPI
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
#endif

  Wire.begin(INA_SDA, INA_SCL);
  Wire.setClock(100000);

  ina238Ready = ina238.begin();
  if (ina238Ready) {
    ina238.setShunt(INA238_SHUNT_OHMS, INA238_MAX_EXPECTED_AMPS);
  }

  ina219Ready = ina219.begin();
  if (ina219Ready) {
    ina219.setCalibration_32V_2A();
  }

  Serial.printf("INA238 %s @0x40, INA219 %s @0x41\n",
                ina238Ready ? "OK" : "MISSING",
                ina219Ready ? "OK" : "MISSING");
  connectWifi();

  display.begin();
  display.setContrast(255);
  display.setPowerSave(0);

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(16, 10, "Dual Power Monitor");
  display.drawHLine(0, 12, 128);
  char buf[20];
  snprintf(buf, sizeof(buf), "INA238: %s", ina238Ready ? "OK" : "MISSING");
  display.drawStr(0, 28, buf);
  snprintf(buf, sizeof(buf), "INA219: %s", ina219Ready ? "OK" : "MISSING");
  display.drawStr(0, 42, buf);
  display.drawStr(0, 60, "I2C SDA=21 SCL=22");
  display.sendBuffer();
  delay(900);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastRefreshMs < REFRESH_MS) {
    return;
  }
  lastRefreshMs = now;

  SensorReading r238 = {};
  r238.ready = ina238Ready;
  if (ina238Ready) {
    r238.busV       = ina238.readBusVoltage();
    r238.shuntmV    = ina238.readShuntVoltage() * 1000.0f;
    r238.current_mA = ina238.readCurrent();
    r238.power_mW   = ina238.readPower();
    r238.reverse = r238.current_mA < -5.0f;
    r238.noLoad  = (r238.busV > 5.0f && fabsf(r238.shuntmV) < 1.0f);
  }

  SensorReading r219 = {};
  r219.ready = ina219Ready;
  if (ina219Ready) {
    r219.busV       = ina219.getBusVoltage_V();
    r219.shuntmV    = ina219.getShuntVoltage_mV();
    r219.current_mA = ina219.getCurrent_mA();
    r219.power_mW   = ina219.getPower_mW();
    r219.reverse = r219.current_mA < -5.0f;
    // INA219 shunt ADC saturates at +/-320 mV for the +/-2A calibration.
    r219.railed  = fabsf(r219.shuntmV) >= 319.0f;
    r219.noLoad  = (r219.busV > 5.0f && fabsf(r219.shuntmV) < 1.0f);
  }

  Serial.printf("INA238: V=%.2f I=%.1fmA P=%.1fmW  |  INA219: V=%.2f I=%.1fmA P=%.1fmW\n",
                r238.busV, r238.current_mA, r238.power_mW,
                r219.busV, r219.current_mA, r219.power_mW);

  drawDual(r238, r219);

  if (now - lastUploadMs >= UPLOAD_MS) {
    lastUploadMs = now;
    uploadReading(r238, r219);
  }
}
