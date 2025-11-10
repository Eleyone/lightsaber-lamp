#ifndef MqttManager_h
#define MqttManager_h

#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "Ledstrip.h"

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

    // Getters
    String getMqttBroker() { return _mqttBroker; }
    uint16_t getMqttPort() { return _mqttPort; }
    String getMqttUsername() { return _mqttUsername; }
    String getMqttPassword() { return _mqttPassword; }
    bool isInitialized() { return _isInitialized; }
};

#endif
