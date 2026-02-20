#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HomeSpan.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

#define FASTLED_ESP32_RMT_BUILTIN_DRIVER 1
#define FASTLED_FORCE_ESP32_RMT_DRIVER 1
#define FASTLED_ESP32_FLASH_LOCK RDY_US=20

#include <FastLED.h>

// ================= LED =================
#define LED_PIN     1
#define LED_COUNT   24
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[LED_COUNT];

// ================= BUTTON =================
#define BUTTON_PIN  0   // GPIO0
// koristi se interni pullup

// ================= NVS =================
Preferences prefs;

// ================= WIFI =================
// Fallback kredencijali ako NVS nije popunjen
const char* DEFAULT_SSID     = "Zhone_8FDC";
const char* DEFAULT_PASSWORD = "znid306852572";

// Aktivni kredencijali (ucitani iz NVS ili default)
String wifiSSID     = "";
String wifiPassword = "";

// ================= AP PROVISIONING =================
#define AP_SSID     "WeatherLamp-Setup"
#define AP_PASSWORD ""        // otvorena mreza, bez lozinke
#define AP_TIMEOUT  120000    // 2 minute u ms

WebServer apServer(80);
DNSServer dnsServer;
bool apActive         = false;
bool newCredsReceived = false;

// ================= WEATHER =================
const char* DEFAULT_APIKEY = "b05b0c606f4ae4bb0f2c0e29a4b96f87";
const char* DEFAULT_CITY   = "Varazdin";

String apiKey = "";
String city   = "";

String weatherType = "Clear";

// ================= STATE =================
bool powerOn = true;
int brightness = 20;
int weatherMode = 0;
int lastWeatherMode = -1;
bool systemReady = false;
bool renderingEnabled = true;

int lastBrightness = 20;

unsigned long pairingStartTime = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastWiFiCheck = 0;
const unsigned long WEATHER_INTERVAL    = 600000;  // 10 minuta
const unsigned long WIFI_CHECK_INTERVAL =  60000;  // 1 minuta

// ================= THUNDER GLOBALS =================
bool lightningActive = false;
unsigned long lastLightningCheck = 0;
unsigned long lastThunderFrame = 0;

// ================= BUTTON STATE =================
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// pointer na HomeKit karakteristike
SpanCharacteristic *brightnessChar = nullptr;
SpanCharacteristic *powerChar      = nullptr;

// ================= EFFECT HEADERS =================
#include "effects_sunny.h"
#include "effects_clouds.h"
#include "effects_rain.h"
#include "effects_snow.h"
#include "effects_thunder.h"
#include "effects_fog.h"
#include "effects_wind.h"
#include "effects_amber.h"

// ================= NVS: LOAD/SAVE WIFI =================
void loadWiFiCredentials() {
  prefs.begin("wifi", true);  // read-only
  wifiSSID     = prefs.getString("ssid",     DEFAULT_SSID);
  wifiPassword = prefs.getString("password", DEFAULT_PASSWORD);
  apiKey       = prefs.getString("apikey",   DEFAULT_APIKEY);
  city         = prefs.getString("city",     DEFAULT_CITY);
  prefs.end();
  Serial.println("WiFi ucitan iz NVS: SSID = " + wifiSSID);
  Serial.println("OWM grad ucitan iz NVS: " + city);
}

void saveWiFiCredentials(const String& ssid, const String& pass,
                           const String& newApiKey, const String& newCity) {
  prefs.begin("wifi", false);  // read-write
  prefs.putString("ssid",     ssid);
  prefs.putString("password", pass);
  prefs.putString("apikey",   newApiKey);
  prefs.putString("city",     newCity);
  prefs.end();
  Serial.println("Postavke snimljene u NVS.");
  Serial.println("  SSID:  " + ssid);
  Serial.println("  Grad:  " + newCity);
  Serial.println("  APIkey: " + newApiKey.substring(0,8) + "...");
}

// ================= WIFI ANIMACIJE =================
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
  unsigned long s = millis();
  while (millis() - s < 500) {}
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

// Plava pulsirajuca animacija dok je AP provisioning aktivan
void apModeAnim() {
  static unsigned long last = 0;
  static int b = 0;
  static int d = 4;
  if (millis() - last < 20) return;
  last = millis();
  b += d;
  if (b <= 0 || b >= 200) d = -d;
  fill_solid(leds, LED_COUNT, CRGB::Black);
  leds[0] = CRGB(0, 0, b);
  leds[1] = CRGB(0, 0, b / 2);
  FastLED.show();
}

// ================= HOMEKIT PAIRING ANIM =================
void homekitPairingAnim() {
  static unsigned long last = 0;
  static int b = 0;
  static int d = 6;
  if (millis() - last < 30) return;
  last = millis();
  b += d;
  if (b <= 0 || b >= 255) d = -d;
  leds[0] = CRGB(0, 0, b);
  FastLED.show();
}

// ================= AP WEB SERVER - HTML =================

// Forma za unos WiFi podataka
const char AP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="hr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WeatherLamp Setup</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .card {
      background: rgba(255,255,255,0.08);
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.15);
      border-radius: 20px;
      padding: 40px 32px;
      width: 100%;
      max-width: 380px;
      color: #fff;
    }
    .icon { font-size: 48px; text-align: center; margin-bottom: 12px; }
    h1 { text-align: center; font-size: 22px; font-weight: 600; margin-bottom: 6px; }
    .sub { text-align: center; font-size: 14px; color: rgba(255,255,255,0.6); margin-bottom: 28px; }
    label { display: block; font-size: 13px; color: rgba(255,255,255,0.7); margin-bottom: 6px; }
    input {
      width: 100%;
      padding: 12px 16px;
      background: rgba(255,255,255,0.1);
      border: 1px solid rgba(255,255,255,0.2);
      border-radius: 10px;
      color: #fff;
      font-size: 15px;
      margin-bottom: 18px;
      outline: none;
      transition: border-color 0.2s;
    }
    input:focus { border-color: rgba(100,180,255,0.6); }
    input::placeholder { color: rgba(255,255,255,0.3); }
    button {
      width: 100%;
      padding: 14px;
      background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
      border: none;
      border-radius: 10px;
      color: #0f3460;
      font-size: 16px;
      font-weight: 700;
      cursor: pointer;
    }
    .timer {
      text-align: center;
      margin-top: 20px;
      font-size: 13px;
      color: rgba(255,255,255,0.4);
    }
    #cd { color: rgba(255,200,100,0.8); font-weight: 600; }
    .divider { border-top: 1px solid rgba(255,255,255,0.12); margin: 8px 0 20px; }
    .hint { font-size: 12px; color: rgba(255,255,255,0.35); margin-bottom: 18px; line-height: 1.5; }
  </style>
</head>
<body>
  <div class="card">
    <div class="icon">&#127751;</div>
    <h1>WeatherLamp Setup</h1>
    <p class="sub">Unesite WiFi podatke za spajanje na internet</p>
    <form method="POST" action="/save">
      <label>WiFi naziv (SSID)</label>
      <input type="text" name="ssid" placeholder="Naziv mreze" required
             autocomplete="off" autocorrect="off" autocapitalize="none" spellcheck="false">
      <label>Lozinka</label>
      <input type="password" name="password" placeholder="WiFi lozinka" autocomplete="off">
      <div class="divider"></div>
      <label>OpenWeatherMap API kljuc</label>
      <input type="text" name="apikey" placeholder="32-znamenkasti API key"
             autocomplete="off" autocorrect="off" autocapitalize="none" spellcheck="false">
      <label>Grad (engleski naziv)</label>
      <input type="text" name="city" placeholder="npr. Zagreb ili Varazdin"
             autocomplete="off" autocorrect="off" spellcheck="false">
      <p class="hint">API kljuc i grad su opcionalni - ostavite prazno za zadrzavanje trenutnih postavki</p>
      <button type="submit">Spoji i spremi &#10003;</button>
    </form>
    <div class="timer">Automatski povratak za <span id="cd">120</span>s</div>
  </div>
  <script>
    let t = 120;
    const el = document.getElementById('cd');
    setInterval(() => { if (t > 0) { t--; el.textContent = t; } }, 1000);
  </script>
</body>
</html>
)rawliteral";

// Stranica potvrde
const char AP_SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="hr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Snimljeno</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      background: linear-gradient(135deg, #1a1a2e, #0f3460);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #fff;
      text-align: center;
      padding: 20px;
    }
    .card {
      background: rgba(255,255,255,0.08);
      border: 1px solid rgba(255,255,255,0.15);
      border-radius: 20px;
      padding: 40px 32px;
      max-width: 380px;
    }
    .icon { font-size: 56px; margin-bottom: 16px; }
    h1 { font-size: 22px; margin-bottom: 10px; }
    p { color: rgba(255,255,255,0.6); font-size: 14px; line-height: 1.7; }
  </style>
</head>
<body>
  <div class="card">
    <div class="icon">&#10004;</div>
    <h1>Snimljeno!</h1>
    <p>WeatherLamp se restartira i pokusava spojiti<br>na novu WiFi mrezu.<br><br>Mozete zatvoriti ovu stranicu.</p>
  </div>
</body>
</html>
)rawliteral";

// ================= AP WEB SERVER - HANDLERI =================
void handleAPRoot() {
  apServer.send(200, "text/html", AP_HTML);
}

void handleAPSave() {
  if (apServer.hasArg("ssid") && apServer.arg("ssid").length() > 0) {
    String newSSID   = apServer.arg("ssid");
    String newPass   = apServer.arg("password");
    // Opcionalna polja - zadrzaj postojece vrijednosti ako su prazna
    String newApiKey = apServer.arg("apikey");
    String newCity   = apServer.arg("city");
    if (newApiKey.length() == 0) newApiKey = apiKey;
    if (newCity.length()   == 0) newCity   = city;

    apServer.send(200, "text/html", AP_SAVED_HTML);

    Serial.println("\n>>> Nove postavke primljene:");
    Serial.println("    SSID: " + newSSID);
    Serial.println("    Grad: " + newCity);

    saveWiFiCredentials(newSSID, newPass, newApiKey, newCity);
    wifiSSID     = newSSID;
    wifiPassword = newPass;
    apiKey       = newApiKey;
    city         = newCity;

    newCredsReceived = true;
  } else {
    apServer.send(400, "text/plain", "SSID ne moze biti prazan.");
  }
}

// Captive portal: preusmjeri sve nepoznate zahtjeve na formu
void handleAPNotFound() {
  apServer.sendHeader("Location", "http://192.168.4.1/", true);
  apServer.send(302, "text/plain", "");
}

// ================= AP START / STOP =================
void startProvisioningAP() {
  Serial.println("\n>>> Pokrecanje WiFi provisioning AP...");
  Serial.println("    SSID: " AP_SSID);
  Serial.println("    IP:   192.168.4.1");
  Serial.println("    Timeout: 2 minute");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);  // otvorena mreza
  delay(200);

  // DNS captive portal - sve domene -> 192.168.4.1
  dnsServer.start(53, "*", WiFi.softAPIP());

  apServer.on("/",     HTTP_GET,  handleAPRoot);
  apServer.on("/save", HTTP_POST, handleAPSave);
  apServer.onNotFound(handleAPNotFound);
  apServer.begin();

  apActive = true;
  Serial.println("    AP aktivan. Spoji mobitel na 'WeatherLamp-Setup'");
  Serial.println("    i otvori browser -> 192.168.4.1");
}

void stopProvisioningAP() {
  apServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  apActive = false;
  Serial.println(">>> AP provisioning ugasen.");
}

// ================= WIFI CONNECT S AP FALLBACK =================
//
// Tijek:
//  1. Ucita kredencijale iz NVS
//  2. Pokusa se spojiti (3 puta po 30s, sa 10s pauzom)
//  3. Nakon 3 neuspjeha -> pokrene AP provisioning (2 minute)
//      a) Ako korisnik unese nove podatke -> spremi + restart
//      b) Ako istekne timeout -> ugasi AP, ponovi od koraka 2
//
void connectWiFiWithProvisioningFallback() {
  const unsigned long ATTEMPT_MS = 30000;  // 30s po pokusaju
  const unsigned long PAUSE_MS   = 10000;  // 10s pauza izmedju pokusaja
  const int MAX_ATTEMPTS         = 3;      // pokusaja prije AP moda

  int attempts = 0;

  while (true) {
    attempts++;
    Serial.printf("\n>>> WiFi pokusaj #%d - SSID: %s\n", attempts, wifiSSID.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    unsigned long tryStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - tryStart < ATTEMPT_MS) {
      wifiConnectingAnim();
    }

    if (WiFi.status() == WL_CONNECTED) {
      // Uspjeh
      Serial.println(">>> WiFi spojen! IP: " + WiFi.localIP().toString());
      wifiConnectedAnim();
      return;
    }

    Serial.println(">>> Spajanje neuspjesno.");
    WiFi.disconnect(true);
    delay(200);

    if (attempts >= MAX_ATTEMPTS) {
      // --- POKRENI AP PROVISIONING ---
      Serial.println(">>> " + String(MAX_ATTEMPTS) + " neuspjesna pokusaja - startam AP...");

      startProvisioningAP();
      newCredsReceived = false;

      unsigned long apStart = millis();

      while (millis() - apStart < AP_TIMEOUT) {
        dnsServer.processNextRequest();
        apServer.handleClient();
        apModeAnim();

        if (newCredsReceived) {
          Serial.println(">>> Novi kredencijali -> restart...");
          stopProvisioningAP();
          delay(1000);
          ESP.restart();  // restart sa novim postavkama
        }
      }

      // Timeout bez unosa - nastavi s postojecim kredencijalima
      Serial.println(">>> AP timeout (2 min) bez unosa - nastavljam s postojecim...");
      stopProvisioningAP();
      delay(200);

      // Reset brojaca, ponovi ciklus
      attempts = 0;
    }

    // Pauza prije sljedeceg pokusaja
    Serial.printf(">>> Pauza %lus...\n", PAUSE_MS / 1000);
    unsigned long pauseStart = millis();
    while (millis() - pauseStart < PAUSE_MS) {
      wifiErrorAnim();
    }
  }
}

// ================= WEATHER FETCH =================
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n>>> fetchWeather: WiFi nije spojen - reconnect...");
    WiFi.disconnect();
    delay(100);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    unsigned long rs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - rs < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n>>> Reconnect OK: " + WiFi.localIP().toString());
    } else {
      Serial.println("\n>>> Reconnect neuspjesan - preskacam dohvat");
      return;
    }
  }

  HTTPClient http;
  String url = "https://api.openweathermap.org/data/2.5/weather?q=" +
               city + "&appid=" + apiKey + "&units=metric";
  Serial.println("Dohvacam vrijeme...");
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    deserializeJson(doc, payload);
    weatherType = doc["weather"][0]["main"].as<String>();
    String description = doc["weather"][0]["description"].as<String>();
    float temp    = doc["main"]["temp"].as<float>();
    int   humidity = doc["main"]["humidity"].as<int>();
    Serial.println("=================================");
    Serial.print("Vrijeme: ");     Serial.println(weatherType);
    Serial.print("Opis: ");        Serial.println(description);
    Serial.print("Temperatura: "); Serial.print(temp); Serial.println(" C");
    Serial.print("Vlaznost: ");    Serial.print(humidity); Serial.println(" %");
    Serial.println("=================================");
  } else {
    Serial.print("HTTP greska: ");
    Serial.println(httpCode);
  }
  http.end();
}

// ================= BUTTON HANDLER =================
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        if (brightness > 0) {
          if (brightness >= 5) lastBrightness = brightness;
          brightness = 0;
          renderingEnabled = false;
          if (brightnessChar) brightnessChar->setVal(0, true);
          Serial.println("Button: Light OFF");
          fill_solid(leds, LED_COUNT, CRGB::Black);
          FastLED.show();
        } else {
          brightness = lastBrightness;
          if (brightness < 5) brightness = 20;
          renderingEnabled = true;
          FastLED.setBrightness(map(brightness, 0, 100, 0, 255));
          if (brightnessChar) brightnessChar->setVal(brightness, true);
          Serial.print("Button: Light ON - ");
          Serial.print(brightness);
          Serial.println("%");
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
    if (newBrightness > 0 && newBrightness != brightness) lastBrightness = newBrightness;
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
      FastLED.show();
      renderingEnabled = false;
    } else {
      renderingEnabled = true;
    }
    return true;
  }

  void loop() override {
    static int lastReportedBrightness = -1;
    if (brightness > 0 && brightness < 5 && !renderingEnabled) brightness = 0;
    if (brightness != lastReportedBrightness) {
      level->setVal(brightness, true);
      lastReportedBrightness = brightness;
      Serial.print("HomeKit: Brightness = ");
      Serial.print(brightness);
      Serial.println("%");
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
    int expected = map(weatherMode, 0, 9, 0, 100);
    if (modeLevel->getVal() != expected) modeLevel->setVal(expected, true);
  }
};

WeatherModeService *weatherModeService = nullptr;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT);
  FastLED.setBrightness(map(brightness, 0, 100, 0, 255));
  FastLED.clear(true);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Ucitaj WiFi kredencijale iz NVS (ili default ako prvi start)
  loadWiFiCredentials();

  Serial.println("\n========================================");
  Serial.println("   Varazdin Weather Lamp - Pokretanje");
  Serial.println("========================================");
  Serial.println("WiFi SSID: " + wifiSSID);
  Serial.println("Za promjenu WiFi: iskljuci struju, cekaj");
  Serial.println("da se pokrene AP 'WeatherLamp-Setup'");
  Serial.println("========================================\n");

  // Spajanje na WiFi s AP provisioning fallback-om
  connectWiFiWithProvisioningFallback();

  fetchWeather();
  lastWeatherUpdate = millis();

  pairingStartTime = millis();

  homeSpan.setPairingCode("55566777");
  homeSpan.begin(Category::Lighting, "Weather Lamp");

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify(true);

  new WeatherLight();
  weatherModeService = new WeatherModeService();

  Serial.println("\nHomeKit pairing code: 555-66-777");
  Serial.println("Sustav spreman!\n");
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
      if (weatherModeService && weatherModeService->modeLevel) {
        weatherModeService->modeLevel->setVal(0);
        Serial.println(">>> Weather mode slider reset na AUTO (0%)");
      }
    }
  }

  if (!powerOn || !renderingEnabled) return;

  // WiFi watchdog - provjeri svake minute
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n>>> WiFi Watchdog: konekcija izgubljena - reconnect...");
      WiFi.disconnect();
      delay(100);
      WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
      unsigned long rs = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - rs < 5000) {
        delay(250);
        Serial.print(".");
      }
      Serial.println(WiFi.status() == WL_CONNECTED
                     ? "\n>>> Watchdog reconnect OK!"
                     : "\n>>> Watchdog reconnect neuspjesan");
    }
  }

  // Vremenski efekti
  if (weatherMode == 0) {
    if (millis() - lastWeatherUpdate > WEATHER_INTERVAL) {
      fetchWeather();
      lastWeatherUpdate = millis();
    }
    if      (weatherType.indexOf("Clear")   >= 0) sunny();
    else if (weatherType.indexOf("Cloud")   >= 0) clouds();
    else if (weatherType.indexOf("Drizzle") >= 0) drizzle();
    else if (weatherType.indexOf("Rain")    >= 0) rain();
    else if (weatherType.indexOf("Thunder") >= 0) thunderstorm();
    else if (weatherType.indexOf("Snow")    >= 0) snow();
    else if (weatherType.indexOf("Fog")     >= 0 ||
             weatherType.indexOf("Mist")    >= 0) fog();
    else                                           wind();
  } else {
    if (weatherMode != lastWeatherMode) {
      Serial.print("\nRucni mode: ");
      switch (weatherMode) {
        case 1: Serial.println("Sunny");        break;
        case 2: Serial.println("Clouds");       break;
        case 3: Serial.println("Drizzle");      break;
        case 4: Serial.println("Rain");         break;
        case 5: Serial.println("Thunderstorm"); break;
        case 6: Serial.println("Snow");         break;
        case 7: Serial.println("Fog");          break;
        case 8: Serial.println("Wind");         break;
        case 9: Serial.println("Amber");        break;
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
