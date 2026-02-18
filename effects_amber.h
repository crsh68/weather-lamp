#pragma once
void amberEffect() {
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = CRGB(255, 120, 0);
  }
 //FastLED.show();
}