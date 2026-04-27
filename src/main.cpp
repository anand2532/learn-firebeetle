#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// SPI pins
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   16
#define OLED_CS   5
#define OLED_RST  17

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);

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
  display.drawCircle(cx, cy, radius, WHITE);
  display.drawCircle(cx, cy, radius - 1, WHITE);
}

void drawEyeOpen(int x, int y) {
  display.fillCircle(x, y, 4, WHITE);
  display.fillCircle(x, y, 2, BLACK);
}

void drawEyeClosed(int x, int y) {
  display.drawLine(x - 5, y, x + 5, y, WHITE);
}

void drawMouthHappy(int x, int y) {
  display.drawCircleHelper(x, y, 10, 0x03, WHITE);
  display.drawCircleHelper(x, y + 1, 10, 0x03, WHITE);
}

void drawMouthNeutral(int x, int y) {
  display.drawLine(x - 10, y + 6, x + 10, y + 6, WHITE);
  display.drawLine(x - 10, y + 7, x + 10, y + 7, WHITE);
}

void drawMouthSad(int x, int y) {
  display.drawCircleHelper(x, y + 18, 10, 0x0C, WHITE);
  display.drawCircleHelper(x, y + 19, 10, 0x0C, WHITE);
}

void drawMouthSurprised(int x, int y) {
  display.drawCircle(x, y + 6, 6, WHITE);
  display.drawCircle(x, y + 6, 5, WHITE);
}

void drawCheeks(int xLeft, int xRight, int y) {
  display.drawCircle(xLeft, y, 2, WHITE);
  display.drawCircle(xRight, y, 2, WHITE);
}

void drawTitle(Emotion emo) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2, 2);
  display.print("Emotion: ");
  switch (emo) {
    case EMO_HAPPY:
      display.print("Happy");
      break;
    case EMO_NEUTRAL:
      display.print("Calm");
      break;
    case EMO_SAD:
      display.print("Sad");
      break;
    case EMO_SURPRISED:
      display.print("Wow");
      break;
    case EMO_WINK:
      display.print("Wink");
      break;
  }
}

void drawSmiley(Emotion emo, bool blinking, int bobOffset) {
  const int cx = SCREEN_WIDTH / 2;
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
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED init failed");
    while (true);
  }

  // Boot confirmation pattern to verify panel is alive before animation starts.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("OLED OK");
  display.drawRect(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12, WHITE);
  display.drawLine(0, 12, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, WHITE);
  display.drawLine(SCREEN_WIDTH - 1, 12, 0, SCREEN_HEIGHT - 1, WHITE);
  display.display();
  delay(900);
  display.clearDisplay();
  display.display();

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

  display.clearDisplay();
  drawTitle(currentEmotion);
  drawSmiley(currentEmotion, blinking, bobOffset);
  display.display();

  delay(25);
}