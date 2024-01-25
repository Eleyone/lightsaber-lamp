#include <Arduino.h>
#include <FastLED.h>
#include "Utils.h"

CRGB Utils::getCRGBFromHEXColorString(String color)
{
    /** parsing HEX string at format #FFFFFF send via HTTP */
    char red[4];
    color.substring(1, 3).toCharArray(red, 4);

    char green[4];
    color.substring(3, 5).toCharArray(green, 4);

    char blue[4];
    color.substring(5, 7).toCharArray(blue, 4);

    return CRGB((uint8_t)strtoul(red, 0, 16), (uint8_t)strtoul(green, 0, 16), (uint8_t)strtoul(blue, 0, 16));
}

String Utils::getHEXColorStringFromCRGB(CRGB color)
{
    String hexColorString = "#";

    long red = (long)color.r;
    if (red == 0) {
        hexColorString.concat("00");
    } else {
        String temp(red, HEX);
        hexColorString.concat(temp);
    }

    long green = (long)color.g;
    if (green == 0) {
        hexColorString.concat("00");
    } else {
        String temp(green, HEX);
        hexColorString.concat(temp);
    }

    long blue = (long)color.b;
    if (blue == 0) {
        hexColorString.concat("00");
    } else {
        String temp(blue, HEX);
        hexColorString.concat(temp);
    }
    
    return hexColorString;
}