#pragma once
#define LED_COUNT 24
const uint8_t CLOUDS_BRIGHTNESS = 56;
static float cloudsPhase1 = 0.0f;
static float cloudsPhase2 = 0.0f;
const float CLOUDS_SPEED1 = 0.0525f * 1.2f;
const float CLOUDS_SPEED2 = 0.0270f * 1.2f;
const unsigned long CLOUDS_FRAME_INTERVAL = 25;
static unsigned long cloudsLastFrameTime = 0;

void clouds() {
  unsigned long now = millis();
  if (now - cloudsLastFrameTime < CLOUDS_FRAME_INTERVAL) return;
  cloudsLastFrameTime = now;

  for (int i = 0; i < LED_COUNT; i++) {
    float v =
      sin(cloudsPhase1 + i * 0.35f) * 0.6f +
      sin(cloudsPhase2 + i * 0.18f) * 0.4f;
    uint8_t b = constrain(CLOUDS_BRIGHTNESS + v * CLOUDS_BRIGHTNESS, 0, 255);
    leds[i] = CRGB(b, b, b);
  }
  cloudsPhase1 += CLOUDS_SPEED1;
  cloudsPhase2 += CLOUDS_SPEED2;
  //FastLED.show();
}