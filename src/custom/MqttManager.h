#ifndef MqttManager_h
#define MqttManager_h

#include <Arduino.h>
#include <Preferences.h>

// IMPORTANT: Must be defined BEFORE including PubSubClient.h
// Default is 256 bytes, but discovery payloads are ~630 bytes
#define MQTT_MAX_PACKET_SIZE 1024

#include <PubSubClient.h>
#include <WiFiClient.h>
#include "Ledstrip.h"

// Callback function type for when MQTT messages change LED state
typedef void (*MqttStateChangeCallback)();

class MqttManager {
  private:
    Preferences *_prefs;
    Ledstrip *_lukeLedStrip;
    Ledstrip *_vaderLedStrip;

    WiFiClient espClient;
    PubSubClient *_mqttClient;

    String _mqttBroker;
    uint16_t _mqttPort;
    String _mqttClientId;
    String _mqttUsername;
    String _mqttPassword;
    bool _isInitialized;
    bool _isConnected;
    unsigned long _lastReconnectAttempt = 0;
    const unsigned long RECONNECT_INTERVAL_MS = 5000;  // 5 seconds

    // Callback for LED state changes from MQTT
    MqttStateChangeCallback _onStateChangeCallback = nullptr;

    // Helper methods
    void _setupMqtt();
    void _publishDiscovery();
    void _publishDiscoveryForSaber(const char* saberName, const char* saberId, const char* saberPath);
    void _handleMessage(char *topic, byte *payload, unsigned int length);

    // Static callback for MQTT
    static MqttManager *_instance;
    static void _mqttCallback(char *topic, byte *payload, unsigned int length);

  public:
    MqttManager(Preferences *prefs, Ledstrip *lukeLedStrip, Ledstrip *vaderLedStrip);
    ~MqttManager();

    // Lifecycle
    void init();
    void loop();
    bool isConnected() { return _isConnected; }

    // Config
    void loadConfig();
    void saveConfig(String broker, uint16_t port, String username = "", String password = "");
    void resetConfig();

    // Publishing
    void publishLukeStatus();
    void publishVaderStatus();
    void publishAllStatus();
    void republishDiscovery();  // Force re-publish discovery messages

    // Getters
    String getMqttBroker() { return _mqttBroker; }
    uint16_t getMqttPort() { return _mqttPort; }
    String getMqttUsername() { return _mqttUsername; }
    String getMqttPassword() { return _mqttPassword; }
    bool isInitialized() { return _isInitialized; }

    // Callback registration for state changes
    void setOnStateChangeCallback(MqttStateChangeCallback callback) {
      _onStateChangeCallback = callback;
    }
};

#endif
