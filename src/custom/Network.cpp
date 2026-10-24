#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "Network.h"

// Log AP client activity: helps diagnose devices that can't join the hotspot
static void _onApEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.printf("AP: client " MACSTR " associated\n", MAC2STR(info.wifi_ap_staconnected.mac));
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.printf("AP: client " MACSTR " left (reason %d)\n", MAC2STR(info.wifi_ap_stadisconnected.mac), info.wifi_ap_stadisconnected.reason);
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.printf("AP: client got IP " IPSTR "\n", IP2STR(&info.wifi_ap_staipassigned.ip));
            break;
        default:
            break;
    }
}

WifiNetwork::WifiNetwork(Preferences *prefs):
    _prefs(prefs)
{}

void WifiNetwork::initConnexion() {
    _getNetworkConfig();
    // Credentials live in Preferences: keep the WiFi driver profiles in RAM so
    // they are rebuilt at every boot instead of being reloaded from NVS
    WiFi.persistent(false);
    WiFi.onEvent(_onApEvent);

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
            return;
        }
        Serial.println("\nWiFi connection failed at startup. Will keep retrying in background...");
        _stopStation();
    }

    // The AP is only a fallback: started when there is no home network to join,
    // stopped by loop() as soon as the station connects
    _initAccesPointMode();
}

bool WifiNetwork::isAccessPoint() {
    return _isAccessPointMode;
}

void WifiNetwork::loop() {
    // Connected to the home network: the fallback AP is no longer needed
    if (WiFi.status() == WL_CONNECTED) {
        _retryInProgress = false;
        if (WiFi.getMode() & WIFI_MODE_AP) {
            Serial.println("NETWORK: WiFi connected, stopping fallback AP");
            WiFi.softAPdisconnect(true);
        }
        return;
    }

    // If no credentials configured, don't attempt to connect
    if (_clientSSID.isEmpty()) {
        return;
    }

    // AP and STA share the radio: while the station looks for its network it
    // scans every channel and the AP beacons become intermittent (AP unusable).
    // So the station only runs during a bounded retry window, AP-only otherwise.
    unsigned long now = millis();
    if (_retryInProgress) {
        if (now - _retryStartedAt < WIFI_RETRY_WINDOW_MS) {
            return;
        }
        Serial.println("NETWORK: WiFi retry timed out, back to AP only");
        _stopStation();
        return;
    }

    // Connection lost outside a retry window: stop the station auto-reconnect
    // scans and bring the fallback AP back
    if (WiFi.getMode() & WIFI_MODE_STA) {
        Serial.println("NETWORK: WiFi connection lost, starting fallback AP");
        _stopStation();
        _initAccesPointMode();
        return;
    }

    if (now - _lastWiFiRetryTime < WIFI_RETRY_INTERVAL_MS) {
        return;
    }

    _lastWiFiRetryTime = now;

    // Don't retry while someone is connected to the AP: the scan would drop them
    if (WiFi.softAPgetStationNum() > 0) {
        Serial.println("NETWORK: AP has clients, skipping WiFi retry");
        return;
    }

    Serial.println("NETWORK: Retrying WiFi connection...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(_clientSSID.c_str(), _clientPWD.c_str());
    _retryInProgress = true;
    _retryStartedAt = now;
}

void WifiNetwork::_stopStation() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    _retryInProgress = false;
    _lastWiFiRetryTime = millis();
}

void WifiNetwork::_initAccesPointMode() {
    Serial.println("Dropping into AP mode for configuration.");

    IPAddress apIP(192, 168, 1, 1);
    IPAddress apGateway(192, 168, 1, 1);
    IPAddress apSubnet(255, 255, 255, 0);

    // softAP() skips esp_wifi_set_config() when SSID/password/auth match the
    // profile the driver already holds (loaded from NVS at init), so a stale
    // stored AP profile would never be rewritten. Clearing it first forces the
    // driver to apply the WPA2 profile again (PMK derived from the passphrase).
    WiFi.AP.clear();
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