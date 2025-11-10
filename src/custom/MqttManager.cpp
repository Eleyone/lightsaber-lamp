#include "MqttManager.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// Helper function to convert RGB to Hue/Saturation
// Returns hue (0-360) and saturation (0-100) for Home Assistant compatibility
struct HSColor {
  float hue;      // 0-360 degrees
  float saturation;  // 0-100 percent
};

HSColor rgbToHS(CRGB rgb) {
  float r = rgb.r / 255.0f;
  float g = rgb.g / 255.0f;
  float b = rgb.b / 255.0f;

  float cmax = max({r, g, b});
  float cmin = min({r, g, b});
  float delta = cmax - cmin;

  float hue = 0;
  if (delta == 0) {
    hue = 0;  // Achromatic (gray)
  } else if (cmax == r) {
    hue = 60 * (fmod(((g - b) / delta), 6));
    if (hue < 0) hue += 360;
  } else if (cmax == g) {
    hue = 60 * (((b - r) / delta) + 2);
  } else {
    hue = 60 * (((r - g) / delta) + 4);
  }

  float saturation = (cmax == 0) ? 0 : (delta / cmax) * 100;

  return {hue, saturation};
}

// Convert Hue/Saturation to RGB
CRGB hsToRGB(float hue, float saturation) {
  float s = saturation / 100.0f;
  float c = s;  // Chroma = saturation (assuming max value = 1)
  float h = hue / 60.0f;
  float x = c * (1 - abs(fmod(h, 2) - 1));

  float r, g, b;
  if (h < 1) {
    r = c; g = x; b = 0;
  } else if (h < 2) {
    r = x; g = c; b = 0;
  } else if (h < 3) {
    r = 0; g = c; b = x;
  } else if (h < 4) {
    r = 0; g = x; b = c;
  } else if (h < 5) {
    r = x; g = 0; b = c;
  } else {
    r = c; g = 0; b = x;
  }

  return CRGB((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

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
// Home Assistant MQTT Light Discovery format
void MqttManager::_publishDiscoveryForSaber(const char* saberName, const char* saberId, const char* saberPath) {
  if (!_isConnected || !_mqttClient) {
    Serial.printf("MQTT Discovery: Skipping %s (connected=%d, client=%p)\n", saberName, _isConnected, _mqttClient);
    return;
  }

  Serial.printf("MQTT Discovery: Publishing for %s (ID: %s)\n", saberName, saberId);

  // Build JSON using ArduinoJson - Home Assistant requires specific fields
  DynamicJsonDocument doc(768);  // Increased from 512 to accommodate all fields

  // Basic light entity properties
  doc["name"] = saberName;
  doc["unique_id"] = saberId;

  // MQTT Topic configuration
  String cmdTopic = String(saberPath) + "/set";
  String stateTopic = String(saberPath) + "/state";
  Serial.printf("MQTT Discovery: Topics - cmd: %s, state: %s\n", cmdTopic.c_str(), stateTopic.c_str());

  // Use short field names for compatibility (Home Assistant MQTT spec)
  doc["cmd_t"] = cmdTopic;      // Command topic
  doc["stat_t"] = stateTopic;   // State topic

  // Brightness configuration
  doc["bri_cmd_t"] = cmdTopic;  // Brightness command topic
  doc["bri_stat_t"] = stateTopic;  // Brightness state topic
  doc["bri_scl"] = 255;         // Brightness scale (0-255)

  // Hue/Saturation Color configuration (better HA support than raw RGB)
  doc["hs_cmd_t"] = cmdTopic;   // Hue/Saturation command topic
  doc["hs_stat_t"] = stateTopic;  // Hue/Saturation state topic

  // JSON payload support - tells HA to expect JSON format with state, brightness, color fields
  doc["schema"] = "json";

  // Supported color modes - using HS for better Home Assistant compatibility
  JsonArray colorModes = doc.createNestedArray("supported_color_modes");
  colorModes.add("hs");

  // Device information - important for grouping in Home Assistant
  JsonObject device = doc.createNestedObject("device");
  device["name"] = "Light Saber Lamp";
  device["identifiers"][0] = "light_saber_esp32";
  device["manufacturer"] = "DIY";
  device["model"] = "ESP32 Light Saber";
  device["sw_version"] = "1.0.0";

  // Serialize to buffer (make it bigger than the payload to be safe)
  char buffer[1024];
  size_t len = serializeJson(doc, buffer);
  if (len >= sizeof(buffer)) {
    Serial.printf("ERROR: Serialized JSON too large (%d bytes, buffer is %d)\n", len, (int)sizeof(buffer));
    return;
  }

  // Publish discovery to MQTT
  // Home Assistant MQTT Discovery expects: homeassistant/light/{device_id}/config
  String discoveryTopic = String(saberPath) + "/config";
  Serial.printf("Publishing discovery payload (%d bytes) to %s\n", len, discoveryTopic.c_str());
  Serial.printf("Payload: %s\n", buffer);

  // Use QoS 1 with retain flag for reliable delivery and persistence
  // publish(topic, payload, length, retain) with QoS 1
  bool publishResult = _mqttClient->publish(discoveryTopic.c_str(), (uint8_t *)buffer, len, true);
  Serial.printf("MQTT publish result (retain=1): %d (1=success, 0=failure)\n", publishResult);

  if (!publishResult) {
    Serial.printf("MQTT publish FAILED! Client state: %d, Connected: %d, Buffer space: %d\n",
      _mqttClient->state(), _mqttClient->connected(), MQTT_MAX_PACKET_SIZE);
    Serial.println("Possible causes: Buffer full, message too large, or client not fully ready");

    // Try one more time - sometimes the first attempt fails but retry works
    delay(50);
    publishResult = _mqttClient->publish(discoveryTopic.c_str(), (uint8_t *)buffer, len, true);
    Serial.printf("MQTT retry result: %d\n", publishResult);
  }

  if (publishResult) {
    Serial.printf("✓ Published discovery for %s\n", saberName);
  } else {
    Serial.printf("✗ FAILED to publish discovery for %s\n", saberName);
  }
}

void MqttManager::_publishDiscovery() {
  if (!_isConnected || !_mqttClient) return;

  // Luke Light Discovery
  _publishDiscoveryForSaber(
    "Luke Saber",
    "luke_saber_esp32",
    "homeassistant/light/luke_saber_esp32"
  );

  // Vader Light Discovery
  _publishDiscoveryForSaber(
    "Vader Saber",
    "vader_saber_esp32",
    "homeassistant/light/vader_saber_esp32"
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
  if (topicStr.indexOf("luke_saber_esp32/set") > 0) {
    bool stateChanged = false;
    if (doc.containsKey("state")) {
      bool state = doc["state"] == "ON";
      _lukeLedStrip->changeState(state);
      stateChanged = true;
    }
    if (doc.containsKey("brightness")) {
      uint8_t brightness = doc["brightness"];
      _lukeLedStrip->changeBrightness(brightness);
      stateChanged = true;
    }
    if (doc.containsKey("color")) {
      JsonObject color = doc["color"];
      // Support both RGB format (r, g, b) and HS format (h, s)
      if (color.containsKey("h") && color.containsKey("s")) {
        // Hue/Saturation format (from Home Assistant HS color mode)
        float hue = color["h"];
        float saturation = color["s"];
        CRGB newColor = hsToRGB(hue, saturation);
        _lukeLedStrip->changeColor(newColor);
        stateChanged = true;
      } else if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
        // RGB format (legacy, from web UI)
        CRGB newColor = CRGB(color["r"], color["g"], color["b"]);
        _lukeLedStrip->changeColor(newColor);
        stateChanged = true;
      }
    }
    // Notify WebSocket clients of state change
    if (stateChanged && _onStateChangeCallback) {
      _onStateChangeCallback();
    }
  }
  // Vader commands
  else if (topicStr.indexOf("vader_saber_esp32/set") > 0) {
    bool stateChanged = false;
    if (doc.containsKey("state")) {
      bool state = doc["state"] == "ON";
      _vaderLedStrip->changeState(state);
      stateChanged = true;
    }
    if (doc.containsKey("brightness")) {
      uint8_t brightness = doc["brightness"];
      _vaderLedStrip->changeBrightness(brightness);
      stateChanged = true;
    }
    if (doc.containsKey("color")) {
      JsonObject color = doc["color"];
      // Support both RGB format (r, g, b) and HS format (h, s)
      if (color.containsKey("h") && color.containsKey("s")) {
        // Hue/Saturation format (from Home Assistant HS color mode)
        float hue = color["h"];
        float saturation = color["s"];
        CRGB newColor = hsToRGB(hue, saturation);
        _vaderLedStrip->changeColor(newColor);
        stateChanged = true;
      } else if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
        // RGB format (legacy, from web UI)
        CRGB newColor = CRGB(color["r"], color["g"], color["b"]);
        _vaderLedStrip->changeColor(newColor);
        stateChanged = true;
      }
    }
    // Notify WebSocket clients of state change
    if (stateChanged && _onStateChangeCallback) {
      _onStateChangeCallback();
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
        _mqttClient->subscribe("homeassistant/light/luke_saber_esp32/set");
        _mqttClient->subscribe("homeassistant/light/vader_saber_esp32/set");

        // Give the client a brief moment to settle after connection
        delay(100);

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

  // Home Assistant expects a combined JSON on the state topic with HS color format
  // This JSON includes state, brightness, and hue/saturation all together
  DynamicJsonDocument statusDoc(512);

  statusDoc["state"] = _lukeLedStrip->getState() ? "ON" : "OFF";
  statusDoc["brightness"] = _lukeLedStrip->getBrightness();

  // Convert RGB to Hue/Saturation for Home Assistant compatibility
  CRGB color = _lukeLedStrip->getColor();
  HSColor hsColor = rgbToHS(color);

  JsonObject colorObj = statusDoc.createNestedObject("color");
  colorObj["h"] = hsColor.hue;      // Hue in degrees (0-360)
  colorObj["s"] = hsColor.saturation; // Saturation in percent (0-100)

  // Serialize combined status
  char buffer[512];
  size_t len = serializeJson(statusDoc, buffer);

  // Publish complete status as JSON
  _mqttClient->publish("homeassistant/light/luke_saber_esp32/state", (uint8_t *)buffer, len, true);

  Serial.printf("Published Luke status: %s\n", buffer);
}

void MqttManager::publishVaderStatus() {
  if (!_isConnected || !_mqttClient) {
    return;
  }

  Serial.println("MQTT: Publishing Vader status");

  // Home Assistant expects a combined JSON on the state topic with HS color format
  // This JSON includes state, brightness, and hue/saturation all together
  DynamicJsonDocument statusDoc(512);

  statusDoc["state"] = _vaderLedStrip->getState() ? "ON" : "OFF";
  statusDoc["brightness"] = _vaderLedStrip->getBrightness();

  // Convert RGB to Hue/Saturation for Home Assistant compatibility
  CRGB color = _vaderLedStrip->getColor();
  HSColor hsColor = rgbToHS(color);

  JsonObject colorObj = statusDoc.createNestedObject("color");
  colorObj["h"] = hsColor.hue;      // Hue in degrees (0-360)
  colorObj["s"] = hsColor.saturation; // Saturation in percent (0-100)

  // Serialize combined status
  char buffer[512];
  size_t len = serializeJson(statusDoc, buffer);

  // Publish complete status as JSON
  _mqttClient->publish("homeassistant/light/vader_saber_esp32/state", (uint8_t *)buffer, len, true);

  Serial.printf("Published Vader status: %s\n", buffer);
}

void MqttManager::publishAllStatus() {
  publishLukeStatus();
  publishVaderStatus();
}

void MqttManager::republishDiscovery() {
  Serial.println("MQTT: Republishing discovery messages");
  if (!_isConnected || !_mqttClient) {
    Serial.println("MQTT: Cannot republish discovery - not connected");
    return;
  }
  _publishDiscovery();
}
