# LightSaber Lamp

## Component

- [ESP32-DevKitC V4] <https://www.amazon.fr/dp/B0BY7VSG4M>
- [WS2811 LED strip] <https://www.amazon.fr/gp/product/B01CNL6LBK>
- [ESP32 devboard] <https://www.amazon.fr/gp/product/B0BCWBW4SR>

### Arduino Pinout

![ESP32-DevKitC v4 board pinout reference](doc/ESP32-DEV-KIT-DevKitC-v4-pinout.jpg)

| **Description** | **GPIO number** | **ESP32 38s pin number** | **ESP32 38s pin name** |
|--- | :-: | :-: | :-: |
| Vader speaker ouput | 3 | 40 | RX |
| Luke speaker ouput | 21 | 42 | P21 |
| Vader strip output | 12 | 18 | P12 |
| Luke strip output | 13 | 20 | P13 |

## Install

Devlop with VSCode and platformIO

Once install, go to the app directory and run ```npm install```

### Arduino Library needed

- Preferences
- FastLED
- LittleFS
- WiFi
- AsyncElegantOTA
- ArduinoJson v7
- PubSubClient (MQTT)
- InputDebounce

### Build

First you need to build the app. To do so, from the app directory run command
```npm run build```

Once this is done, you can upload data partition to the ESP, build the firmware and uplaod it.

## Features

### Control Methods

The LightSaber Lamp supports three independent control interfaces that stay synchronized:

1. **Physical Buttons** - Direct control via GPIO switches
2. **Web Interface** - Real-time control via WebSocket (http://device-ip)
3. **Home Assistant** - Remote automation via MQTT with auto-discovery

### Control Parameters (per saber)

For each sabre (Luke and Vader), you can control:
- **On/Off** - Power the LED strip on or off
- **Brightness** - 0-255 intensity level
- **Color** - Full RGB color control (0-255 per channel)

### MQTT & Home Assistant Integration

The device automatically publishes itself to Home Assistant using MQTT Discovery. Once configured with your MQTT broker, both sabers appear as controllable light entities in Home Assistant.

**Configuration:**
1. Access the web UI (http://device-ip)
2. Navigate to Settings → MQTT
3. Enter your MQTT broker address and port
4. Save (device will restart)
5. Both lights should appear in Home Assistant under "Discovered Devices"

**MQTT Topics:**
- Luke state: `homeassistant/light/light_saber/luke_saber/state`
- Vader state: `homeassistant/light/light_saber/vader_saber/state`
- Commands: `homeassistant/light/light_saber/{luke,vader}_saber/set`

## Event Cycle & Synchronization

The device maintains state synchronization across all three control interfaces using an event-driven architecture:

```
┌─────────────────────────────────────────────────────────────────┐
│                      LED State Change Event                     │
│                                                                  │
│  Triggered by: Button Press │ Web UI │ HomeAssistant Command   │
└──────────────────────────────────────┬──────────────────────────┘
                                       │
                    ┌──────────────────▼──────────────────┐
                    │  Update Ledstrip Object             │
                    │  - state, brightness, color         │
                    └──────────────────┬──────────────────┘
                                       │
          ┌────────────────────────────┼────────────────────────────┐
          │                            │                            │
          ▼                            ▼                            ▼
    ┌──────────────┐           ┌──────────────┐           ┌──────────────┐
    │ LED Animation│           │ WebSocket    │           │ MQTT Publish │
    │              │           │ Broadcast    │           │              │
    │ powerUp()    │           │              │           │ Publish to   │
    │ powerDown()  │           │ All web UI   │           │ homeassistant│
    │ powerLoop()  │           │ clients      │           │ /*/state     │
    │              │           │ receive      │           │              │
    └──────────────┘           │ status JSON  │           └──────────────┘
                               │              │
                               └──────────────┘
                                       │
                                       ▼
                    ┌──────────────────────────────────┐
                    │ All 3 interfaces show same state │
                    │ Synchronization complete        │
                    └──────────────────────────────────┘
```

### Event Priority

When multiple control sources try to change state simultaneously:
1. **Physical buttons** have highest priority (hardware directly changes Ledstrip)
2. **Web UI** and **HomeAssistant** changes treated equally
3. Last-write-wins if conflicts occur (rare with human-speed inputs)

### Offline Behavior

If MQTT broker becomes unavailable:
- Device continues to operate normally (buttons & web UI work)
- No error states or lockups
- Automatic reconnection attempts every 5 seconds
- Home Assistant shows device as offline until reconnection

### State Persistence

- LED color and brightness settings are saved to flash storage
- Settings survive power cycles
- Button and web UI changes are persistent
- MQTT configuration is persistent (requires device restart to apply)

## Arduino development ressources

- <https://www.studiopieters.nl/esp32-pinout/>
- <https://github.com/BenLand100/smartplug/blob/master/smartplug.ino>
- <https://ben.land/post/2023/01/12/diy-smart-plug/>
- <https://randomnerdtutorials.com/esp32-http-get-post-arduino/>
- <https://randomnerdtutorials.com/esp32-websocket-server-arduino/>
- <https://randomnerdtutorials.com/esp32-web-server-spiffs-spi-flash-file-system/>
- <https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/>

## App development ressources

- <https://tailwindcss.com>
- <https://tailwind-elements.com>
- <https://heroicons.com>
- <https://tailwindcomponents.com>
- <https://alpinejs.dev>
- <https://github.com/markmead/alpinejs-form-data>

## License

ICON lightsaber by Nhor from [https://thenounproject.com/browse/icons/term/lightsaber/](Noun Project) (CC BY 3.0)