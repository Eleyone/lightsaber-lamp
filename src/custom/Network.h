#ifndef Network_h
#define Network_h
#include <Arduino.h>
#include <Preferences.h>

class Network {
    Preferences *_prefs;
    public:
        explicit Network(Preferences *prefs);
        void initConnexion();
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
        void _initClientMode();
        void _getNetworkConfig();
        IPAddress _getIPAddressFromString(String ipString);
};
#endif