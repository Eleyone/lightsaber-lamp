#ifndef Utils_h
#define Utils_h
#include <Arduino.h>
#include <FastLED.h>

class Utils
{
    public:
        static CRGB getCRGBFromHEXColorString(String color);
        static String getHEXColorStringFromCRGB(CRGB color);
};
#endif