#ifndef Network_h
#define Network_h
#include <Arduino.h>
#include <Preferences.h>

class Network {
    Preferences *_prefs;
    unsigned long _lastWiFiCheckTime = 0;
    const unsigned long WIFI_CHECK_INTERVAL_MS = 10000;  // Check every 10 seconds
    int _wifiConnectionAttempts = 0;
    const int MAX_WIFI_CONNECTION_ATTEMPTS = 3;
    public:
        explicit Network(Preferences *prefs);
        void initConnexion();
        void loop();  // Background WiFi reconnection attempt
        bool isAccessPoint();
    private:
        String _apSSID = "LightSaberLamp";
        String _apPWD = "LightSaberLamp";
        String _clientSSID;
        String _clientPWD;
        IPAddress _clientIP;
        IPAddress _clientGateway;
        IPAddress _clientSubnet;
        bool _isAccessPointMode = true;
        bool _isConnected = false;
        void _initAccesPointMode();
        void _initClientMode(int attemptNumber = 1);
        void _getNetworkConfig();
        IPAddress _getIPAddressFromString(String ipString);
};
#endif