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

## Version Management

The LightSaber Lamp tracks both firmware and web app versions to help you manage updates.

### Checking Installed Versions

**Via Web UI:**
1. Open the device web interface (http://device-ip)
2. Scroll down to the "Version Information" section
3. View both Firmware and Web App versions with build dates

**Via REST API:**
```bash
# Get firmware version
curl http://device-ip/api/version
# Response:
# {
#   "firmware": "1.2.0",
#   "build_date": "Nov 10 2025 14:33:45"
# }

# Get app version
curl http://device-ip/version.json
# Response:
# {
#   "app": "1.0.0",
#   "buildDate": "2025-11-10T...",
#   "buildTime": "10/11/2025, 14:33:45"
# }
```

### Updating Firmware and App

#### Automatic OTA Update (Recommended)

Use the provided `UPDATE_OTA.sh` script for automatic OTA updates:

```bash
# Configure the device IP (edit UPDATE_OTA.sh)
nano UPDATE_OTA.sh  # Change DEVICE_IP if needed

# Run the update
./UPDATE_OTA.sh
```

The script will:
1. Build the latest web app
2. Package it into the firmware LittleFS filesystem
3. Upload firmware via OTA
4. Wait for device restart
5. Display the new versions after update

#### Manual OTA Update

If you prefer to update manually or the script doesn't work, follow these steps:

**Prerequisites:**
- Device must be powered on and connected to WiFi
- Device IP address (find it in your router's admin panel)
- The device is accessible on the network at `http://<device-ip>`

**Step 1: Build the firmware with updated app**

```bash
# Navigate to the project root
cd /path/to/light-saber

# Build the web app
cd app
npm install
npm run build
cd ..

# Build the LittleFS filesystem with the web app
pio run --target buildfs --environment light-saber

# Build the firmware
pio run --environment light-saber
```

**Step 2: Upload via OTA using curl**

```bash
# From the project root directory, run:
timeout 60 curl -F "filedata=@.pio/build/light-saber/firmware.bin" \
  http://<device-ip>/update

# Replace <device-ip> with your device's IP (e.g., 192.168.3.51)
```

**Expected output:**
- The LED should flash (device is rebooting)
- Wait 10-15 seconds for the device to restart
- Connection should resume automatically

**Step 3: Verify the update**

```bash
# Check the new version
curl http://<device-ip>/api/version

# Expected response:
# {
#   "firmware": "X.Y.Z",
#   "build_date": "Month DD YYYY HH:MM:SS"
# }
```

**Troubleshooting:**

| Issue | Solution |
|-------|----------|
| Connection timeout | Check device IP, ensure device is powered on and connected to WiFi |
| 500 Internal Server Error | Device may still be rebooting, wait 30 seconds and try again |
| Upload stuck | Press the reset button on the device and retry |
| Device unresponsive after upload | Power cycle the device (turn off/on) and check serial monitor for errors |

**Via Web Browser (Alternative method):**

If curl is not available, you can also upload via the web interface:

1. Open `http://<device-ip>` in your browser
2. Navigate to the firmware update section (if available in your build)
3. Select the firmware file: `.pio/build/light-saber/firmware.bin`
4. Click upload and wait for the device to restart

### Version Numbering

**Firmware:** Semantic versioning (MAJOR.MINOR.PATCH)
- Edit `src/Config.h` to update version:
```cpp
#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_PATCH 0
```

**App:** Managed in `app/package.json`
- Edit the `version` field:
```json
{
  "version": "1.0.0"
}
```

### Build Information

Each build captures:
- Version number (semantic versioning)
- Build date and time
- Compilation timestamp

This helps track which version is running and when it was built.

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

**Exposing Mosquitto via Swag Reverse-Proxy:**

If your Home Assistant is exposed online via Swag reverse-proxy, you can expose Mosquitto on a subdomain to allow the Light Saber to connect via the public domain:

1. **Add Mosquitto service to docker-compose.yml** (if not already present):
```yaml
mosquitto:
  image: eclipse-mosquitto:latest
  container_name: mosquitto
  ports:
    - "1883:1883"  # Internal port only, exposed via Swag
    - "9001:9001"  # WebSocket (optional)
  volumes:
    - ./mosquitto/config:/mosquitto/config
    - ./mosquitto/data:/mosquitto/data
  networks:
    - proxy
  environment:
    - TZ=Europe/Paris
```

2. **Create Mosquitto configuration** (mosquitto/config/mosquitto.conf):
```
listener 1883
protocol mqtt

allow_anonymous true
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
```

3. **Configure Swag proxy rules** (swag/nginx/proxy-confs/mosquitto.subdomain.conf):
```nginx
# For TCP MQTT (port 1883)
upstream mosquitto_backend {
    server mosquitto:1883;
}

server {
    listen 8883 ssl http2;
    listen [::]:8883 ssl http2;
    server_name mqtt.*;

    include /config/nginx/ssl.conf;
    include /config/nginx/resolver.conf;

    client_max_body_size 0;

    location / {
        proxy_pass mqtt://mosquitto_backend;
        proxy_buffering off;
        proxy_request_buffering off;
    }
}
```

4. **Configure Swag DNS records** and SSL (via your registrar):
```
mqtt   A   your.public.ip
```

5. **Update Light Saber MQTT settings**:
- Access web UI: `http://192.168.3.51`
- Settings → MQTT
- Broker: `mqtt.your-domain.com`
- Port: `8883`
- Save and restart

**Note:** MQTT uses raw TCP protocol which requires special Swag configuration. WebSocket MQTT (port 9001) is easier but requires additional setup. For simple deployments, keep the Light Saber on the internal network (192.168.x.x) and only expose Home Assistant to the internet.

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