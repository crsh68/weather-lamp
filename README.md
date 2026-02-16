# Weather Lamp 🌦️💡

ESP32-based smart lamp that displays real-time weather conditions through dynamic LED animations. Fully integrated with Apple HomeKit via HomeSpan.

![Weather Lamp](images/weather-lamp-demo.jpg)

## Features

- **Real-time Weather Display**: Automatically fetches weather data from OpenWeatherMap API
- **8 Weather Effects**: 
  - ☀️ Sunny - Warm sunrise/sunset gradient animation
  - ☁️ Cloudy - Gentle grey cloud movements
  - 🌧️ Rain - Blue raindrops falling effect
  - ⛈️ Thunder - Lightning flashes with storm clouds
  - ❄️ Snow - White snowflakes falling
  - 🌫️ Fog - Misty grey gradient waves
  - 💨 Wind - Fast-moving white/cyan streaks
  - 🟠 Amber Alert - Solid amber warning color
- **Apple HomeKit Integration**: Control via Home app (on/off, brightness)
- **Manual Control**: Physical button for power toggle
- **Auto-updates**: Weather refreshes every 10 minutes
- **WiFi Watchdog**: Auto-reconnect if connection drops

## Hardware Requirements

- ESP32 development board (tested on ESP32-WROOM-32)
- WS2812B LED strip (24 LEDs)
- Push button
- Power supply (5V, adequate for LED count)
- Resistors and wiring

### Wiring Diagram

```
ESP32           Component
-----           ---------
GPIO 5    -->   LED Data Pin
GPIO 15   -->   Button (other side to GND, using internal pullup)
5V        -->   LED Power (+)
GND       -->   LED Ground (-)
```

## Software Requirements

### Arduino IDE Setup

1. Install [Arduino IDE](https://www.arduino.cc/en/software) 1.8.13 or later
2. Add ESP32 board support:
   - Go to File > Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"

### Required Libraries

Install via Arduino Library Manager (Sketch > Include Library > Manage Libraries):

- **FastLED** (v3.6.0 or later) - LED control
- **HomeSpan** (v1.7.0 or later) - HomeKit integration
- **ArduinoJson** (v6.21.0 or later) - JSON parsing
- **HTTPClient** (included with ESP32 core) - API requests

## Installation

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/weather-lamp.git
cd weather-lamp
```

### 2. Configure Credentials

Edit `WeatherLamp.ino` and update these lines with your information:

```cpp
// WiFi credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// OpenWeatherMap API
String apiKey = "YOUR_API_KEY";  // Get free API key at https://openweathermap.org/api
String city   = "YOUR_CITY";     // e.g., "London" or "New York"
```

### 3. Upload Code

1. Connect ESP32 to your computer via USB
2. Open `WeatherLamp.ino` in Arduino IDE
3. Select your board: Tools > Board > ESP32 Arduino > ESP32 Dev Module
4. Select correct port: Tools > Port > (your ESP32 port)
5. Click Upload

### 4. Pair with HomeKit

1. Open the Home app on your iPhone/iPad
2. Tap "+" to add accessory
3. Select "Add Accessory"
4. Scan the HomeSpan QR code shown in Serial Monitor
   - Or manually enter the 8-digit setup code (default: `466-37-726`)
5. Follow on-screen instructions to complete pairing

## Usage

### Physical Controls

- **Button Press**: Toggle lamp on/off

### HomeKit Controls

- **Power**: Turn lamp on/off via Home app
- **Brightness**: Adjust LED intensity (0-100%)
- **Automation**: Create scenes and automations

### Weather Updates

- Weather data automatically updates every 10 minutes
- Current weather effect displayed on LEDs
- Check Serial Monitor (115200 baud) for debug info

## Weather API Mapping

| OpenWeatherMap Condition | Display Effect |
|-------------------------|----------------|
| Clear | Sunny ☀️ |
| Clouds | Cloudy ☁️ |
| Rain, Drizzle | Rain 🌧️ |
| Thunderstorm | Thunder ⛈️ |
| Snow | Snow ❄️ |
| Mist, Fog, Haze | Fog 🌫️ |
| Dust, Sand, Ash | Wind 💨 |
| Tornado, Squall | Wind 💨 |
| Smoke | Amber 🟠 |

## Troubleshooting

### WiFi Connection Issues
- Check SSID and password are correct
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check Serial Monitor for connection status

### HomeKit Pairing Problems
- Long-press button during startup to unpair
- Reset HomeKit data: Delete lamp from Home app
- Power cycle ESP32 and try pairing again

### LEDs Not Working
- Verify correct GPIO pin (default: GPIO 5)
- Check LED strip power supply
- Confirm LED_COUNT matches your strip length
- Test with FastLED examples first

### Weather Not Updating
- Verify API key is valid at [OpenWeatherMap](https://openweathermap.org/api)
- Check city name spelling
- Monitor Serial output for API errors
- Free API tier: 60 calls/minute, 1,000,000 calls/month

## Project Structure

```
weather-lamp/
├── WeatherLamp.ino          # Main Arduino sketch
├── effects_sunny.h          # Sunny weather effect
├── effects_clouds.h         # Cloudy weather effect
├── effects_rain.h           # Rain weather effect
├── effects_thunder.h        # Thunderstorm effect
├── effects_snow.h           # Snow weather effect
├── effects_fog.h            # Fog/mist effect
├── effects_wind.h           # Wind/dust effect
├── effects_amber.h          # Amber alert effect
├── README.md                # This file
├── LICENSE                  # MIT License
└── .gitignore              # Git ignore rules
```

## Customization

### Change LED Count
```cpp
#define LED_COUNT   24  // Change to your LED strip length
```

### Adjust Update Interval
```cpp
const unsigned long WEATHER_INTERVAL = 600000;  // 10 min in milliseconds
```

### Modify Effects
Edit individual effect files (e.g., `effects_sunny.h`) to customize animations:
- Color palettes
- Animation speed
- Patterns and transitions

### Change Button Pin
```cpp
#define BUTTON_PIN  15   // Change to your GPIO pin
```

## API Rate Limits

OpenWeatherMap Free Tier:
- 60 calls per minute
- 1,000,000 calls per month
- Updates every 10 minutes = ~4,320 calls/month (well within limit)

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## Credits

- **FastLED** - LED control library
- **HomeSpan** - HomeKit implementation for ESP32
- **OpenWeatherMap** - Weather data API

## Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Check existing issues for solutions
- Serial Monitor (115200 baud) provides debug information

## Roadmap

- [ ] Add more weather effects (hail, sleet, etc.)
- [ ] Web configuration portal (no code editing needed)
- [ ] Multiple city support with switching
- [ ] Weather forecast display (next hour/day)
- [ ] Custom color schemes via HomeKit
- [ ] MQTT integration
- [ ] Power consumption optimization

---

**Made with ❤️ for HomeKit and smart home enthusiasts**
