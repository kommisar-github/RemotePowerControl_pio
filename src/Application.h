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

#include "Log.h"

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
//   SWITCH1=D1/GPIO5, SWITCH3=D2/GPIO4 — high-impedance at boot (boot-safe for active-HIGH relay).
//   SWITCH2=D8/GPIO15 — strapping pin with external pulldown; held LOW at boot (not high-impedance,
//   but safe for active-HIGH relay: LOW at boot = relay off). Do NOT use active-LOW relay on D8.
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
//   SWITCH1=GPIO22, SWITCH3=GPIO21 — not strapping pins; high-impedance at boot (boot-safe).
//   SWITCH2=GPIO13 — not a strapping pin; high-impedance at boot (boot-safe for active-HIGH relay).
//   (GPIO5 was used previously but is an ESP32 strapping pin pulled HIGH at boot — relay fires on reset.)
#define POWER_DETECT1_GPIO 23
#define POWER_SWITCH1_GPIO 22
#define POWER_DETECT2_GPIO 19
#define POWER_SWITCH2_GPIO 13
#define POWER_DETECT3_GPIO 18
#define POWER_SWITCH3_GPIO 21

#define TEST_WIFI_LED      16
#define BUTTON_GPIO        26
#endif

#endif
