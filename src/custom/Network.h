#ifndef Network_h
#define Network_h
#include <Arduino.h>
#include <Preferences.h>

class WifiNetwork {
    Preferences *_prefs;
    unsigned long _lastWiFiRetryTime = 0;
    unsigned long _retryStartedAt = 0;
    bool _retryInProgress = false;
    const unsigned long WIFI_RETRY_INTERVAL_MS = 120000;  // Retry home WiFi every 2 minutes
    const unsigned long WIFI_RETRY_WINDOW_MS = 15000;     // Give each retry 15 seconds
    int _wifiConnectionAttempts = 0;
    const int MAX_WIFI_CONNECTION_ATTEMPTS = 3;
    public:
        explicit WifiNetwork(Preferences *prefs);
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
        void _stopStation();
        IPAddress _getIPAddressFromString(String ipString);
};
#endif