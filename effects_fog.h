#pragma once
void fog() {
  static uint8_t fog[24] = {0};
  static uint8_t targetFog[24] = {0};
  static int cloudCenters[3] = {-1, -1, -1};
  static unsigned long lastUpdate = 0;

  unsigned long now = millis();
  if (now - lastUpdate < 40) return;
  lastUpdate = now;

  for (int i = 0; i < LED_COUNT; i++) {
    if (fog[i] < targetFog[i]) fog[i] += 6;
    else if (fog[i] > targetFog[i]) fog[i] -= 4;
  }

  for (int c = 0; c < 3; c++) {
    if (cloudCenters[c] == -1 || random(100) < 5) {
      cloudCenters[c] = random(LED_COUNT);
    }
  }

  memset(targetFog, 0, sizeof(targetFog));

  for (int c = 0; c < 3; c++) {
    int center = cloudCenters[c];
    for (int i = 0; i < LED_COUNT; i++) {
      int d = abs(i - center);
      if (d < 6) {
        targetFog[i] = max(targetFog[i], (uint8_t)(80 - d * 12));
      }
    }
  }

  for (int i = 0; i < LED_COUNT; i++) {
    // Dark blue background with white fog overlay
    uint8_t fogValue = fog[i];
    leds[i] = CRGB(fogValue, fogValue, min(255, fogValue + 30));
  }
  //FastLED.show();
}