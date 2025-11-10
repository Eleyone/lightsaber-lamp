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
#include "custom/MqttManager.h"
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
MqttManager mqttManager(&prefs, &lukeLedStrip, &vaderLedStrip);
Endpoint endpoint(&server, &ws, &prefs, &lukeLedStrip, &vaderLedStrip, &mqttManager);

static InputDebounce buttonLuke;
static InputDebounce buttonVader;

bool configured;

// LED Animation state machine (non-blocking)
struct LEDAnimationState {
  enum State { IDLE, POWERING_UP, POWERING_DOWN } state = IDLE;
  int currentLED = 0;
  unsigned long lastStepTime = 0;
  const unsigned long STEP_DELAY_MS = 20;
  int saber = -1;  // LUKE or VADER
};

LEDAnimationState lukeLEDState, vaderLEDState;

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
  endpoint.wsBroadcastLedStatus();
}

void buttonVaderPressedCallback(uint8_t pinIn) {
  Serial.print("Vader button push");
  bool newVaderState = !vaderLedStrip.getPreviousState();
  vaderLedStrip.changeState(newVaderState);
  endpoint.wsBroadcastLedStatus();
}

void loopButton() {
  unsigned long now = millis();
  buttonLuke.process(now);
  buttonVader.process(now);
}

// Non-blocking LED animation update - OPTIMIZED
// Only calls showLeds() once per step, not per LED
void updateLEDAnimation(LEDAnimationState &animState, Ledstrip &ledStrip, CRGB *leds, CLEDController *controller) {
  unsigned long now = millis();

  // Check if it's time for next step
  if (now - animState.lastStepTime < animState.STEP_DELAY_MS) {
    return;  // Not time yet, skip
  }

  animState.lastStepTime = now;
  bool needsDisplay = false;

  switch (animState.state) {
    case LEDAnimationState::POWERING_UP:
      if (animState.currentLED < NB_LED_SABER) {
        leds[animState.currentLED] = ledStrip.getColor();
        animState.currentLED++;
        needsDisplay = true;
      } else {
        // Animation complete - switch to IDLE for continuous update
        animState.state = LEDAnimationState::IDLE;
        Serial.println("Power up complete");
      }
      break;

    case LEDAnimationState::POWERING_DOWN:
      if (animState.currentLED >= 0) {
        leds[animState.currentLED] = CRGB::Black;
        animState.currentLED--;
        needsDisplay = true;
      } else {
        // Animation complete - switch to IDLE for continuous update
        animState.state = LEDAnimationState::IDLE;
        Serial.println("Power down complete");
      }
      break;

    case LEDAnimationState::IDLE:
      // Idle state - only update if state changed (handled by hasStateChanged in loopLed)
      // This keeps the current display without unnecessary updates
      break;
  }

  // Display update happens only when needed (animation step or state change)
  if (needsDisplay) {
    controller->showLeds(ledStrip.getBrightness());
  }
}

// Start power-up animation
void startPowerUpSaber(int saber) {
  LEDAnimationState &animState = (saber == LUKE) ? lukeLEDState : vaderLEDState;
  animState.state = LEDAnimationState::POWERING_UP;
  animState.currentLED = 0;
  animState.lastStepTime = millis();
  animState.saber = saber;
  Serial.printf("Starting power up for saber %d\n", saber);
}

// Start power-down animation
void startPowerDownSaber(int saber) {
  LEDAnimationState &animState = (saber == LUKE) ? lukeLEDState : vaderLEDState;
  animState.state = LEDAnimationState::POWERING_DOWN;
  animState.currentLED = NB_LED_SABER - 1;
  animState.lastStepTime = millis();
  animState.saber = saber;
  Serial.printf("Starting power down for saber %d\n", saber);
}

void loopLed() {
  // Handle Luke state changes
  if (lukeLedStrip.hasStateChanged()) {
    if (lukeLedStrip.getState()) {
      startPowerUpSaber(LUKE);
    } else {
      startPowerDownSaber(LUKE);
    }
    lukeLedStrip.resetPreviousState();
  }

  // Handle Vader state changes
  if (vaderLedStrip.hasStateChanged()) {
    if (vaderLedStrip.getState()) {
      startPowerUpSaber(VADER);
    } else {
      startPowerDownSaber(VADER);
    }
    vaderLedStrip.resetPreviousState();
  }

  // Update LED animations (non-blocking)
  updateLEDAnimation(lukeLEDState, lukeLedStrip, lukeLeds, ledControllers[LUKE]);
  updateLEDAnimation(vaderLEDState, vaderLedStrip, vaderLeds, ledControllers[VADER]);
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

  mqttManager.init();

  ledControllers[LUKE] = &FastLED.addLeds<LED_TYPE, LUKE_LED_PIN, COLOR_ORDER>(lukeLeds, NB_LED_SABER);
  ledControllers[VADER] = &FastLED.addLeds<LED_TYPE, VADER_LED_PIN, COLOR_ORDER>(vaderLeds, NB_LED_SABER);
  lukeLedStrip.initLedstrip();
  vaderLedStrip.initLedstrip();

  // Initialize LED display to current state (all off by default)
  for (int n = 0; n < NB_LED_SABER; n++) {
    lukeLeds[n] = CRGB::Black;
    vaderLeds[n] = CRGB::Black;
  }
  ledControllers[LUKE]->showLeds(0);
  ledControllers[VADER]->showLeds(0);

  buttonLuke.registerCallbacks(buttonLukePressedCallback, NULL, NULL, NULL);
  buttonVader.registerCallbacks(buttonVaderPressedCallback, NULL, NULL, NULL);
  buttonLuke.setup(LUKE_SWITCH_PIN, BUTTON_DEBOUNCE_DELAY);
  buttonVader.setup(VADER_SWITCH_PIN, BUTTON_DEBOUNCE_DELAY);
}

void loop() {
  loopButton();
  loopLed();
  mqttManager.loop();
}
