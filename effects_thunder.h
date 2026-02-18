#pragma once

// koristi globalne varijable iz WeatherLamp.ino
extern bool lightningActive;
extern unsigned long lastLightningCheck;
extern unsigned long lastThunderFrame;

extern CRGB leds[LED_COUNT];

// lokalni state za munje
static int lightningPhase = 0;
static int flashCount = 0;
static int maxFlashes = 0;
static unsigned long lastFlashTime = 0;

// forward deklaracije
void startLightning();
void updateLightning();

// GLAVNI THUNDER EFEKT
void thunderstorm() {
  unsigned long now = millis();

  // kiša stalno ide u pozadini
  rain();

  // povremeno pokreni munju
  if (!lightningActive && now - lastLightningCheck > 1500 + random(2500)) {
    if (random(100) < 60) {
      startLightning();
    }
    lastLightningCheck = now;
  }

  updateLightning();
}

// POKRETANJE MUNJE
void startLightning() {
  lightningActive = true;
  lightningPhase = 0;
  flashCount = 0;
  maxFlashes = random(2, 5);   // broj bljeskova
  lastFlashTime = millis();
}

// UPDATE MUNJE – NON BLOCKING
void updateLightning() {
  if (!lightningActive) return;

  unsigned long now = millis();

  // timing između faza munje
  if (now - lastFlashTime < 40) return;
  lastFlashTime = now;

  if (lightningPhase == 0) {
    // BLJESAK
    fill_solid(leds, LED_COUNT, CRGB::White);
    //FastLED.show();
    lightningPhase = 1;
  }
  else {
    // POVRATAK U TAMU
    fill_solid(leds, LED_COUNT, CRGB(5, 5, 20));
    FastLED.show();
    lightningPhase = 0;
    flashCount++;

    if (flashCount >= maxFlashes) {
      lightningActive = false;
    }
  }
}