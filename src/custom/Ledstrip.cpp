#include <Arduino.h>
#include <FastLED.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Ledstrip.h"
#include "Utils.h"

Ledstrip::Ledstrip(const char *name, CRGB color) : _name(name),
                                                   _color(color),
                                                   _defaultColor(color)
{}

void Ledstrip::initLedstrip()
{
    _color = _defaultColor;
    _state = true;
    _prevState = false;
}

const char *Ledstrip::getName()
{
    return this->_name;
}

CRGB Ledstrip::getColor()
{
    return this->_color;
}

void Ledstrip::changeColor(String hexColorString)
{
    this->_color = Utils::getCRGBFromHEXColorString(hexColorString);
}

void Ledstrip::changeColor(CRGB newColor)
{
    this->_color = newColor;
}

bool Ledstrip::getState()
{
    return this->_state;
}

bool Ledstrip::getPreviousState()
{
    return this->_prevState;
}

void Ledstrip::changeState(bool newState)
{
    Serial.print("change state (");
    this->_prevState = this->_state;
    Serial.print(this->_prevState);
    Serial.print(" => ");
    this->_state = newState;
    Serial.print(this->_state);
    Serial.println(")");
}

bool Ledstrip::hasStateChanged()
{
    return this->_prevState != this->_state;
}

void Ledstrip::resetPreviousState()
{
    this->_prevState = this->_state;
}

uint8_t Ledstrip::getBrightness()
{
    return this->_brightness;
}

void Ledstrip::changeBrightness(uint8_t newBrightness)
{
    this->_brightness = newBrightness;
}