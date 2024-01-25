#ifndef Endpoint_h
#define Endpoint_h
#include <Arduino.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Ledstrip.h"

class Endpoint {
    AsyncWebServer *_server;
    AsyncWebSocket *_websocket;
    Preferences *_prefs;
    Ledstrip *_lukeLedStrip;
    Ledstrip *_vaderLedStrip;
    public:
        Endpoint(AsyncWebServer *server, AsyncWebSocket *websocket, Preferences *prefs, Ledstrip *lukeLedStrip, Ledstrip *vaderLedStrip);
        void wifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
        void networkConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
        void resetWifiConfig(AsyncWebServerRequest *request);
        void resetNetworkConfig(AsyncWebServerRequest *request);
        void status(AsyncWebServerRequest *request);
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
};
#endif