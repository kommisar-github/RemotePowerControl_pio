#ifndef SN_APPLICATION_H
#define SN_APPLICATION_H

#if ARDUINO >= 100
//#pragma message "ALEX: ARDUINO >= 100"
#include <Arduino.h>       // for delayMicroseconds, digitalPinToBitMask, etc
#else
//#pragma message "ALEX: ARDUINO < 100"
#include "WProgram.h"      // for delayMicroseconds
#include "pins_arduino.h"  // for digitalPinToBitMask, etc
#endif

#include "log.h"

// ESP8266  ESP32
// D1 mini  D1 mini
//   D0       26
//   D1       22
//   D2       21
//   D3       17
//   D4       16
//   D5       18
//   D6       19
//   D7       23
//   D8        5  

#if defined(ESP8266)
// ESP8266: D1 mini: 
//     For Relay use either D1/GPIO05 and D2/GPIO04. They are high impendance during reset and boot.
#define POWER_DETECT1_GPIO D7
#define POWER_SWITCH1_GPIO D1
#define POWER_DETECT2_GPIO D6
#define POWER_SWITCH2_GPIO D8
#define POWER_DETECT3_GPIO D5
#define POWER_SWITCH3_GPIO D2

#define TEST_WIFI_LED      D4
#define BUTTON_GPIO        D0
#endif
#if defined(ESP32)
// ESP32: D1 mini: 
#define POWER_DETECT1_GPIO 23
#define POWER_SWITCH1_GPIO 22
#define POWER_DETECT2_GPIO 19
#define POWER_SWITCH2_GPIO  5
#define POWER_DETECT3_GPIO 18
#define POWER_SWITCH3_GPIO 21

#define TEST_WIFI_LED      16
#define BUTTON_GPIO        26
#endif

#endif
