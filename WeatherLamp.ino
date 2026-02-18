#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HomeSpan.h>

#define FASTLED_ESP32_RMT_BUILTIN_DRIVER 1
#define FASTLED_FORCE_ESP32_RMT_DRIVER 1
#define FASTLED_ESP32_FLASH_LOCK RDY_US=20

#include <FastLED.h>

// ================= LED =================
#define LED_PIN     5
#define LED_COUNT   24
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[LED_COUNT];

// ================= BUTTON =================
#define BUTTON_PIN  15   // GPIO15
// koristi se interni pullup

// ================= WIFI & WEATHER =================
// TODO: Replace with your credentials before uploading
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Get your free API key at: https://openweathermap.org/api
String apiKey = "YOUR_OPENWEATHERMAP_API_KEY";
String city   = "YOUR_CITY";  // e.g., "London", "New York", "Tokyo"

String weatherType = "Clear";

// ================= STATE =================
bool powerOn = true;
int brightness = 20;
int weatherMode = 0;
int lastWeatherMode = -1;  // Track mode changes
bool systemReady = false;
bool renderingEnabled = true;

int lastBrightness = 20;

unsigned long pairingStartTime = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastWiFiCheck = 0;  // WiFi watchdog timer
const unsigned long WEATHER_INTERVAL = 600000;  // 10 minutes
const unsigned long WIFI_CHECK_INTERVAL = 60000;  // 1 minute

// ================= THUNDER GLOBALS =================
bool lightningActive = false;
unsigned long lastLightningCheck = 0;
unsigned long lastThunderFrame = 0;

// ================= BUTTON STATE =================
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// pointer na HomeKit brightness karakteristiku
SpanCharacteristic *brightnessChar = nullptr;
SpanCharacteristic *powerChar = nullptr;

// ================= EFFECT HEADERS =================
#include "effects_sunny.h"
#include "effects_clouds.h"
#include "effects_rain.h"
#include "effects_snow.h"
#include "effects_thunder.h"
#include "effects_fog.h"
#include "effects_wind.h"
#include "effects_amber.h"

// ================= WIFI ANIMS =================
void wifiConnectingAnim() {
  static unsigned long last = 0;
  static bool state = false;

  if (millis() - last < 400) return;
  last = millis();

  fill_solid(leds, LED_COUNT, CRGB::Black);
  leds[0] = state ? CRGB::Red : CRGB::Black;
  FastLED.show();
  state = !state;
}

void wifiConnectedAnim() {
  fill_solid(leds, LED_COUNT, CRGB::Black);
  leds[0] = CRGB::Green;
  FastLED.show();
  
  unsigned long start = millis();
  while (millis() - start < 500) {
    // Non-blocking wait
  }
  
  fill_solid(leds, LED_COUNT, CRGB::Black);
  FastLED.show();
}

void wifiErrorAnim() {
  static unsigned long last = 0;
  static bool state = false;

  if (millis() - last < 500) return;
  last = millis();

  fill_solid(leds, LED_COUNT, CRGB::Black);
  leds[0] = state ? CRGB(255, 80, 0) : CRGB::Black;
  FastLED.show();
  state = !state;
}

// ================= HOMEKIT PAIRING =================
void homekitPairingAnim() {
  static unsigned long last = 0;
  static int b = 0;
  static int d = 6;

  if (millis() - last < 30) return;
  last = millis();

  b += d;
  if (b <= 0 || b >= 255) d = -d;

  leds[0] = CRGB(0, 0, b);
  FastLED.show();  // Potrebno jer se poziva prije systemReady
}

// ================= WEATHER FETCH =================
void fetchWeather() {

  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n!!! WiFi disconnected - attempting reconnect...");
    
    // Try to reconnect
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid, password);
    
    unsigned long reconnectStart = millis();
    int attempt = 0;
    
    while (WiFi.status() != WL_CONNECTED && millis() - reconnectStart < 10000) {
      delay(500);
      Serial.print(".");
      attempt++;
      
      if (attempt % 10 == 0) {
        Serial.println();
        Serial.print("Reconnect attempt ");
        Serial.print(attempt / 10);
        Serial.print("...");
      }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✓ WiFi reconnected successfully!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n✗ WiFi reconnect failed - will retry in 10 minutes");
      return;
    }
  } else {
    Serial.println("\n✓ WiFi connected - IP: " + WiFi.localIP().toString());
  }

  HTTPClient http;
  String url =
    "https://api.openweathermap.org/data/2.5/weather?q=" +
    city + "&appid=" + apiKey + "&units=metric";

  Serial.println("Fetching weather data...");
  http.begin(url);

  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    deserializeJson(doc, payload);
    
    weatherType = doc["weather"][0]["main"].as<String>();
    String description = doc["weather"][0]["description"].as<String>();
    float temp = doc["main"]["temp"].as<float>();
    int humidity = doc["main"]["humidity"].as<int>();
    
    Serial.println("=================================");
    Serial.print("Weather: ");
    Serial.println(weatherType);
    Serial.print("Description: ");
    Serial.println(description);
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.println("=================================");
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }

  http.end();
}

// ================= BUTTON HANDLER =================
void handleButton() {

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {

        if (brightness > 0) {

          // Save current brightness before turning off
          if (brightness >= 5) {
            lastBrightness = brightness;
          }
          
          brightness = 0;
          renderingEnabled = false;

          // Update HomeKit slider to 0%
          if (brightnessChar) {
            brightnessChar->setVal(0, true);
          }

          Serial.println("Button: Light OFF - Brightness = 0%, Slider = 0%");

          fill_solid(leds, LED_COUNT, CRGB::Black);
          FastLED.show();
        }
        else {

          brightness = lastBrightness;
          
          // If brightness is too low (HomeKit set it to 1-4%), use default 20%
          if (brightness < 5) {
            brightness = 20;
            Serial.println("Brightness was <5%, resetting to 20%");
          }
          
          renderingEnabled = true;

          FastLED.setBrightness(map(brightness, 0, 100, 0, 255));

          // Update HomeKit slider to new brightness
          if (brightnessChar) {
            brightnessChar->setVal(brightness, true);
          }

          Serial.print("Button: Light ON - Brightness = ");
          Serial.print(brightness);
          Serial.println("%, Slider updated");
        }
      }
    }
  }

  lastButtonState = reading;
}

// ================= HOMEKIT =================
struct WeatherLight : Service::LightBulb {

  SpanCharacteristic *power;
  SpanCharacteristic *level;

  WeatherLight() {

    power = new Characteristic::On(true);
    level = new Characteristic::Brightness(20);

    brightnessChar = level;
    powerChar = power;
  }

  boolean update() override {

    powerOn = power->getNewVal();
    int newBrightness = level->getNewVal();

    // Save last non-zero brightness before updating
    if (newBrightness > 0 && newBrightness != brightness) {
      lastBrightness = newBrightness;
    }
    
    brightness = newBrightness;

    FastLED.setBrightness(map(brightness, 0, 100, 0, 255));

    if (!powerOn) {
      fill_solid(leds, LED_COUNT, CRGB::Black);
      FastLED.show();
      renderingEnabled = false;
      return true;
    }

    if (brightness == 0) {
      fill_solid(leds, LED_COUNT, CRGB::Black);
      FastLED.show();  // Odmah gasi LEDs
      renderingEnabled = false;
    } else {
      renderingEnabled = true;
    }

    return true;
  }
  
  void loop() override {
    // Actively sync slider with actual brightness value
    static int lastReportedBrightness = -1;
    
    // Force brightness to 0 if iOS set it to low value (1-4%)
    if (brightness > 0 && brightness < 5 && !renderingEnabled) {
      brightness = 0;
      Serial.println("Forcing brightness to 0% (was <5%)");
    }
    
    if (brightness != lastReportedBrightness) {
      level->setVal(brightness, true);  // true = force event notification
      lastReportedBrightness = brightness;
      
      Serial.print("HomeKit: Brightness slider updated to ");
      Serial.print(brightness);
      Serial.println("% (with notification)");
    }
  }
};

struct WeatherModeService : Service::LightBulb {

  SpanCharacteristic *modeLevel;

  WeatherModeService() {
    new Characteristic::On(true);
    modeLevel = new Characteristic::Brightness(0);
  }

  boolean update() override {
    weatherMode = map(modeLevel->getNewVal(), 0, 100, 0, 9);
    return true;
  }
  
  void loop() override {
    // Sync HomeKit slider with actual weatherMode
    int expectedSliderValue = map(weatherMode, 0, 9, 0, 100);
    if (modeLevel->getVal() != expectedSliderValue) {
      modeLevel->setVal(expectedSliderValue, true);  // true = force notification
    }
  }
};

// Pointer to WeatherModeService for mode slider control
WeatherModeService *weatherModeService = nullptr;

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT);
  
  FastLED.setBrightness(map(brightness, 0, 100, 0, 255));
  FastLED.clear(true);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    wifiConnectingAnim();
    if (millis() - start > 30000) {
      while (true) wifiErrorAnim();
    }
  }

  wifiConnectedAnim();

  Serial.println("\n========================================");
  Serial.println("   Varaždin Weather Lamp - Starting");
  Serial.println("========================================");
  
  fetchWeather();
  lastWeatherUpdate = millis();

  pairingStartTime = millis();

  homeSpan.setPairingCode("55566777");
  homeSpan.begin(Category::Lighting, "Varaždin Weather");

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify(true);

  new WeatherLight();
  weatherModeService = new WeatherModeService();
  
  Serial.println("\nHomeKit pairing code: 555-66-777");
  Serial.println("System ready!\n");
  Serial.println("Weather mode: AUTO (0)\n");
}

// ================= LOOP =================
void loop() {

  homeSpan.poll();
  handleButton();

  if (!systemReady) {
    if (millis() - pairingStartTime < 45000) {
      homekitPairingAnim();
    } else {
      systemReady = true;
      
      // Reset weather mode slider to 0% after startup
      if (weatherModeService && weatherModeService->modeLevel) {
        weatherModeService->modeLevel->setVal(0);
        Serial.println("✓ Weather mode slider reset to AUTO (0%)");
      }
    }
  }

  if (!powerOn || !renderingEnabled) {
    return;
  }

  // WiFi watchdog - check connection every minute
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n⚠ WiFi Watchdog: Connection lost - reconnecting...");
      
      WiFi.disconnect();
      delay(100);
      WiFi.begin(ssid, password);
      
      unsigned long reconnectStart = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - reconnectStart < 5000) {
        delay(250);
        Serial.print(".");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi Watchdog: Reconnected!");
      } else {
        Serial.println("\n✗ WiFi Watchdog: Reconnect failed");
      }
    }
  }

  if (weatherMode == 0) {

    if (millis() - lastWeatherUpdate > WEATHER_INTERVAL) {
      fetchWeather();
      lastWeatherUpdate = millis();
    }

    if (weatherType.indexOf("Clear") >= 0)        sunny();
    else if (weatherType.indexOf("Cloud") >= 0)   clouds();
    else if (weatherType.indexOf("Drizzle") >= 0) drizzle();
    else if (weatherType.indexOf("Rain") >= 0)    rain();
    else if (weatherType.indexOf("Thunder") >= 0) thunderstorm();
    else if (weatherType.indexOf("Snow") >= 0)    snow();
    else if (weatherType.indexOf("Fog") >= 0 ||
             weatherType.indexOf("Mist") >= 0)    fog();
    else                                          wind();
  }
  else {
    // Display mode change
    if (weatherMode != lastWeatherMode) {
      Serial.print("\nManual mode selected: ");
      switch (weatherMode) {
        case 1: Serial.println("Sunny"); break;
        case 2: Serial.println("Clouds"); break;
        case 3: Serial.println("Drizzle"); break;
        case 4: Serial.println("Rain"); break;
        case 5: Serial.println("Thunderstorm"); break;
        case 6: Serial.println("Snow"); break;
        case 7: Serial.println("Fog"); break;
        case 8: Serial.println("Wind"); break;
        case 9: Serial.println("Amber"); break;
      }
      lastWeatherMode = weatherMode;
    }
    
    switch (weatherMode) {
      case 1: sunny();        break;
      case 2: clouds();       break;
      case 3: drizzle();      break;
      case 4: rain();         break;
      case 5: thunderstorm(); break;
      case 6: snow();         break;
      case 7: fog();          break;
      case 8: wind();         break;
      case 9: amberEffect();  break;
    }
  }
  
  FastLED.show();
  
}
