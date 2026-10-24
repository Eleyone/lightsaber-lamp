#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "Network.h"

WifiNetwork::WifiNetwork(Preferences *prefs):
    _prefs(prefs)
{}

void WifiNetwork::initConnexion() {
    _getNetworkConfig();

    if (!_isAccessPointMode) {
        // Credentials are configured, try to connect multiple times quickly at startup
        _wifiConnectionAttempts = 0;
        while (_wifiConnectionAttempts < MAX_WIFI_CONNECTION_ATTEMPTS && !_isConnected) {
            _wifiConnectionAttempts++;
            Serial.printf("\nWiFi connection attempt %d/%d (startup phase)\n", _wifiConnectionAttempts, MAX_WIFI_CONNECTION_ATTEMPTS);
            _initClientMode(_wifiConnectionAttempts);

            // If failed, wait a bit before retrying
            if (!_isConnected && _wifiConnectionAttempts < MAX_WIFI_CONNECTION_ATTEMPTS) {
                Serial.println("WiFi connection failed, waiting before retry...");
                delay(1000);  // Wait 1 second before next attempt
            }
        }

        if (_isConnected) {
            Serial.println("\n✓ WiFi connected successfully at startup!");
        } else {
            Serial.println("\nWiFi connection failed at startup. Will keep retrying in background...");
        }
    }

    // Always start AP mode for accessibility and recovery
    // Device will be in dual mode (AP+STA) to allow both:
    // 1. Access via AP while waiting for WiFi connection
    // 2. Automatic WiFi connection attempts in background via loop()
    _initAccesPointMode();
}

bool WifiNetwork::isAccessPoint() {
    return _isAccessPointMode;
}

void WifiNetwork::loop() {
    // If already connected, nothing to do
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    // If no credentials configured, don't attempt to connect
    if (_clientSSID.isEmpty()) {
        return;
    }

    // Check if it's time to attempt reconnection (every 10 seconds)
    unsigned long now = millis();
    if (now - _lastWiFiCheckTime < WIFI_CHECK_INTERVAL_MS) {
        return;
    }

    _lastWiFiCheckTime = now;

    wifi_mode_t currentMode = WiFi.getMode();

    // AP and STA share the radio: WiFi.begin() scans and moves the AP channel,
    // dropping its clients. Don't retry while someone is connected to the AP.
    if ((currentMode == WIFI_AP || currentMode == WIFI_AP_STA) && WiFi.softAPgetStationNum() > 0) {
        Serial.println("NETWORK: AP has clients, skipping WiFi retry");
        return;
    }

    // If in AP-only mode, switch to dual mode and try to connect
    if (currentMode == WIFI_AP) {
        Serial.println("NETWORK: WiFi credentials available. Switching to AP+STA dual mode...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(_clientSSID.c_str(), _clientPWD.c_str());
    }
    // If in STA or dual mode but not connected, just retry the connection
    else if (currentMode == WIFI_STA || currentMode == WIFI_AP_STA) {
        Serial.println("NETWORK: Retrying WiFi connection...");
        WiFi.begin(_clientSSID.c_str(), _clientPWD.c_str());
    }
}

void WifiNetwork::_initAccesPointMode() {
    Serial.println("Dropping into AP mode for configuration.");

    IPAddress apIP(192, 168, 1, 1);
    IPAddress apGateway(192, 168, 1, 1);
    IPAddress apSubnet(255, 255, 255, 0);

    WiFi.softAP(_apSSID, _apPWD);
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    delay(100);

    Serial.print("AP WiFi SSID: ");
    Serial.println(_apSSID);
    Serial.print("AP WiFi Password: ");
    Serial.println(_apPWD);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
}

void WifiNetwork::_initClientMode(int attemptNumber) {
    if (!_clientIP.toString().isEmpty()) {
        Serial.print("Set static IP address: ");
        Serial.print(_clientIP);
        Serial.print(", gateway: ");
        Serial.print(_clientGateway);
        Serial.print(", subnet: ");
        Serial.println(_clientSubnet);
        if (!WiFi.config(_clientIP, _clientGateway, _clientSubnet)) {
            Serial.println("Failed to configure static address");
        }
    }

    Serial.print("Try to connect to WiFi network : ");
    Serial.print(_clientSSID);
    Serial.println("(Note: Network may be hidden/not visible in scans)");

    // Check current WiFi mode
    wifi_mode_t currentMode = WiFi.getMode();
    if (currentMode == WIFI_AP) {
        // Already in AP mode, switch to dual mode (AP+STA)
        WiFi.mode(WIFI_AP_STA);
    } else if (currentMode == WIFI_OFF) {
        // Not initialized yet, start STA mode
        WiFi.mode(WIFI_STA);
    }
    // If already in STA or AP_STA mode, just use begin() without changing mode

    // For hidden networks: pass empty BSSID to force connection attempt
    // even if SSID is not visible in scan. WiFi.begin() with just SSID+password
    // works for hidden networks as long as you know the exact SSID
    WiFi.begin(_clientSSID.c_str(), _clientPWD.c_str(), 0, nullptr, true);

    // Use consistent 15-second timeout for startup attempts
    // Background retries in loop() will be more frequent
    int timeoutIterations = 150;  // 15 seconds per attempt
    Serial.printf("Waiting up to %d seconds for WiFi connection...\n", timeoutIterations / 10);

    for (int i = 0; i < timeoutIterations; i++) {
        Serial.print(".");
        delay(100);
        if (WiFi.status() == WL_CONNECTED) {
            _isConnected = true;
            Serial.println();
            Serial.print("Connected! IP Address: ");
            Serial.println(WiFi.localIP());
            break;
        }
    }

    if (!_isConnected) {
        Serial.println();
        if (attemptNumber < MAX_WIFI_CONNECTION_ATTEMPTS) {
            Serial.println("WiFi connection timeout - will retry...");
        } else {
            Serial.println("WiFi connection failed after all attempts - staying in AP+STA mode for background retry");
        }
    }
}

void WifiNetwork::_getNetworkConfig() {
    _prefs->begin("wifi");
    _clientSSID = _prefs->getString("SSID", "");
    _clientPWD = _prefs->getString("PWD", "");
    _prefs->end();

    _prefs->begin("network");
    String configIP = _prefs->getString("IP", "");
    String configGateway = _prefs->getString("Gateway", "");
    String configSubnet = _prefs->getString("Subnet", "");
    _prefs->end();

    _isAccessPointMode = (_clientSSID.isEmpty());

    if (!configIP.isEmpty()) {
        _clientIP = _getIPAddressFromString(configIP);
    }
    if (!configGateway.isEmpty()) {
        _clientGateway = _getIPAddressFromString(configGateway);
    }
    if (!configSubnet.isEmpty()) {
        _clientSubnet = _getIPAddressFromString(configSubnet);
    }
}

IPAddress WifiNetwork::_getIPAddressFromString(String ipString) {
    IPAddress ipAddress;
    if (!ipAddress.fromString(ipString.c_str())) {
        Serial.print("Enable to parse to an actual IP address: ");
        Serial.println(ipString);
    }
    return ipAddress;
}