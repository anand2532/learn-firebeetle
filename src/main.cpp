#include <Arduino.h>
#include <math.h>

namespace {
constexpr uint8_t kLedPin = LED_BUILTIN; // FireBeetle32 internal LED is GPIO2.
constexpr uint8_t kPwmChannel = 0;
constexpr uint32_t kPwmFreqHz = 5000;
constexpr uint8_t kPwmResolutionBits = 8;
constexpr uint16_t kMaxPwm = (1U << kPwmResolutionBits) - 1;

// 12 second loop:
// - 8 seconds of smooth breathing
// - 4 seconds of double-heartbeat pulses
constexpr uint32_t kLoopDurationMs = 12000;
constexpr uint32_t kBreathDurationMs = 8000;

float smoothBreath(float t) {
  // 0..1 -> 0..1 with a gentle sine-shaped envelope.
  const float x = t * TWO_PI;
  return 0.5F - 0.5F * cosf(x);
}

float heartbeat(float t) {
  // Two gaussian-like pulses, then fade to dark.
  const float p1 = expf(-120.0F * (t - 0.18F) * (t - 0.18F));
  const float p2 = 0.8F * expf(-120.0F * (t - 0.38F) * (t - 0.38F));
  const float trail = 0.12F * expf(-5.0F * t);
  float v = p1 + p2 + trail;
  if (v > 1.0F) v = 1.0F;
  return v;
}

void writeLedLevel(float level) {
  if (level < 0.0F) level = 0.0F;
  if (level > 1.0F) level = 1.0F;

  // Gamma correction for visually smoother brightness steps.
  const float gammaCorrected = powf(level, 2.2F);
  const uint32_t duty = static_cast<uint32_t>(gammaCorrected * kMaxPwm + 0.5F);
  ledcWrite(kPwmChannel, duty);
}
} // namespace

void setup() {
  ledcSetup(kPwmChannel, kPwmFreqHz, kPwmResolutionBits);
  ledcAttachPin(kLedPin, kPwmChannel);
}

void loop() {
  const uint32_t now = millis();
  const uint32_t phaseMs = now % kLoopDurationMs;

  float level = 0.0F;
  if (phaseMs < kBreathDurationMs) {
    const float t = static_cast<float>(phaseMs) / static_cast<float>(kBreathDurationMs);
    level = smoothBreath(t);
  } else {
    const uint32_t heartbeatMs = phaseMs - kBreathDurationMs;
    const float t = static_cast<float>(heartbeatMs) /
                    static_cast<float>(kLoopDurationMs - kBreathDurationMs);
    level = heartbeat(t);
  }

  writeLedLevel(level);
  delay(10); // Small delay keeps animation smooth and stable.
}