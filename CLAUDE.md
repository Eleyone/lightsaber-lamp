# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LightSaber Lamp is an ESP32-based IoT device that controls two addressable LED light sabers using a web interface. It consists of two main parts:
- **Firmware**: C++ code running on ESP32 (using Arduino framework and PlatformIO)
- **Web App**: HTML/JS/CSS frontend using Tailwind CSS and Alpine.js

The device supports WiFi connectivity, OTA updates, and WebSocket communication between the firmware and web interface.

## Build Commands

### Build the Web App
```bash
cd app && npm install && npm run build
```
This creates optimized static files that get embedded into the ESP32 filesystem.

### Build Firmware
```bash
pio run --environment light-saber
```

### Build Filesystem (LittleFS)
```bash
pio run --target buildfs --environment light-saber
```
This packages the built web app into the LittleFS partition.

### Full Build (App + Filesystem + Firmware)
```bash
cd app && npm install && npm run build && cd .. && pio run --target buildfs --environment light-saber && pio run --environment light-saber
```
This mirrors the GitLab CI process.

### Monitor Serial Output
```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```
Useful for debugging - shows Serial.print() output from the firmware.

## Architecture

### Firmware Architecture (`src/`)
The firmware is organized into modular components:

- **light-saber.cpp**: Main entry point with setup() and loop() functions. Handles:
  - LED state management (power up/down/loop animations)
  - Button input processing (physical switches for Luke and Vader sabers)
  - WebSocket connection handling

- **custom/Ledstrip.h/cpp**: `Ledstrip` class managing individual LED strips
  - Stores state, color, brightness for each saber
  - Persists settings to flash storage (Preferences)
  - Tracks state changes to trigger animations

- **custom/Endpoint.h/cpp**: HTTP and WebSocket endpoints
  - REST API for WiFi/network configuration
  - WebSocket message handlers for real-time communication with web UI
  - Status JSON serialization

- **custom/Network.h/cpp**: WiFi management
  - Access point (hotspot) mode setup
  - Client mode (connects to existing WiFi) setup
  - Network configuration persistence

- **Config.h**: Hardware pin definitions and constants
  - GPIO pins for LEDs (GPIO 18, 21) and buttons (GPIO 32, 33)
  - LED configuration (WS2811, 36 LEDs per strip, BRG color order)
  - Default colors: Blue for Luke, Red for Vader

### Web App Architecture (`app/src/`)

Built with Parcel bundler, Tailwind CSS, and Alpine.js:

- **html/**: HTML templates using PostHTML for component includes
  - Main entry point is `index.html`
  - Components in `components/` folder (control panels, settings, notifications)

- **js/app.js**: Main Alpine.js application logic
  - Coordinates between components
  - Manages global state

- **js/components/**: Modular JavaScript components
  - `websocket.js`: WebSocket connection and message handling
  - `endpoint.js`: HTTP endpoint utilities
  - `color.js`: Color manipulation and hex conversion
  - `notifs.js`: User notifications

- **css/app.css**: Tailwind CSS with custom components

## Key Concepts

### LED Animation Flow
1. Button press → `buttonLukePressed/buttonVaderPressed()` callbacks
2. Changes LED state via `Ledstrip.changeState()`
3. Main loop detects state change in `loopLed()`
4. Triggers `powerUpSaber()` or `powerDownSaber()` with 20ms delay per LED
5. Continuous `powerLoopSaber()` updates during steady state
6. WebSocket broadcasts new status to all connected clients

### WebSocket Communication
- Protocol: JSON messages over `/ws` endpoint
- Client connects and immediately receives full LED status
- Supports real-time color/brightness/state changes
- Device broadcasts status changes to all connected clients

### Configuration Persistence
- Uses ESP32 `Preferences` API to store WiFi/network settings
- LED strip properties (color, brightness) saved per saber
- Persisted settings loaded on boot via `initLedstrip()`

## Dependencies

### Firmware Libraries (PlatformIO)
- **FastLED**: LED strip control and animations
- **ElegantOTA** (async mode): Over-the-air firmware updates (`/update` page, `/ota/start` + `/ota/upload`)
- **ESPAsyncWebServer**: HTTP and WebSocket server
- **ArduinoJson**: JSON serialization for configuration/status
- **InputDebounce**: Button debouncing (20ms)

### Web App Packages
- **Alpine.js**: Lightweight reactive framework
- **Tailwind CSS**: Utility-first CSS framework
- **Parcel**: Zero-config bundler

## Testing

PlatformIO unit tests are configured but not yet implemented. Test directory at `test/` can be used for Arduino unit tests via:
```bash
pio test -e light-saber
```

## Hardware Configuration

ESP32-DevKitC V4 pinout (for reference):
- Luke LED strip: GPIO 18
- Vader LED strip: GPIO 21
- Luke button: GPIO 33
- Vader button: GPIO 32
- Audio outputs: GPIO 4 (Luke speaker), GPIO 16 (Vader speaker)

LED configuration: 36 WS2811 LEDs per strip, BRG color order.
