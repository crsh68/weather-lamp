#pragma once
const uint8_t WIND_BRIGHTNESS = 64;
static float windPhase1 = 0.0f;
static float windPhase2 = 0.0f;
const float SPEED1 = 0.0525f * 2.0f;
const float SPEED2 = 0.0270f * 2.0f;
static float colorPhase = 0.0f;
const float COLOR_SPEED = 0.012f;
const unsigned long WIND_FRAME_INTERVAL = 20;
static unsigned long windLastFrameTime = 0;

void wind() {
  unsigned long now = millis();
  if (now - windLastFrameTime < WIND_FRAME_INTERVAL) return;
  windLastFrameTime = now;

  for (int i = 0; i < LED_COUNT; i++) {
    float v =
      sin(windPhase1 + i * 0.4f) * 0.6f +
      sin(windPhase2 + i * 0.2f) * 0.4f;

    uint8_t b = WIND_BRIGHTNESS + v * WIND_BRIGHTNESS;
    leds[i] = CRGB(0, b, b);
  }

  windPhase1 += SPEED1;
  windPhase2 += SPEED2;
  colorPhase += COLOR_SPEED;
  //FastLED.show();
}