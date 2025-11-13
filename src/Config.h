
#ifndef Config_h
#define Config_h

/**-- VERSION MANAGEMENT --*/
#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_PATCH 8

// Build date and time captured at compile time
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define BUILD_DATE_TIME BUILD_DATE " " BUILD_TIME

// Construct semantic version string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERSION_STRING TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)

/**-- Start pin list --*/
/* led strip data pin */
#define LUKE_LED_PIN GPIO_NUM_18
#define VADER_LED_PIN GPIO_NUM_21

/* input pin */
#define LUKE_SWITCH_PIN GPIO_NUM_33
#define VADER_SWITCH_PIN GPIO_NUM_32

/* Speaker pin */
#define LUKE_SPEAKER_PIN GPIO_NUM_4
#define VADER_SPEAKER_PIN GPIO_NUM_16
/**-- End pin list --*/

#define LED_TYPE WS2811
#define COLOR_ORDER BRG // RGB

#define COLOR_LUKE CRGB::Blue
#define COLOR_VADER CRGB::Red

#define NB_LED_SABER 36

#define BUTTON_DEBOUNCE_DELAY 20 

// defini des enumeration pour les selecteur d'action type get ou set
enum saber
{
    LUKE = 0,
    VADER = 1
};

#endif
