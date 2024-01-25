#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Ledstrip.h"
#include "Endpoint.h"
#include "Utils.h"

Endpoint::Endpoint(AsyncWebServer *server, AsyncWebSocket *websocket, Preferences *prefs, Ledstrip *lukeLedStrip, Ledstrip *vaderLedStrip):
  _prefs(prefs),
  _server(server),
  _lukeLedStrip(lukeLedStrip),
  _vaderLedStrip(vaderLedStrip),
  _websocket(websocket)
{}

DynamicJsonDocument Endpoint::_getFullStatusJson() {
  DynamicJsonDocument json(1024);

  JsonObject jsonLukeLedStrip  = json.createNestedObject(_lukeLedStrip->getName());
  jsonLukeLedStrip["state"] = _lukeLedStrip->getState();
  jsonLukeLedStrip["color"] = Utils::getHEXColorStringFromCRGB(_lukeLedStrip->getColor());
  jsonLukeLedStrip["brightness"] = _lukeLedStrip->getBrightness();

  JsonObject jsonVaderLedStrip  = json.createNestedObject(_vaderLedStrip->getName());
  jsonVaderLedStrip["state"] = _vaderLedStrip->getState();
  jsonVaderLedStrip["color"] = Utils::getHEXColorStringFromCRGB(_vaderLedStrip->getColor());
  jsonVaderLedStrip["brightness"] = _vaderLedStrip->getBrightness();

  return json;
}

void Endpoint::status(AsyncWebServerRequest *request) {
  Serial.println("Getting Status");
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(200);

  DynamicJsonDocument json = this->_getFullStatusJson();

  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);

  serializeJson(json, *response);

  char   buffer[200]; // create temp buffer
  size_t len = serializeJson(json, buffer);

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

    this->_success(request, "Wifi configuration saved. Boardgame table will restart to connect to the new configured network. You can leave this page and reconnect to your configured network. You will acces to your app throug a new IP. (you can find it in your router admin)");

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

    String message("Network configuration saved. Boardgame table will restart to connect with the new stati IP set. You can leave this page and navigate to the new url http://" + ip + ".");

    this->_success(request, message.c_str());

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

    this->_success(request, "Wifi configuration has been reseted. Boardgame table will restart in acces point mode. You can connect to the default wif BoardgameTable, and navigate to http://192.168.1.1 to acces the wifi-settings.");

    delay(1000);
    ESP.restart();
}

void Endpoint::resetNetworkConfig(AsyncWebServerRequest *request) {
    this->_prefs->begin("network", false);
    this->_prefs->putString("IP", "");
    this->_prefs->putString("Gateway", "");
    this->_prefs->putString("Subnet", "");
    this->_prefs->end();

    this->_success(request, "Network configuration has been reseted. Boardgame table will restart to connect to the configured network. You will acces to your app throug a new IP. (you can find it in your router admin)");

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
  _server->on("/app.css.map", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending app.css.map");
    request->send(LittleFS, "/app.css.map", "text/plain");
  });
  _server->on("/app.js.map", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending app.js.map");
    request->send(LittleFS, "/app.js.map", "text/plain");
  });
  _server->on("/favicon.ico", HTTP_GET, [&](AsyncWebServerRequest *request){
    Serial.println("Sending favicon.ico");
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
  });
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

    Serial.println("Sending status json :");
    serializeJsonPretty(message, Serial);

    char   buffer[200]; // create temp buffer
    size_t len = serializeJson(message, buffer);
    _websocket->textAll(buffer, len);
  }
}

void Endpoint::wsBroadcastLedStatus() {
  DynamicJsonDocument json = this->_getFullStatusJson();

  Serial.println("Sending status json :");
  serializeJsonPretty(json, Serial);

  char   buffer[200]; // create temp buffer
  size_t len = serializeJson(json, buffer);
  _websocket->textAll(buffer, len);
}