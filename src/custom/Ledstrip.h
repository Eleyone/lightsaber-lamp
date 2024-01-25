#ifndef Ledstrip_h
#define Ledstrip_h
#include <Arduino.h>
#include <FastLED.h>
#include <Preferences.h>
#include "Config.h"

class Ledstrip {
    const char* _name;
    CRGB _color;
    CRGB _defaultColor;
    bool _state = true;
    bool _prevState = false;
    uint8_t _brightness = 255;
    public:
        Ledstrip(const char* name, CRGB color);
        const char* getName();
        bool getState();
        bool getPreviousState();
        void changeState(bool newState);
        bool hasStateChanged();
        void resetPreviousState();
        CRGB getColor();
        void changeColor(String hexColorString);
        uint8_t getBrightness();
        void changeBrightness(uint8_t newBrightness);
        void initLedstrip();
};
#endif