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

enum Emotion {
  EMO_HAPPY = 0,
  EMO_NEUTRAL,
  EMO_SAD,
  EMO_SURPRISED,
  EMO_WINK
};

const unsigned long EMOTION_HOLD_MS = 2600;
const unsigned long BLINK_PERIOD_MS = 3200;
const unsigned long BLINK_DURATION_MS = 140;

unsigned long lastEmotionChangeMs = 0;
Emotion currentEmotion = EMO_HAPPY;

void drawFaceOutline(int cx, int cy, int radius) {
  display.drawCircle(cx, cy, radius);
  display.drawCircle(cx, cy, radius - 1);
}

void drawEyeOpen(int x, int y) {
  display.drawDisc(x, y, 4);
  display.setDrawColor(0);
  display.drawDisc(x, y, 2);
  display.setDrawColor(1);
}

void drawEyeClosed(int x, int y) {
  display.drawLine(x - 5, y, x + 5, y);
}

void drawMouthHappy(int x, int y) {
  display.drawArc(x, y + 2, 11, 30, 150);
  display.drawArc(x, y + 3, 11, 30, 150);
}

void drawMouthNeutral(int x, int y) {
  display.drawLine(x - 10, y + 6, x + 10, y + 6);
  display.drawLine(x - 10, y + 7, x + 10, y + 7);
}

void drawMouthSad(int x, int y) {
  display.drawArc(x, y + 18, 11, 210, 250);
  display.drawArc(x, y + 19, 11, 210, 250);
}

void drawMouthSurprised(int x, int y) {
  display.drawCircle(x, y + 6, 6);
  display.drawCircle(x, y + 6, 5);
}

void drawCheeks(int xLeft, int xRight, int y) {
  display.drawCircle(xLeft, y, 2);
  display.drawCircle(xRight, y, 2);
}

void drawTitle(Emotion emo) {
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(2, 10, "Emotion:");
  switch (emo) {
    case EMO_HAPPY:
      display.drawStr(52, 10, "Happy");
      break;
    case EMO_NEUTRAL:
      display.drawStr(52, 10, "Calm");
      break;
    case EMO_SAD:
      display.drawStr(52, 10, "Sad");
      break;
    case EMO_SURPRISED:
      display.drawStr(52, 10, "Wow");
      break;
    case EMO_WINK:
      display.drawStr(52, 10, "Wink");
      break;
  }
}

void drawSmiley(Emotion emo, bool blinking, int bobOffset) {
  const int cx = 64;
  const int cy = 37 + bobOffset;
  const int eyeY = cy - 7;
  const int leftEyeX = cx - 11;
  const int rightEyeX = cx + 11;

  drawFaceOutline(cx, cy, 24);

  bool leftClosed = blinking;
  bool rightClosed = blinking;

  if (emo == EMO_WINK) {
    leftClosed = true;
    rightClosed = false;
  }

  if (leftClosed) {
    drawEyeClosed(leftEyeX, eyeY);
  } else {
    drawEyeOpen(leftEyeX, eyeY);
  }

  if (rightClosed) {
    drawEyeClosed(rightEyeX, eyeY);
  } else {
    drawEyeOpen(rightEyeX, eyeY);
  }

  switch (emo) {
    case EMO_HAPPY:
      drawMouthHappy(cx, cy + 1);
      drawCheeks(cx - 16, cx + 16, cy + 2);
      break;
    case EMO_NEUTRAL:
      drawMouthNeutral(cx, cy + 2);
      break;
    case EMO_SAD:
      drawMouthSad(cx, cy + 2);
      break;
    case EMO_SURPRISED:
      drawMouthSurprised(cx, cy + 1);
      break;
    case EMO_WINK:
      drawMouthHappy(cx, cy + 1);
      break;
  }
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

  lastEmotionChangeMs = millis();
}

void loop() {
  const unsigned long now = millis();

  if (now - lastEmotionChangeMs >= EMOTION_HOLD_MS) {
    currentEmotion = static_cast<Emotion>((static_cast<int>(currentEmotion) + 1) % 5);
    lastEmotionChangeMs = now;
  }

  const bool blinking = (now % BLINK_PERIOD_MS) < BLINK_DURATION_MS;
  const int bobOffset = static_cast<int>((now / 260) % 2) == 0 ? 0 : 1;

  display.clearBuffer();
  drawTitle(currentEmotion);
  drawSmiley(currentEmotion, blinking, bobOffset);
  display.sendBuffer();

  delay(25);
}