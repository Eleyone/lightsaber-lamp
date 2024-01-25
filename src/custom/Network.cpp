#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "Network.h"

Network::Network(Preferences *prefs):
    _prefs(prefs)
{}

void Network::initConnexion() {
    _getNetworkConfig();
    
    if (!_isAccessPointMode) {
        _initClientMode();
    }
    if (!_isConnected) {
        _initAccesPointMode();
    }
}

bool Network::isAccessPoint() {
    return _isAccessPointMode;
}

void Network::_initAccesPointMode() {
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

void Network::_initClientMode() {
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

    WiFi.begin(_clientSSID.c_str(), _clientPWD.c_str()); 
    for (int i = 0; i < 150; i++) {
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
}

void Network::_getNetworkConfig() {
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

IPAddress Network::_getIPAddressFromString(String ipString) {
    IPAddress ipAddress;
    if (!ipAddress.fromString(ipString.c_str())) {
        Serial.print("Enable to parse to an actual IP address: ");
        Serial.println(ipString);
    }
    return ipAddress;
}