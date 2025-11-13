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
    CRGB _prevColor;  // Track previous color for change detection
    bool _state = true;
    bool _prevState = false;
    uint8_t _brightness = 255;
    uint8_t _prevBrightness = 255;  // Track previous brightness for change detection
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
        void changeColor(CRGB newColor);
        bool hasColorChanged();
        void resetColorChange();
        uint8_t getBrightness();
        void changeBrightness(uint8_t newBrightness);
        bool hasBrightnessChanged();
        void resetBrightnessChange();
        void initLedstrip();
};
#endif