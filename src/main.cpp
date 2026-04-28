#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>

// SPI pins
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   16
#define OLED_CS   5
#define OLED_RST  17

// Set to 1 for SH1106 modules, 0 for SSD1306 modules.
// If your screen is blank, first keep this as 1. If still blank, change to 0 and re-upload.
#define OLED_IS_SH1106 1

// Software SPI is slower but often more reliable for bring-up.
// Keep as 1 while debugging blank screens.
#define OLED_USE_SW_SPI 1

// Some modules do not wire the reset pin. Set to 1 to ignore reset pin.
#define OLED_IGNORE_RST_PIN 0

#if OLED_IGNORE_RST_PIN
#define OLED_RST_PIN U8X8_PIN_NONE
#else
#define OLED_RST_PIN OLED_RST
#endif

#if OLED_IS_SH1106 && OLED_USE_SW_SPI
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST_PIN);
#elif OLED_IS_SH1106
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI display(U8G2_R0, OLED_CS, OLED_DC, OLED_RST_PIN);
#elif OLED_USE_SW_SPI
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST_PIN);
#else
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI display(U8G2_R0, OLED_CS, OLED_DC, OLED_RST_PIN);
#endif

const unsigned long SCENE_INTRO_MS = 12000;
const unsigned long SCENE_WALK_MS = 26000;
const unsigned long SCENE_CHARGE_MS = 20000;
const unsigned long SCENE_PUNCH_MS = 15000;
const unsigned long SCENE_COOLDOWN_MS = 17000;
const unsigned long MOVIE_LOOP_MS =
    SCENE_INTRO_MS + SCENE_WALK_MS + SCENE_CHARGE_MS + SCENE_PUNCH_MS + SCENE_COOLDOWN_MS;

unsigned long movieStartMs = 0;

void drawSpeedLines(uint8_t phase) {
  for (int i = 0; i < 7; ++i) {
    int x = static_cast<int>((phase * 9 + i * 17) % 128);
    int y = 14 + i * 7;
    display.drawLine(x, y, x + 12, y);
  }
}

void drawDustCloud(int x, int y, int spread) {
  display.drawCircle(x, y, 2 + spread / 3);
  display.drawCircle(x + 4, y + 1, 2 + spread / 4);
  display.drawCircle(x - 4, y + 1, 2 + spread / 4);
}

void drawHero(int x, int y, bool punchPose, bool eyesSquint, uint8_t legPhase) {
  // Head
  display.drawCircle(x, y - 12, 7);
  display.drawCircle(x, y - 12, 6);

  // Eyes
  if (eyesSquint) {
    display.drawLine(x - 4, y - 13, x - 1, y - 13);
    display.drawLine(x + 1, y - 13, x + 4, y - 13);
  } else {
    display.drawPixel(x - 3, y - 13);
    display.drawPixel(x + 3, y - 13);
  }

  // Cape
  display.drawLine(x - 5, y - 5, x - 11, y + 5);
  display.drawLine(x - 4, y - 4, x - 13, y + 10);
  display.drawLine(x - 3, y - 3, x - 10, y + 12);

  // Body
  display.drawLine(x, y - 5, x, y + 10);
  display.drawLine(x - 1, y - 5, x - 1, y + 10);

  // Arms
  if (punchPose) {
    display.drawLine(x, y - 1, x + 14, y - 4);
    display.drawLine(x, y, x + 14, y - 3);
    display.drawDisc(x + 16, y - 4, 2);
    display.drawLine(x - 1, y + 1, x - 7, y + 5);
  } else {
    display.drawLine(x, y, x + 6, y + 2);
    display.drawLine(x - 1, y + 1, x - 7, y + 4);
  }

  // Legs (simple walk cycle)
  if (legPhase & 1U) {
    display.drawLine(x - 1, y + 10, x - 6, y + 18);
    display.drawLine(x, y + 10, x + 4, y + 18);
  } else {
    display.drawLine(x - 1, y + 10, x - 4, y + 18);
    display.drawLine(x, y + 10, x + 6, y + 18);
  }
}

void drawEnemy(int x, int y, uint8_t wobble) {
  int wobbleY = (wobble & 1U) ? 1 : 0;
  display.drawFrame(x - 6, y - 10 + wobbleY, 12, 20);
  display.drawCircle(x, y - 14 + wobbleY, 5);
  display.drawPixel(x - 2, y - 15 + wobbleY);
  display.drawPixel(x + 2, y - 15 + wobbleY);
  display.drawLine(x - 3, y - 12 + wobbleY, x + 3, y - 12 + wobbleY);
}

void drawBanner(const char* text) {
  display.drawBox(0, 0, 128, 11);
  display.setDrawColor(0);
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(2, 9, text);
  display.setDrawColor(1);
}

void drawSceneIntro(unsigned long localMs) {
  drawBanner("ANIME HERO TRAINING");
  int heroX = 18 + static_cast<int>((localMs / 220) % 6);
  drawHero(heroX, 41, false, false, static_cast<uint8_t>(localMs / 180));
  display.drawStr(56, 30, "100 PUSH UPS");
  display.drawStr(56, 44, "100 SIT UPS");
  display.drawStr(56, 58, "RUN 10 KM");
}

void drawSceneWalk(unsigned long localMs) {
  drawBanner("CITY PATROL");
  int heroX = 8 + static_cast<int>((localMs / 120) % 112);
  drawHero(heroX, 42, false, false, static_cast<uint8_t>(localMs / 120));
  display.drawLine(0, 60, 127, 60);
  for (int i = 0; i < 4; ++i) {
    int bx = (i * 34 + static_cast<int>(localMs / 6)) % 128;
    display.drawFrame(bx, 44, 18, 16);
    display.drawPixel(bx + 4, 48);
    display.drawPixel(bx + 10, 50);
    display.drawPixel(bx + 14, 55);
  }
}

void drawSceneCharge(unsigned long localMs) {
  drawBanner("SERIOUS MODE");
  int heroX = 42;
  drawHero(heroX, 42, false, true, static_cast<uint8_t>(localMs / 120));
  drawEnemy(100, 45, static_cast<uint8_t>(localMs / 220));

  uint8_t aura = static_cast<uint8_t>((localMs / 140) % 4);
  for (uint8_t r = 0; r < aura + 2; ++r) {
    display.drawCircle(heroX, 30, 10 + r * 3);
  }
  drawSpeedLines(static_cast<uint8_t>(localMs / 90));
}

void drawScenePunch(unsigned long localMs) {
  drawBanner("ONE HIT ATTACK!");
  int heroX = 50 + static_cast<int>((localMs / 80) % 10);
  drawHero(heroX, 41, true, true, static_cast<uint8_t>(localMs / 80));

  int impactX = 92 + static_cast<int>((localMs / 60) % 8);
  int impactY = 35;
  display.drawCircle(impactX, impactY, 8);
  display.drawCircle(impactX, impactY, 12);
  display.drawLine(impactX - 16, impactY, impactX + 16, impactY);
  display.drawLine(impactX, impactY - 16, impactX, impactY + 16);
  display.drawStr(82, 58, "B O O M");
  drawDustCloud(106, 55, static_cast<int>((localMs / 120) % 5));
}

void drawSceneCooldown(unsigned long localMs) {
  drawBanner("BACK TO NORMAL");
  int heroX = 20 + static_cast<int>((localMs / 300) % 4);
  drawHero(heroX, 42, false, false, static_cast<uint8_t>(localMs / 200));
  drawDustCloud(86, 52, static_cast<int>((localMs / 240) % 4));
  drawDustCloud(96, 55, static_cast<int>((localMs / 210) % 4));
  drawDustCloud(106, 53, static_cast<int>((localMs / 190) % 4));
  display.drawStr(58, 30, "THREAT GONE");
  display.drawStr(58, 44, "GROCERY TIME");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Booting OLED test...");
  Serial.printf("Driver: %s\n", OLED_IS_SH1106 ? "SH1106" : "SSD1306");
  Serial.printf("Bus: %s\n", OLED_USE_SW_SPI ? "Software SPI" : "Hardware SPI");
  Serial.printf("Pins CLK=%d MOSI=%d CS=%d DC=%d RST=%d\n", OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST_PIN);

#if !OLED_USE_SW_SPI
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
#endif
  delay(120);

  display.begin();
  display.setContrast(255);
  display.setPowerSave(0);

  // Strong visible diagnostic sequence.
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 12, "OLED INIT OK");
  display.drawFrame(0, 16, 128, 48);
  display.drawLine(0, 16, 127, 63);
  display.drawLine(127, 16, 0, 63);
  display.sendBuffer();
  delay(1000);

  display.clearBuffer();
  display.drawBox(0, 0, 128, 64);
  display.sendBuffer();
  delay(500);

  display.clearBuffer();
  display.sendBuffer();
  delay(250);

  movieStartMs = millis();
}

void loop() {
  const unsigned long elapsed = (millis() - movieStartMs) % MOVIE_LOOP_MS;
  display.clearBuffer();
  if (elapsed < SCENE_INTRO_MS) {
    drawSceneIntro(elapsed);
  } else if (elapsed < SCENE_INTRO_MS + SCENE_WALK_MS) {
    drawSceneWalk(elapsed - SCENE_INTRO_MS);
  } else if (elapsed < SCENE_INTRO_MS + SCENE_WALK_MS + SCENE_CHARGE_MS) {
    drawSceneCharge(elapsed - SCENE_INTRO_MS - SCENE_WALK_MS);
  } else if (elapsed < SCENE_INTRO_MS + SCENE_WALK_MS + SCENE_CHARGE_MS + SCENE_PUNCH_MS) {
    drawScenePunch(elapsed - SCENE_INTRO_MS - SCENE_WALK_MS - SCENE_CHARGE_MS);
  } else {
    drawSceneCooldown(elapsed - SCENE_INTRO_MS - SCENE_WALK_MS - SCENE_CHARGE_MS - SCENE_PUNCH_MS);
  }
  display.sendBuffer();
  delay(33);
}