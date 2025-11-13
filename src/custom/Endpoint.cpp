#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../Config.h"
#include "Ledstrip.h"
#include "Endpoint.h"
#include "MqttManager.h"
#include "Utils.h"

Endpoint::Endpoint(AsyncWebServer *server, AsyncWebSocket *websocket, Preferences *prefs, Ledstrip *lukeLedStrip, Ledstrip *vaderLedStrip, MqttManager *mqttManager):
  _prefs(prefs),
  _server(server),
  _lukeLedStrip(lukeLedStrip),
  _vaderLedStrip(vaderLedStrip),
  _websocket(websocket),
  _mqttManager(mqttManager)
{}

DynamicJsonDocument Endpoint::_getFullStatusJson() {
  // Reuse the buffer instead of allocating new one each time
  _jsonBuffer.clear();

  JsonObject jsonLukeLedStrip  = _jsonBuffer.createNestedObject(_lukeLedStrip->getName());
  jsonLukeLedStrip["state"] = _lukeLedStrip->getState();
  jsonLukeLedStrip["color"] = Utils::getHEXColorStringFromCRGB(_lukeLedStrip->getColor());
  jsonLukeLedStrip["brightness"] = _lukeLedStrip->getBrightness();

  JsonObject jsonVaderLedStrip  = _jsonBuffer.createNestedObject(_vaderLedStrip->getName());
  jsonVaderLedStrip["state"] = _vaderLedStrip->getState();
  jsonVaderLedStrip["color"] = Utils::getHEXColorStringFromCRGB(_vaderLedStrip->getColor());
  jsonVaderLedStrip["brightness"] = _vaderLedStrip->getBrightness();

  // Add MQTT status
  if (_mqttManager) {
    JsonObject jsonMqtt = _jsonBuffer.createNestedObject("mqtt");
    jsonMqtt["connected"] = _mqttManager->isConnected();
    jsonMqtt["broker"] = _mqttManager->getMqttBroker();
    jsonMqtt["port"] = _mqttManager->getMqttPort();
  }

  // Add WiFi status for debugging
  JsonObject jsonWifi = _jsonBuffer.createNestedObject("wifi");
  jsonWifi["status"] = WiFi.status();  // 0=OFF, 1=SCANNING, 2=FAILED, 3=NO_SSID, 4=CONNECTED, 6=DISCONNECTED
  jsonWifi["connected"] = (WiFi.status() == WL_CONNECTED);
  jsonWifi["mode"] = WiFi.getMode();  // 1=STA, 2=AP, 3=STA+AP
  jsonWifi["local_ip"] = WiFi.localIP().toString();
  jsonWifi["ap_ip"] = WiFi.softAPIP().toString();

  return _jsonBuffer;
}

void Endpoint::status(AsyncWebServerRequest *request) {
  Serial.println("Getting Status");
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(200);

  // Create a fresh JSON document for status response
  DynamicJsonDocument json(1024);

  JsonObject jsonLukeLedStrip  = json.createNestedObject(_lukeLedStrip->getName());
  jsonLukeLedStrip["state"] = _lukeLedStrip->getState();
  jsonLukeLedStrip["color"] = Utils::getHEXColorStringFromCRGB(_lukeLedStrip->getColor());
  jsonLukeLedStrip["brightness"] = _lukeLedStrip->getBrightness();

  JsonObject jsonVaderLedStrip  = json.createNestedObject(_vaderLedStrip->getName());
  jsonVaderLedStrip["state"] = _vaderLedStrip->getState();
  jsonVaderLedStrip["color"] = Utils::getHEXColorStringFromCRGB(_vaderLedStrip->getColor());
  jsonVaderLedStrip["brightness"] = _vaderLedStrip->getBrightness();

  // Add MQTT status
  if (_mqttManager) {
    JsonObject jsonMqtt = json.createNestedObject("mqtt");
    jsonMqtt["connected"] = _mqttManager->isConnected();
    jsonMqtt["broker"] = _mqttManager->getMqttBroker();
    jsonMqtt["port"] = _mqttManager->getMqttPort();
  }

  // Add WiFi status for debugging
  JsonObject jsonWifi = json.createNestedObject("wifi");
  jsonWifi["status"] = WiFi.status();  // 0=OFF, 1=SCANNING, 2=FAILED, 3=NO_SSID, 4=CONNECTED, 6=DISCONNECTED
  jsonWifi["connected"] = (WiFi.status() == WL_CONNECTED);
  jsonWifi["mode"] = WiFi.getMode();  // 1=STA, 2=AP, 3=STA+AP
  jsonWifi["local_ip"] = WiFi.localIP().toString();
  jsonWifi["ap_ip"] = WiFi.softAPIP().toString();

  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);

  serializeJson(json, *response);
  request->send(response);
}

void Endpoint::version(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(200);

  DynamicJsonDocument json(512);
  json["firmware"] = VERSION_STRING;
  json["build_date"] = BUILD_DATE_TIME;

  serializeJson(json, *response);
  request->send(response);
}

void Endpoint::_notFound(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(404);
  DynamicJsonDocument json(1024);
  json["message"] = "Not found";
  
  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);


  char   buffer[200]; // create temp buffer
  size_t len = serializeJson(json, buffer);

  serializeJson(json, *response);
  request->send(response);
}

void Endpoint::_error(AsyncWebServerRequest *request, const char *message)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(400);
  DynamicJsonDocument json(1024);
  json["message"] = message;

  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);

  char   buffer[200]; // create temp buffer
  size_t len = serializeJson(json, buffer);

  serializeJson(json, *response);
  request->send(response);
}

void Endpoint::_success(AsyncWebServerRequest *request, const char *message)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(200);
  DynamicJsonDocument json(1024);
  json["message"] = message;
  
  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);

  char   buffer[200]; // create temp buffer
  size_t len = serializeJson(json, buffer);

  serializeJson(json, *response);
  request->send(response);
}

void Endpoint::wifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
  const size_t capacity = len + 60;
  DynamicJsonDocument json(capacity);

  DeserializationError error = deserializeJson(json, (const char *)data, len);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    this->_error(request, "Error happen");
    return;
  }

  JsonObject object = json.as<JsonObject>();

  String SSID = object["ssid"];
  String PWD = object["pwd"];

  if (!SSID.isEmpty()) {
    this->_prefs->begin("wifi", false);
    this->_prefs->putString("SSID",SSID);
    this->_prefs->putString("PWD",PWD);
    this->_prefs->end();

    // Create response and send it before restarting
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->setCode(200);
    DynamicJsonDocument responseJson(512);
    responseJson["message"] = "WiFi configuration saved. Device will restart to connect to the new configured network. You can reconnect to your configured network. You will access the app through a new IP (you can find it in your router admin).";

    serializeJson(responseJson, *response);
    request->send(response);

    delay(1000);
    ESP.restart();
  } else {
    this->_error(request, "Network SSID is mandatory.");
  }
}

void Endpoint::networkConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len) {

  const size_t capacity = len + 60;
  DynamicJsonDocument json(capacity);

  DeserializationError error = deserializeJson(json, (const char *)data, len);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    this->_error(request, "Error happen");
    return;
  }

  JsonObject object = json.as<JsonObject>();

  String ip = object["ip"];
  String gateway = object["gateway"];
  String subnet = object["subnet"];

  if (!ip.isEmpty() && !gateway.isEmpty() && !subnet.isEmpty()) {
    this->_prefs->begin("network", false);
    this->_prefs->putString("IP", ip);
    this->_prefs->putString("Gateway", gateway);
    this->_prefs->putString("Subnet", subnet);
    this->_prefs->end();

    // Create response and send it before restarting
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->setCode(200);
    DynamicJsonDocument responseJson(512);
    responseJson["message"] = "Network configuration saved. Device will restart to apply the new static IP. You can navigate to the new address http://" + ip + " after the restart.";

    serializeJson(responseJson, *response);
    request->send(response);

    delay(1000);
    ESP.restart();
  } else {
    this->_error(request, "All fields are mandatory");
  }
}

void Endpoint::resetWifiConfig(AsyncWebServerRequest *request) {
    this->_prefs->begin("wifi", false);
    this->_prefs->putString("SSID", "");
    this->_prefs->putString("PWD", "");
    this->_prefs->end();

    // Create response and send it before restarting
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->setCode(200);
    DynamicJsonDocument responseJson(512);
    responseJson["message"] = "WiFi configuration has been reset. Device will restart in access point mode. You can connect to LightSaberLamp WiFi and navigate to http://192.168.1.1 to reconfigure WiFi.";

    serializeJson(responseJson, *response);
    request->send(response);

    delay(1000);
    ESP.restart();
}

void Endpoint::resetNetworkConfig(AsyncWebServerRequest *request) {
    this->_prefs->begin("network", false);
    this->_prefs->putString("IP", "");
    this->_prefs->putString("Gateway", "");
    this->_prefs->putString("Subnet", "");
    this->_prefs->end();

    // Create response and send it before restarting
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->setCode(200);
    DynamicJsonDocument responseJson(512);
    responseJson["message"] = "Network configuration has been reset. Device will restart and use DHCP to obtain an IP address. You can find the new IP in your router admin panel.";

    serializeJson(responseJson, *response);
    request->send(response);

    delay(1000);
    ESP.restart();
}

void Endpoint::initHotpsotRouting()
{
  _initStatics();
  _server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending hotspot.html");
    request->send(LittleFS, "/hotspot.html", "text/html");
  });
  _server->on("/wifi-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("Recieving POST with no data");
  }, NULL, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.println("Recieving POST with data");
    this->wifiConfig(request, data, len);
  });
}

void Endpoint::initClientRouting()
{
  _initStatics();
  _server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending index.html");
    request->send(LittleFS, "/index.html", "text/html");
  });
  _server->on("/status", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending status");
    this->status(request);
  });
  _server->on("/api/version", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending version");
    this->version(request);
  });
  _server->on("/version.json", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending version.json file");
    request->send(LittleFS, "/version.json", "application/json");
  });

  // Debug endpoint to check WiFi configuration
  _server->on("/debug/wifi", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Debug: Checking saved WiFi credentials");
    DynamicJsonDocument doc(512);

    _prefs->begin("wifi", true);
    String ssid = _prefs->getString("SSID", "");
    String pwd = _prefs->getString("PWD", "");
    _prefs->end();

    doc["ssid"] = ssid.isEmpty() ? "(not configured)" : ssid;
    doc["password"] = pwd.isEmpty() ? "(not configured)" : "***";
    doc["ssid_length"] = ssid.length();
    doc["pwd_length"] = pwd.length();
    doc["wifi_status_code"] = WiFi.status();
    doc["wifi_status_name"] = WiFi.status() == WL_CONNECTED ? "CONNECTED" : "NOT_CONNECTED";
    doc["wifi_mode"] = WiFi.getMode();
    doc["sta_ip"] = WiFi.localIP().toString();
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["rssi"] = WiFi.RSSI();  // Signal strength
    doc["is_actually_connected"] = (WiFi.localIP() != IPAddress(0, 0, 0, 0));

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJsonPretty(doc, *response);
    request->send(response);
  });

  _server->on("/network-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/network-settings: Recieving POST with no data");
  }, NULL, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.println("/network-settings: Recieving POST with data");
    this->networkConfig(request, data, len);
  });
  _server->on("/reset-network-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/reset-network-settings: Recieving POST with no data");
    this->resetNetworkConfig(request);
  });
  _server->on("/wifi-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/wifi-settings: Recieving POST with no data");
  }, NULL, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.println("/wifi-settings: Recieving POST with data");
    this->wifiConfig(request, data, len);
  });
  _server->on("/reset-wifi-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/reset-wifi-settings: Recieving POST with no data");
    this->resetWifiConfig(request);
  });
  _server->on("/mqtt-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/mqtt-settings: Recieving POST with no data");
  }, NULL, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.println("/mqtt-settings: Recieving POST with data");
    this->mqttConfig(request, data, len);
  });
  _server->on("/reset-mqtt-settings", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/reset-mqtt-settings: Recieving POST with no data");
    this->resetMqttConfig(request);
  });
  _server->on("/mqtt-discovery", HTTP_POST, [&](AsyncWebServerRequest *request){
    Serial.println("/mqtt-discovery: Republishing MQTT discovery messages");
    if (_mqttManager) {
      _mqttManager->republishDiscovery();
      this->_success(request, "MQTT discovery messages republished");
    } else {
      this->_error(request, "MQTT Manager not available");
    }
  });

}

void Endpoint::_initStatics()
{
  _server->on("/app.css", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending app.css");
    request->send(LittleFS, "/app.css", "text/css");
  });
  _server->on("/app.js", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending app.js");
    request->send(LittleFS, "/app.js", "text/javascript");
  });
  _server->on("/fa.icon.png", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending favicon.png");
    request->send(LittleFS, "/fa.icon.png", "image/png");
  });
  // Legacy favicon redirect for compatibility
  _server->on("/favicon.ico", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Redirecting favicon.ico to fa.icon.png");
    request->send(LittleFS, "/fa.icon.png", "image/png");
  });
}

// Internal function: broadcast via WebSocket only (no MQTT)
void Endpoint::_broadcastLedStatusWebsocketOnly() {
  DynamicJsonDocument json = this->_getFullStatusJson();

  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);

  // Use larger buffer to accommodate full status JSON with WiFi info
  // JSON includes: led states (luke/vader), colors, brightness, mqtt config, and wifi status
  char buffer[1024]; // Increased from 200 to 1024 bytes
  size_t len = serializeJson(json, buffer);
  _websocket->textAll(buffer, len);
}

// Public function: broadcast WebSocket AND publish MQTT
void Endpoint::wsBroadcastLedStatus() {
  _broadcastLedStatusWebsocketOnly();

  // Publish to MQTT (only once)
  if (_mqttManager) {
    _mqttManager->publishAllStatus();
  }
}

void Endpoint::wsMessageHandler(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    const size_t capacity = len + 60;
    DynamicJsonDocument message(capacity);
    DeserializationError error = deserializeJson(message, (const char *)data, len);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    JsonObject object = message.as<JsonObject>();

    JsonObject jsonLukeLedStrip  = object[_lukeLedStrip->getName()];
    if (jsonLukeLedStrip) {
      if (!jsonLukeLedStrip["state"].isNull()) {
        _lukeLedStrip->changeState(jsonLukeLedStrip["state"].as<bool>());
      }

      if (!jsonLukeLedStrip["color"].isNull()) {
        _lukeLedStrip->changeColor(jsonLukeLedStrip["color"].as<String>());
      }

      if (!jsonLukeLedStrip["brightness"].isNull()) {
        _lukeLedStrip->changeBrightness(jsonLukeLedStrip["brightness"].as<uint8_t>());
      }
    }

    JsonObject jsonVaderLedStrip  = object[_vaderLedStrip->getName()];
    if (jsonVaderLedStrip) {
      if (!jsonVaderLedStrip["state"].isNull()) {
        _vaderLedStrip->changeState(jsonVaderLedStrip["state"].as<bool>());
      }

      if (!jsonVaderLedStrip["color"].isNull()) {
        _vaderLedStrip->changeColor(jsonVaderLedStrip["color"].as<String>());
      }

      if (!jsonVaderLedStrip["brightness"].isNull()) {
        _vaderLedStrip->changeBrightness(jsonVaderLedStrip["brightness"].as<uint8_t>());
      }
    }

    // Broadcast update via WebSocket
    _broadcastLedStatusWebsocketOnly();

    // Publish to MQTT (only once)
    if (_mqttManager) {
      _mqttManager->publishAllStatus();
    }
  }
}

void Endpoint::mqttConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
  const size_t capacity = len + 60;
  DynamicJsonDocument json(capacity);

  DeserializationError error = deserializeJson(json, (const char *)data, len);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    this->_error(request, "Error parsing JSON");
    return;
  }

  JsonObject object = json.as<JsonObject>();

  // Support both field names: broker/mqtt_server and port/mqtt_port
  String broker = object["mqtt_server"].as<String>();
  if (broker.isEmpty()) {
    broker = object["broker"].as<String>();
  }

  uint16_t port = 1883;
  if (!object["mqtt_port"].isNull()) {
    port = object["mqtt_port"].as<uint16_t>();
  } else if (!object["port"].isNull()) {
    port = object["port"].as<uint16_t>();
  }

  String username = object["mqtt_user"].as<String>();
  String password = object["mqtt_pass"].as<String>();

  if (!broker.isEmpty()) {
    if (_mqttManager) {
      // Save broker, port, username and password
      _mqttManager->saveConfig(broker, port, username, password);

      if (!username.isEmpty() && !password.isEmpty()) {
        Serial.printf("MQTT credentials configured: user=%s\n", username.c_str());
      } else if (!username.isEmpty() || !password.isEmpty()) {
        Serial.println("Warning: Only one of username/password is set. Both are recommended.");
      } else {
        Serial.println("MQTT: Connecting without authentication (no username/password provided)");
      }
    }

    // Create response and send it before restarting
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->setCode(200);
    DynamicJsonDocument responseJson(256);
    responseJson["message"] = "MQTT configuration saved. Device will restart to reconnect to the new MQTT broker.";

    serializeJson(responseJson, *response);
    request->send(response);

    // Schedule restart asynchronously to allow response to be sent first
    delay(500);
    ESP.restart();
  } else {
    this->_error(request, "Broker address is mandatory.");
  }
}

void Endpoint::resetMqttConfig(AsyncWebServerRequest *request) {
  if (_mqttManager) {
    _mqttManager->resetConfig();
  }

  // Create response and send it before restarting
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(200);
  DynamicJsonDocument responseJson(256);
  responseJson["message"] = "MQTT configuration reset. Device will restart.";

  serializeJson(responseJson, *response);
  request->send(response);

  // Schedule restart with delay to allow response to be sent first
  delay(500);
  ESP.restart();
}

