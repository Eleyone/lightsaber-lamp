#include "MqttManager.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// Instance statique pour le callback
MqttManager *MqttManager::_instance = nullptr;

void MqttManager::_mqttCallback(char *topic, byte *payload, unsigned int length) {
  if (_instance) {
    _instance->_handleMessage(topic, payload, length);
  }
}

MqttManager::MqttManager(Preferences *prefs, Ledstrip *lukeLedStrip, Ledstrip *vaderLedStrip)
  : _prefs(prefs),
    _lukeLedStrip(lukeLedStrip),
    _vaderLedStrip(vaderLedStrip),
    _mqttPort(1883),
    _mqttClientId("light-saber-esp32"),
    _isInitialized(false),
    _isConnected(false),
    _mqttClient(nullptr)
{
  _instance = this;
}

MqttManager::~MqttManager() {
  if (_mqttClient) {
    delete _mqttClient;
  }
}

void MqttManager::loadConfig() {
  _prefs->begin("mqtt", true);
  _mqttBroker = _prefs->getString("broker", "");
  _mqttPort = _prefs->getUInt("port", 1883);
  _prefs->end();

  Serial.printf("MQTT Config loaded: broker=%s, port=%d\n", _mqttBroker.c_str(), _mqttPort);
}

void MqttManager::saveConfig(String broker, uint16_t port) {
  _prefs->begin("mqtt", false);
  _prefs->putString("broker", broker);
  _prefs->putUInt("port", port);
  _prefs->end();

  _mqttBroker = broker;
  _mqttPort = port;
  Serial.printf("MQTT Config saved: broker=%s, port=%d\n", broker.c_str(), port);
}

void MqttManager::resetConfig() {
  _prefs->begin("mqtt", false);
  _prefs->putString("broker", "");
  _prefs->putUInt("port", 1883);
  _prefs->end();

  _mqttBroker = "";
  _mqttPort = 1883;
  Serial.println("MQTT Config reset");
}

void MqttManager::_setupMqtt() {
  if (_mqttClient) {
    delete _mqttClient;
  }

  _mqttClient = new PubSubClient(espClient);
  _mqttClient->setServer(_mqttBroker.c_str(), _mqttPort);
  _mqttClient->setCallback(_mqttCallback);

  Serial.println("MQTT Client setup complete");
}

// Helper function to create discovery payload for a saber
void MqttManager::_publishDiscoveryForSaber(const char* saberName, const char* saberId, const char* saberPath) {
  if (!_isConnected || !_mqttClient) return;

  // Build JSON using ArduinoJson instead of string concatenation
  DynamicJsonDocument doc(512);  // 512 bytes for discovery payload

  doc["name"] = saberName;
  doc["unique_id"] = saberId;
  doc["cmd_t"] = String(saberPath) + "/set";
  doc["stat_t"] = String(saberPath) + "/state";
  doc["bri_cmd_t"] = String(saberPath) + "/brightness/set";
  doc["bri_stat_t"] = String(saberPath) + "/brightness";
  doc["rgb_cmd_t"] = String(saberPath) + "/color/set";
  doc["rgb_stat_t"] = String(saberPath) + "/color";
  doc["brightness_scale"] = 255;
  doc["schema"] = "json";

  JsonObject device = doc.createNestedObject("device");
  device["name"] = "Light Saber Lamp";
  device["identifiers"][0] = "light_saber_esp32";

  // Serialize to buffer
  char buffer[512];
  size_t len = serializeJson(doc, buffer);

  // Publish discovery
  String discoveryTopic = String(saberPath) + "/config";
  _mqttClient->publish(discoveryTopic.c_str(), (uint8_t *)buffer, len, true);
  Serial.printf("Published discovery for %s: %s\n", saberName, discoveryTopic.c_str());
}

void MqttManager::_publishDiscovery() {
  if (!_isConnected || !_mqttClient) return;

  // Luke Light Discovery
  _publishDiscoveryForSaber(
    "Luke Saber",
    "luke_saber_esp32",
    "homeassistant/light/light_saber/luke_saber"
  );

  // Vader Light Discovery
  _publishDiscoveryForSaber(
    "Vader Saber",
    "vader_saber_esp32",
    "homeassistant/light/light_saber/vader_saber"
  );
}

void MqttManager::_handleMessage(char *topic, byte *payload, unsigned int length) {
  String topicStr = String(topic);
  String message((char *)payload, length);

  Serial.printf("MQTT Message - Topic: %s, Message: %s\n", topic, message.c_str());

  // Parse JSON from message
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, message) != DeserializationError::Ok) {
    Serial.println("Failed to deserialize JSON");
    return;
  }

  // Luke commands
  if (topicStr.indexOf("luke_saber/set") > 0) {
    if (doc.containsKey("state")) {
      bool state = doc["state"] == "ON";
      _lukeLedStrip->changeState(state);
    }
    if (doc.containsKey("brightness")) {
      uint8_t brightness = doc["brightness"];
      _lukeLedStrip->changeBrightness(brightness);
    }
    if (doc.containsKey("color")) {
      JsonObject color = doc["color"];
      if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
        CRGB newColor = CRGB(color["r"], color["g"], color["b"]);
        _lukeLedStrip->changeColor(newColor);
      }
    }
  }
  // Vader commands
  else if (topicStr.indexOf("vader_saber/set") > 0) {
    if (doc.containsKey("state")) {
      bool state = doc["state"] == "ON";
      _vaderLedStrip->changeState(state);
    }
    if (doc.containsKey("brightness")) {
      uint8_t brightness = doc["brightness"];
      _vaderLedStrip->changeBrightness(brightness);
    }
    if (doc.containsKey("color")) {
      JsonObject color = doc["color"];
      if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
        CRGB newColor = CRGB(color["r"], color["g"], color["b"]);
        _vaderLedStrip->changeColor(newColor);
      }
    }
  }
}

void MqttManager::init() {
  loadConfig();

  if (_mqttBroker.isEmpty()) {
    Serial.println("MQTT: No broker configured, skipping MQTT initialization");
    _isInitialized = false;
    return;
  }

  Serial.printf("MQTT: Initializing with broker: %s:%d\n", _mqttBroker.c_str(), _mqttPort);

  _setupMqtt();
  _isInitialized = true;

  Serial.println("MQTT Manager initialized");
}

void MqttManager::loop() {
  if (!_isInitialized || !_mqttClient) {
    return;
  }

  // Si pas de WiFi, on sort
  if (WiFi.status() != WL_CONNECTED) {
    _isConnected = false;
    return;
  }

  // Connexion/reconnexion - using timer instead of unreliable millis() % logic
  if (!_mqttClient->connected()) {
    _isConnected = false;
    unsigned long now = millis();

    // Only attempt reconnection at regular intervals
    if (now - _lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
      _lastReconnectAttempt = now;
      Serial.printf("MQTT: Attempting to connect to %s:%d\n", _mqttBroker.c_str(), _mqttPort);

      if (_mqttClient->connect(_mqttClientId.c_str())) {
        Serial.println("MQTT: Connected!");
        _isConnected = true;

        // Subscribe to command topics
        _mqttClient->subscribe("homeassistant/light/light_saber/luke_saber/set");
        _mqttClient->subscribe("homeassistant/light/light_saber/vader_saber/set");

        // Publish discovery and status
        _publishDiscovery();
        publishAllStatus();
      } else {
        Serial.printf("MQTT: Connection failed, will retry in %lu ms\n", RECONNECT_INTERVAL_MS);
      }
    }
  } else {
    _isConnected = true;
    _mqttClient->loop();
  }
}

void MqttManager::publishLukeStatus() {
  if (!_isConnected || !_mqttClient) {
    return;
  }

  Serial.println("MQTT: Publishing Luke status");

  // Publish state
  String stateMsg = _lukeLedStrip->getState() ? "ON" : "OFF";
  _mqttClient->publish("homeassistant/light/light_saber/luke_saber/state", stateMsg.c_str(), true);

  // Publish brightness
  String brightnessMsg = String(_lukeLedStrip->getBrightness());
  _mqttClient->publish("homeassistant/light/light_saber/luke_saber/brightness", brightnessMsg.c_str(), true);

  // Publish color as RGB
  CRGB color = _lukeLedStrip->getColor();
  char colorMsg[32];
  snprintf(colorMsg, sizeof(colorMsg), "{\"r\":%d,\"g\":%d,\"b\":%d}", color.r, color.g, color.b);
  _mqttClient->publish("homeassistant/light/light_saber/luke_saber/color", (uint8_t *)colorMsg, strlen(colorMsg), true);
}

void MqttManager::publishVaderStatus() {
  if (!_isConnected || !_mqttClient) {
    return;
  }

  Serial.println("MQTT: Publishing Vader status");

  // Publish state
  String stateMsg = _vaderLedStrip->getState() ? "ON" : "OFF";
  _mqttClient->publish("homeassistant/light/light_saber/vader_saber/state", stateMsg.c_str(), true);

  // Publish brightness
  String brightnessMsg = String(_vaderLedStrip->getBrightness());
  _mqttClient->publish("homeassistant/light/light_saber/vader_saber/brightness", brightnessMsg.c_str(), true);

  // Publish color as RGB
  CRGB color = _vaderLedStrip->getColor();
  char colorMsg[32];
  snprintf(colorMsg, sizeof(colorMsg), "{\"r\":%d,\"g\":%d,\"b\":%d}", color.r, color.g, color.b);
  _mqttClient->publish("homeassistant/light/light_saber/vader_saber/color", (uint8_t *)colorMsg, strlen(colorMsg), true);
}

void MqttManager::publishAllStatus() {
  publishLukeStatus();
  publishVaderStatus();
}
