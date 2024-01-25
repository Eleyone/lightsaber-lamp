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
- AiEsp32RotaryEncoder

### Build

First you need to build the app. To do so, from the app directory run command
```npm run build```

Once this is done, you can upload data partition to the ESP, build the firmware and uplaod it.

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