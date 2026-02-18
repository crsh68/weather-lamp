#pragma once

void snow() {

  const int MAX_FLAKES = 6;

  static float pos[MAX_FLAKES] = {0};
  static float speed[MAX_FLAKES] = {0};
  static uint8_t baseBright[MAX_FLAKES] = {0};
  static bool active[MAX_FLAKES] = {false};

  static unsigned long lastFrame = 0;
  static unsigned long lastSpawn = 0;

  unsigned long now = millis();
  if (now - lastFrame < 40) return;
  lastFrame = now;

  fill_solid(leds, LED_COUNT, CRGB::Black);

  for (int i = 0; i < MAX_FLAKES; i++) {

    if (active[i]) {

      int p = (int)pos[i];
      float frac = pos[i] - p;  // decimalni dio pozicije (0.0 = na LED p, 0.9 = skoro na p-1)

      if (p >= 0 && p < LED_COUNT) {

        // fade prema dnu ali nikad na 0
        float heightFactor = 0.2f +
            (0.8f * ((float)pos[i] / (LED_COUNT - 1)));

        uint8_t targetBright = baseBright[i] * heightFactor;

        // LED gore (odakle dolazi) - dobiva intenzitet kako frac raste
        leds[p].r = max(leds[p].r, (uint8_t)(targetBright * frac));
        leds[p].g = max(leds[p].g, (uint8_t)(targetBright * frac));
        leds[p].b = max(leds[p].b, (uint8_t)(targetBright * frac));

        // LED dolje (kamo ide) - gubi intenzitet kako frac raste
        if (p - 1 >= 0) {
          leds[p - 1].r = max(leds[p - 1].r, (uint8_t)(targetBright * (1.0f - frac)));
          leds[p - 1].g = max(leds[p - 1].g, (uint8_t)(targetBright * (1.0f - frac)));
          leds[p - 1].b = max(leds[p - 1].b, (uint8_t)(targetBright * (1.0f - frac)));
        }

        // pomak tek NAKON crtanja
        pos[i] -= speed[i];

      } else {
        active[i] = false;
      }
    }
  }

  // spawn nove pahulje
  if (now - lastSpawn > 300 && random(100) < 45) {

    for (int i = 0; i < MAX_FLAKES; i++) {

      if (!active[i]) {

        active[i] = true;
        pos[i] = LED_COUNT - 1;   // LED23 će sada svijetliti

        baseBright[i] = random(120, 255);

        // originalna brzina:
        float baseSpeed = 0.02f +
          ((float)(baseBright[i] - 120) / 135.0f) * 0.07f;

        // +75%
        speed[i] = baseSpeed * 1.75f;

        lastSpawn = now;
        break;
      }
    }
  }

  //FastLED.show();
}