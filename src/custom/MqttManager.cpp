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
  _mqttUsername = _prefs->getString("username", "");
  _mqttPassword = _prefs->getString("password", "");
  _prefs->end();

  Serial.printf("MQTT Config loaded: broker=%s, port=%d, user=%s\n", _mqttBroker.c_str(), _mqttPort, _mqttUsername.c_str());
}

void MqttManager::saveConfig(String broker, uint16_t port, String username, String password) {
  _prefs->begin("mqtt", false);
  _prefs->putString("broker", broker);
  _prefs->putUInt("port", port);
  _prefs->putString("username", username);
  _prefs->putString("password", password);
  _prefs->end();

  _mqttBroker = broker;
  _mqttPort = port;
  _mqttUsername = username;
  _mqttPassword = password;
  Serial.printf("MQTT Config saved: broker=%s, port=%d, user=%s\n", broker.c_str(), port, username.c_str());
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

  // Check if WiFi has a valid IP address
  // Note: WiFi.status() may return confusing values for hidden networks,
  // so we check for a valid local IP instead
  IPAddress localIP = WiFi.localIP();
  if (localIP == IPAddress(0, 0, 0, 0)) {
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

      // Connect with credentials if available, otherwise connect without
      bool connected;
      if (!_mqttUsername.isEmpty() && !_mqttPassword.isEmpty()) {
        Serial.printf("MQTT: Connecting with credentials (user=%s)...\n", _mqttUsername.c_str());
        connected = _mqttClient->connect(_mqttClientId.c_str(), _mqttUsername.c_str(), _mqttPassword.c_str());
      } else {
        Serial.println("MQTT: Connecting without credentials...");
        connected = _mqttClient->connect(_mqttClientId.c_str());
      }

      if (connected) {
        Serial.println("MQTT: Connected!");
        _isConnected = true;

        // Subscribe to command topics
        _mqttClient->subscribe("homeassistant/light/light_saber/luke_saber/set");
        _mqttClient->subscribe("homeassistant/light/light_saber/vader_saber/set");

        // Publish discovery and status
        _publishDiscovery();
        publishAllStatus();
      } else {
        int state = _mqttClient->state();
        Serial.printf("MQTT: Connection failed (state=%d), will retry in %lu ms\n", state, RECONNECT_INTERVAL_MS);
        // State codes: -4=MQTT_CONNECTION_TIMEOUT, -3=MQTT_CONNECTION_LOST, -2=MQTT_CONNECT_FAILED, -1=MQTT_DISCONNECTED, 0=MQTT_CONNECTED
        // 1=MQTT_UNACCEPTABLE_PROTOCOL_VERSION, 2=MQTT_IDENTIFIER_REJECTED, 3=MQTT_SERVER_UNAVAILABLE, 4=MQTT_BAD_USERNAME_PASSWORD, 5=MQTT_NOT_AUTHORIZED
        Serial.println("MQTT state meanings: -4=TIMEOUT, -3=LOST, -2=FAILED, -1=DISCONNECTED, 1=BAD_PROTOCOL, 2=ID_REJECTED, 3=SERVER_UNAVAILABLE, 4=BAD_CREDS, 5=NOT_AUTHORIZED");
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
