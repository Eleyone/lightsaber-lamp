#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <AsyncElegantOTA.h>
#include <InputDebounce.h>
#include "custom/Ledstrip.h"
#include "custom/Endpoint.h"
#include "custom/Network.h"
#include "Config.h"

Preferences prefs;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

CRGB lukeLeds[NB_LED_SABER];
CRGB vaderLeds[NB_LED_SABER];
CLEDController *ledControllers[2];

Ledstrip lukeLedStrip("luke_strip", COLOR_LUKE);
Ledstrip vaderLedStrip("vader_strip", COLOR_VADER);

Network network(&prefs);
Endpoint endpoint(&server, &ws, &prefs, &lukeLedStrip, &vaderLedStrip);

static InputDebounce buttonLuke;
static InputDebounce buttonVader;

bool configured;

void wsEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      endpoint.wsBroadcastLedStatus();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      endpoint.wsMessageHandler(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void buttonLukePressedCallback(uint8_t pinIn) {
  Serial.print("Luke button push");
  bool newLukeState = !lukeLedStrip.getPreviousState();
  lukeLedStrip.changeState(newLukeState);
}

void buttonVaderPressedCallback(uint8_t pinIn) {
  Serial.print("Vader button push");
  bool newVaderState = !vaderLedStrip.getPreviousState();
  vaderLedStrip.changeState(newVaderState);
}

void loopButton() {
  unsigned long now = millis();
  buttonLuke.process(now);
  buttonVader.process(now);
}

void powerUpSaber(int saber) {
  int idx = 0;
  int maxIdx = NB_LED_SABER;

  for (int n = idx; n < maxIdx; n++) {
    switch (saber) 
    {
      case LUKE:
        Serial.println("Power up Luke saber");
        lukeLeds[n] = lukeLedStrip.getColor();
        ledControllers[LUKE]->showLeds(lukeLedStrip.getBrightness());
        break;
      case VADER:
        Serial.println("Power up Vader saber");
        vaderLeds[n] = vaderLedStrip.getColor();
        ledControllers[VADER]->showLeds(vaderLedStrip.getBrightness());
        break;
      default:
        Serial.println("Unknown saber");
        break;
    }
    delay(20);
  }
}

void powerDownSaber(int saber) {
  int idx = 0;
  int maxIdx = NB_LED_SABER;

  for (int n = maxIdx; n >= idx ; n--) {
    switch (saber) 
    {
      case LUKE:
        Serial.println("Power down Luke saber");
        lukeLeds[n] = CRGB::Black;
        ledControllers[LUKE]->showLeds(lukeLedStrip.getBrightness());
        break;
      case VADER:
        Serial.println("Power down Vader saber");
        vaderLeds[n] = CRGB::Black;
        ledControllers[VADER]->showLeds(vaderLedStrip.getBrightness());
        break;
      default:
        Serial.println("Unknown saber");
        break;
    }
    delay(20);
  }
}

void powerLoopSaber(int saber) {
  int idx = 0;
  int maxIdx = NB_LED_SABER;

  for (int n = 0; n < maxIdx ; n++) {
    switch (saber) 
    {
      case LUKE:
        //Serial.println("Power loop Luke saber");
        if (lukeLedStrip.getState()) {
          lukeLeds[n] = lukeLedStrip.getColor();
        } else {
          lukeLeds[n] = CRGB::Black;
        }
        ledControllers[LUKE]->showLeds(lukeLedStrip.getBrightness());
        break;
      case VADER:
        //Serial.println("Power loop Vader saber");
        if (vaderLedStrip.getState()) {
          vaderLeds[n] = vaderLedStrip.getColor();
        } else {
          vaderLeds[n] = CRGB::Black;
        }
        ledControllers[VADER]->showLeds(vaderLedStrip.getBrightness());
        break;
      default:
        Serial.println("Unknown saber");
        break;
    }
  }
}

void loopLed() {
  if (lukeLedStrip.hasStateChanged()) {
    if (lukeLedStrip.getState()) {
      powerUpSaber(LUKE);
    } else {
      powerDownSaber(LUKE);
    }
    lukeLedStrip.resetPreviousState();
  } else {
    powerLoopSaber(LUKE);
  }
  if (vaderLedStrip.hasStateChanged()) {
    if (vaderLedStrip.getState()) {
      powerUpSaber(VADER);
    } else {
      powerDownSaber(VADER);
    }
    vaderLedStrip.resetPreviousState();
  } else {
    powerLoopSaber(VADER);
  }
}

void setup() {
  Serial.begin(115200); 
  Serial.println("LightSaberLamp start");

  if (!LittleFS.begin(1)) {
    Serial.println("Failling mounting LittleFS particition");
    return;
  }

  network.initConnexion();

  if (network.isAccessPoint()) {
    endpoint.initHotpsotRouting();
  } else {
    endpoint.initClientRouting();
  }

  AsyncElegantOTA.begin(&server);

  ws.onEvent(wsEventHandler);

  server.addHandler(&ws);
  server.begin();
  Serial.println("HTTP server started");

  ledControllers[LUKE] = &FastLED.addLeds<LED_TYPE, LUKE_LED_PIN, COLOR_ORDER>(lukeLeds, NB_LED_SABER);
  ledControllers[VADER] = &FastLED.addLeds<LED_TYPE, VADER_LED_PIN, COLOR_ORDER>(vaderLeds, NB_LED_SABER);
  lukeLedStrip.initLedstrip();
  vaderLedStrip.initLedstrip();

  buttonLuke.registerCallbacks(buttonLukePressedCallback, NULL, NULL, NULL);
  buttonVader.registerCallbacks(buttonVaderPressedCallback, NULL, NULL, NULL);
  buttonLuke.setup(LUKE_SWITCH_PIN, BUTTON_DEBOUNCE_DELAY);
  buttonVader.setup(VADER_SWITCH_PIN, BUTTON_DEBOUNCE_DELAY);
}

void loop() {
  loopButton();
  loopLed();
}
