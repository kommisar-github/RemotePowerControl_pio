---
description: Build / platform / pin maps / main.cpp shell. Canonical owner of secrets, boot-safe pin map, version-bump. Never inline a credential.
globs: platformio.ini, src/Application.h, src/main.cpp, src/SerialDebug.cpp, src/SerialDebug.h, src/Log.cpp, src/Log.h
alwaysApply: false
---

# /firmware — Build/integration rules (condensed)

Full domain knowledge: `.claude/skills/firmware/SKILL.md`. This is the quick guardrail.

## Owns
- `platformio.ini`, `src/Application.h`, `src/SerialDebug.*`, `src/Log.*`, `BUILD.md`, and the `main.cpp` **shell** (setup/loop/send_info/debug wiring — the MQTT section is `/mqtt`'s, power callbacks are `/power`'s, C6).

## NEVER touches
- Module *logic* in `RSwitch`/`MqttClient`/`WiFiConnect`/`WebSServer` (their specialists). MQTT topic `#define`s and power-callback bodies in `main.cpp` (C6).

## Key pitfalls / invariants (canonical owner)
- **Secrets invariant:** never inline a credential. All via `sysenv` → `SMARTHOME_WIFI_SSID/PWD`, `SMARTHOME_MQTT_BROKER`, `SMARTHOME_OTA_PASSWD` → `-D _WIFI_SSID_`/`_WIFI_PWD_`/`_MQTT_BROKER_`/`_OTA_PASSWD_`. `build_flags_home`/`build_flags_office` toggle network.
- **Boot-safe-GPIO invariant:** relay/switch output pins high-impedance through reset or the relay fires on flash. ESP8266 SWITCH=D1/D8/D2, DETECT=D7/D6/D5, BUTTON=D0, LED=D4. ESP32 SWITCH=22/5/21, DETECT=23/19/18, BUTTON=26, LED=16. Coordinate electrical rationale with `/power`.
- **`setup()` init order is load-bearing** (wires callbacks): test→Serial→time/SNTP→WiFi→MQTT(+topics+cb)→RSwitch[3]→OTA→web→serial-debug. Don't shuffle.
- **Version-bump protocol (canonical owner):** `APP_VER` in `main.cpp` + per-module header-comment dates on release; `/scm` tags after.
- Identity flags `_HOST_NAME_`=`rempowercntl1`, `_AREA_`=`home` drive topics+OTA+mDNS.
- Envs: `RELEASE_OTA_esp8266` (default espota), `RELEASE_COM_esp8266`/`TESTING_COM_esp8266` (COM5/6), `RELEASE_COM_esp32dv`. Boards `d1_mini`/`wemos_d1_mini32`.
- Lib pins: ArduinoJson `~6.21.2` (6.x API — 7.x is breaking), PubSubClient `~2.8`.
- SNTP differs per MCU; both `#ifdef` branches must stay in sync. Non-blocking-loop invariant in `loop()`.
