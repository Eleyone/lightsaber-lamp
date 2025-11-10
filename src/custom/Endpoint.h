#ifndef Endpoint_h
#define Endpoint_h
#include <Arduino.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Ledstrip.h"
#include "MqttManager.h"

class Endpoint {
    AsyncWebServer *_server;
    AsyncWebSocket *_websocket;
    Preferences *_prefs;
    Ledstrip *_lukeLedStrip;
    Ledstrip *_vaderLedStrip;
    MqttManager *_mqttManager;

    // JSON buffer pool (reusable to avoid repeated allocations)
    static const size_t JSON_BUFFER_SIZE = 1024;
    DynamicJsonDocument _jsonBuffer{JSON_BUFFER_SIZE};

    public:
        Endpoint(AsyncWebServer *server, AsyncWebSocket *websocket, Preferences *prefs, Ledstrip *lukeLedStrip, Ledstrip *vaderLedStrip, MqttManager *mqttManager);
        void wifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
        void networkConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
        void resetWifiConfig(AsyncWebServerRequest *request);
        void resetNetworkConfig(AsyncWebServerRequest *request);
        void mqttConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
        void resetMqttConfig(AsyncWebServerRequest *request);
        void status(AsyncWebServerRequest *request);
        void version(AsyncWebServerRequest *request);
        void initHotpsotRouting();
        void initClientRouting();
        void wsMessageHandler(void *arg, uint8_t *data, size_t len);
        void wsBroadcastLedStatus();
    private:
        void _initStatics();
        void _initWebSocket();
        void _notFound(AsyncWebServerRequest *request);
        void _error(AsyncWebServerRequest *request, const char *message);
        void _success(AsyncWebServerRequest *request, const char *message);
        DynamicJsonDocument _getFullStatusJson();
        void _broadcastLedStatusWebsocketOnly();  // Internal WS broadcast only
};
#endif