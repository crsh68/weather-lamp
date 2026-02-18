#ifndef EFFECTS_SUNNY_H
#define EFFECTS_SUNNY_H

// ===== SUNNY EFFECT =====
// Gornje 4 LEDs = žuto sunce (pulsiranje 2s period)
// Donje 20 LEDs = plavo nebo sa bijelim oblacima (max 2 oblaka)

// Oblaci state
struct sunnyCloud {
  int center;              // centar oblaka (LED pozicija)
  float phase;             // faza rasta/pada oblaka
  uint8_t maxBrightness;   // maksimalni intenzitet ovog oblaka
  uint8_t maxSize;         // maksimalna veličina oblaka (4-10 LEDs)
  float growSpeed;         // brzina rasta
  float fadeSpeed;         // brzina nestajanja
  bool active;             // je li oblak aktivan
  bool growing;            // raste ili pada
};

void sunny() {
  // Sunce state
  static float sunPulse = 0.0f;
  const float SUN_SPEED = 0.05f;  // brzina pulsiranja (2s period ≈ 0.05)
  
  static sunnyCloud whiteclouds[2];  // maksimalno 2 oblaka
  static unsigned long lastCloudSpawn = 0;
  static unsigned long lastSunnyFrame = 0;
  
  unsigned long now = millis();
  if (now - lastSunnyFrame < 35) return;
  lastSunnyFrame = now;

  // ===== SUNCE (LEDs 20-23) =====
  // Pulsiranje sa sin valom, period ~2 sekunde
  sunPulse += SUN_SPEED;
  float sunIntensity = sin(sunPulse) * 0.3f + 0.7f;  // 0.4 - 1.0 range
  
  uint8_t sunBrightness = 255 * sunIntensity;
  CRGB sunColor = CRGB(sunBrightness, sunBrightness * 0.8f, 0);  // žuto
  
  leds[23] = sunColor;
  leds[22] = sunColor;
  leds[21] = sunColor;
  leds[20] = sunColor;

  // ===== NEBO (LEDs 0-19) - plava pozadina =====
  for (int i = 0; i < 20; i++) {
    leds[i] = CRGB(30, 100, 180);  // plavo nebo
  }

  // ===== OBLACI =====
  // Update postojećih oblaka
  for (int c = 0; c < 2; c++) {
    if (whiteclouds[c].active) {
      
      // Ažuriraj fazu oblaka
      if (whiteclouds[c].growing) {
        whiteclouds[c].phase += whiteclouds[c].growSpeed;  // varijabilna brzina rasta
        if (whiteclouds[c].phase >= 1.0f) {
          whiteclouds[c].phase = 1.0f;
          whiteclouds[c].growing = false;  // počni padati
        }
      } else {
        whiteclouds[c].phase -= whiteclouds[c].fadeSpeed;  // varijabilna brzina nestajanja
        if (whiteclouds[c].phase <= 0.0f) {
          whiteclouds[c].active = false;  // oblak nestao
          continue;
        }
      }

      // Crtaj oblak - simetrično širenje od centra s varijabilnom veličinom
      int center = whiteclouds[c].center;
      float phase = whiteclouds[c].phase;
      uint8_t maxBright = whiteclouds[c].maxBrightness;
      uint8_t maxSize = whiteclouds[c].maxSize;

      // Koliko LEDs trenutno treba renderirati (0 -> maxSize)
      float currentSpread = phase * (maxSize / 2.0f);  // širenje na svaku stranu od centra
      
      // Crtaj LEDs simetrično oko centra
      for (int offset = 0; offset <= (maxSize / 2); offset++) {
        if (offset > currentSpread) break;  // ne crtaj van trenutnog širenja
        
        // Računaj intenzitet - centar najjači, rubovi slabiji
        float distFactor = 1.0f - (offset / (float)(maxSize / 2));
        uint8_t brightness = maxBright * phase * distFactor;
        
        // Centralni LED
        if (offset == 0 && center >= 0 && center < 20) {
          leds[center] = blend(leds[center], CRGB::White, brightness);
        }
        // LEDs lijevo i desno
        else {
          if (center - offset >= 0 && center - offset < 20) {
            leds[center - offset] = blend(leds[center - offset], CRGB::White, brightness);
          }
          if (center + offset >= 0 && center + offset < 20) {
            leds[center + offset] = blend(leds[center + offset], CRGB::White, brightness);
          }
        }
      }
    }
  }

  // ===== SPAWN NOVOG OBLAKA =====
  // Povremeno spawna novi oblak ako ima mjesta
  if (now - lastCloudSpawn > 2000 && random(100) < 30) {  // ~30% šansa svakih 2s
    
    // Nađi slobodan slot
    for (int c = 0; c < 2; c++) {
      if (!whiteclouds[c].active) {
        whiteclouds[c].active = true;
        whiteclouds[c].center = random(5, 15);  // sigurnija zona za velike oblake
        whiteclouds[c].phase = 0.0f;
        
        // Random veličina oblaka (4-10 LEDs)
        whiteclouds[c].maxSize = random(4, 11);  // 4-10 uključivo
        
        // Random intenzitet (100-220)
        whiteclouds[c].maxBrightness = random(100, 221);
        
        // Random brzina rasta (brži ili sporiji oblaci)
        whiteclouds[c].growSpeed = random(40, 100) / 1000.0f;  // 0.04 - 0.10
        
        // Random brzina nestajanja (obično sporije od rasta)
        whiteclouds[c].fadeSpeed = random(20, 60) / 1000.0f;   // 0.02 - 0.06
        
        whiteclouds[c].growing = true;
        lastCloudSpawn = now;
        break;  // samo jedan oblak po spawn-u
      }
    }
  }
}

#endif  // EFFECTS_SUNNY_H