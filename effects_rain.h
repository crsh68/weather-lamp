#pragma once

// ===== DRIZZLE =====
// max 3 kapi, sporije

void drizzle() {

  const int MAX_DROPS = 3;

  static float pos[MAX_DROPS] = {0};
  static float speed[MAX_DROPS] = {0};
  static uint8_t bright[MAX_DROPS] = {0};
  static bool active[MAX_DROPS] = {false};

  static unsigned long lastFrame = 0;
  static unsigned long lastSpawn = 0;

  unsigned long now = millis();
  if (now - lastFrame < 35) return;
  lastFrame = now;

  fill_solid(leds, LED_COUNT, CRGB::Black);

  // pomak prema dolje (prema LED0)
  for (int i = 0; i < MAX_DROPS; i++) {
    if (active[i]) {
      
      int p = (int)pos[i];
      pos[i] -= speed[i];

      if (p >= 0 && p < LED_COUNT) {
        leds[p] = CRGB(0, 0, bright[i]);
      } else {
        active[i] = false;
      }
    }
  }

  // spawn na vrhu
  if (now - lastSpawn > 250 && random(100) < 50) {
    for (int i = 0; i < MAX_DROPS; i++) {
      if (!active[i]) {
        active[i] = true;
        pos[i] = LED_COUNT - 1;
        speed[i] = random(2, 4) * 0.05f;   // sporije od rain
        bright[i] = random(100, 160);
        lastSpawn = now;
        break;
      }
    }
  }

  //FastLED.show();
}


// ===== RAIN =====
// max 8 kapi, brže

void rain() {

  const int MAX_DROPS = 8;

  static float pos[MAX_DROPS] = {0};
  static float speed[MAX_DROPS] = {0};
  static uint8_t bright[MAX_DROPS] = {0};
  static bool active[MAX_DROPS] = {false};

  static unsigned long lastFrame = 0;
  static unsigned long lastSpawn = 0;

  unsigned long now = millis();
  if (now - lastFrame < 35) return;
  lastFrame = now;

  fill_solid(leds, LED_COUNT, CRGB::Black);

  for (int i = 0; i < MAX_DROPS; i++) {
    if (active[i]) {
                          // prema dolje
      int p = (int)pos[i];
      pos[i] -= speed[i];

      if (p >= 0 && p < LED_COUNT) {
        leds[p] = CRGB(0, 0, bright[i]);
      } else {
        active[i] = false;
      }
    }
  }

  if (now - lastSpawn > 120 && random(100) < 70) {
    for (int i = 0; i < MAX_DROPS; i++) {
      if (!active[i]) {
        active[i] = true;
        pos[i] = LED_COUNT - 1;    // spawn na vrhu
        speed[i] = random(5, 10) * 0.1f;   // brže
        bright[i] = random(140, 220);
        lastSpawn = now;
        break;
      }
    }
  }

  //FastLED.show();
}