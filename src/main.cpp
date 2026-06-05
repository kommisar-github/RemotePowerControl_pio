/*
  RemotePowerControl
  
    > MQTT supported
    > Simple WEB Server supported
    > MDNS supported: http://<hostname>.local/
    > OTA (Sketch update over WiFi) supported
    
    Note: For switch/relay use D1/GPIO05 and D2/GPIO04 on ESP8266. Both are high impendance 
    during reset and boot. Thus they will not get high and do not impact switch/relay on restart/flash

  Versions:
    v1.00 20231212 - Initial version
    v1.01 20241012 - Add "mc" field
*/

#if defined(ESP8266)
#include <coredecls.h>                  // settimeofday_cb()
#include <TZ.h>
#endif
#if defined(ESP32)
#include <lwip/apps/sntp.h>
#include "esp_sntp.h"
#endif

#include <time.h>                       // time() ctime()
#include <ArduinoJson.h>                // JSON parser
#include "Application.h"
#include "WebSServer.h"
#include "WiFiConnect.h"
#include "MqttClient.h"
#include "OTA.h"
#include "Test.h"
#include "SerialDebug.h"
#include "RSwitch.h"

// === main.cpp shared-file ownership (BOOTSTRAP_PLAN §7 C6) ===
//   Default owner of this file is /firmware (integration shell: includes, globals,
//   setup(), loop(), send_info(), SNTP, serial-debug wiring, restart, Test/keepalive wiring).
//   Regions banner-marked below are owned by /mqtt (topic schema + inbound callback, C1/C3)
//   and /power (status-change -> publish, C2; refresh-all, C4). Edit only inside your owned
//   region and cite it in the dispatch payload; cross-boundary changes go through /pm.
//   Ref: doc/BOOTSTRAP_PLAN/07_interface_contracts.md
// === end ownership note ===

const char* APP_NAME PROGMEM = "RemotePowerControl";
const char* APP_VER  PROGMEM = "1.0.0 build 20260606";

#define MYTZ TZ_Asia_Jerusalem
static time_t boottime = 0L;
int timeset_cnt = 0;

// WiFi, OTA
const char* SSID = _WIFI_SSID_ ;
const char* PWD  = _WIFI_PWD_;
const char* HOST  = _HOST_NAME_;
const char* OTA_PASSWD  = _OTA_PASSWD_;

// MQTT
const char* mqtt_server = _MQTT_BROKER_;
const char* clientId = _HOST_NAME_;

// === /mqtt section — MQTT topic schema + topic list (C1) — owner: /mqtt ===
const char *announcementTopic = _AREA_ "/announcement/" _HOST_NAME_;
const char *lastWillTopic     = _AREA_ "/announcement/" _HOST_NAME_;
#define STR_KEEPALIVE           _AREA_ "/keepalive/" _HOST_NAME_
#define STR_ALIVE               _AREA_ "/alive"
#define STR_ALIVE_GET           _AREA_ "/alive/get"
#define STR_INFO                _AREA_ "/" _HOST_NAME_ "/info"
#define STR_INFO_GET            _AREA_ "/" _HOST_NAME_ "/info/get"
#define STR_CONNECTION_INFO     _AREA_ "/" _HOST_NAME_ "/connection/info"
#define STR_POWER_STATE         _AREA_ "/" _HOST_NAME_ "/power/state/"   // REtained: yes, topic: STR_POWER_STATE   + <switch_idx> Payload: <0|1>  - OFF|ON status only
#define STR_POWER_STATUS        _AREA_ "/" _HOST_NAME_ "/power/status/"  // REtained: yes, topic: STR_POWER_STATUS  + <switch_idx> Payload: <power_status_string>
#define STR_POWER_GET           _AREA_ "/" _HOST_NAME_ "/power/get"      // REtained: no  Payload: <switch_idx>
#define STR_POWER_SET           _AREA_ "/" _HOST_NAME_ "/power/set"      // Retained: no  Payload: <0|1|3> <switch_idx>

#define TOPIC_COUNT 4
const char *topics[] = {
  STR_ALIVE_GET,
  STR_INFO_GET,
  STR_POWER_GET,
  STR_POWER_SET
};
// === end /mqtt section ===

// JSON and temp buffers
DynamicJsonDocument doc(1024); // JSON
#define BUFFER_SIZE  (1024)
char BUF[BUFFER_SIZE];

void send_status();

// Callbacks:
void mqtt_received_callback(char* topic, byte* payload, unsigned int length);
void test_change_event_callback(uint8_t event);
void keepAlive_event_callback();
void power_status_change_callback(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status);

#if defined(ESP8266)
void settime_cb() {
  //Serial.printf("\n--- settimeofday() has been called - possibly from SNTP\n");
  if (boottime == 0L) {
    boottime = time(nullptr) - millis()/1000;
  }
  timeset_cnt++;
}
#endif
#if defined(ESP32)
void settime_cb(struct timeval *tv) {
  if (boottime == 0L) {
    time_t bt_new = time(nullptr) - millis()/1000;
    Serial.printf("\n--- settimeofday()  called. Boot time initial: %d new: %d\n", boottime, bt_new);
    boottime = bt_new;
  }
  else {
      Serial.printf("\n--- settimeofday()  called. Boot time: %d\n", boottime);
  }
  timeset_cnt++;
}
#endif

RSwitch **rSwitch;


// === /power section — status-change -> MQTT publish (C2) + refresh-all (C4) — owner: /power ===
void power_status_change(int switch_idx, RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status)  {
  char switch_idx_char = '1' + switch_idx;
  sprintf(BUF, "%s", src->decodePowerStat(new_status));
  char *topic = &BUF[0] + strlen(BUF) + 1;
  sprintf(topic, "%s%c", STR_POWER_STATUS, switch_idx_char);
  mqttClient.publish(topic, BUF, true);
  topic = &BUF[0];
  sprintf(topic, "%s%c", STR_POWER_STATE, switch_idx_char);
  switch (new_status) {
    case POWER_OFF:
      mqttClient.publish(topic, "0", true);
      break;
    case POWER_ON:
      mqttClient.publish(topic, "1", true);
      break;
    case SLEEP:
      mqttClient.publish(topic, "0", true);
      break;
    case POWERING_OFF:
      //mqttClient.publish(topic, "0", true);
      break;
    case POWERING_ON:
      //mqttClient.publish(topic, "1", true);
      break;
    default:
      //mqttClient.publish(topic, "0", true);
      break;
  }
}

void power_status_change_callback(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status) {
  for (int i=0; i<3; i++) {
    if (rSwitch[i] == src) {
      Serial.printf("## power_status_change_callback for swith #%d\n", i+1);
      power_status_change(i, src, new_status, old_status);
    }
  }
}

void refresh_all_switches_state() {
  for (int i=0; i<3; i++) {
    if (rSwitch[i] != NULL) {
        Serial.printf("## Refresh power_status for swith #%d\n", i+1);
        POWER_STATUS status = rSwitch[i]->getPowerStatus();
        power_status_change(i, rSwitch[i], status, status);
    }
  }
}
// === end /power section ===

void main_debug_info();

void wifiConnect_debug_info() {
  wifiConnect.debug_info();
}

void wifi_reconnect() {
  wifiConnect.try_reconnect();
}

void wifi_set_11b() {
  wifiConnect.set_phy_11b();
  wifiConnect.try_reconnect();
}
void wifi_set_11g() {
  wifiConnect.set_phy_11g();
  wifiConnect.try_reconnect();
}
void wifi_set_11n() {
  wifiConnect.set_phy_11n();
  wifiConnect.try_reconnect();
}
#ifdef ESP32
void wifi_set_lr() {
  wifiConnect.set_phy_lr();
  wifiConnect.try_reconnect();
}
#endif

void switch_debug_info_idx(int idx) {
  if (rSwitch[idx] != NULL) {
    rSwitch[idx]->debug_info();
  }
}

void switch_debug_info_1() {
  switch_debug_info_idx(0);
}

void switch_debug_info() {
  for (int i=0; i<3; i++) {
    switch_debug_info_idx(i);
  }
}

void otaClient_debug_info() {
  otaClient.debug_info();
}

void mqttClient_debug_info() {
  mqttClient.debug_info();
}

void test_debug_info() {
  test.debug_info();
}

void restart() {
  ESP.restart();
}

// =======================================================
// SETUP
// =======================================================

void setup() {

  test.begin(test_change_event_callback, TEST_LED_PULLUP);
  // test.begin(test_change_event_callback);
  test.setKeepAliveEventCallback(keepAlive_event_callback, 60000L);

  Serial.begin(115200);
  delay(2000);
  Serial.printf_P(PSTR("\n%s %s..\n"), FPSTR(APP_NAME), FPSTR(APP_VER));

  // Time, SNTP
#if defined(ESP8266)
  settimeofday_cb(settime_cb);
  configTime(MYTZ, "pool.ntp.org");
#endif
#if defined(ESP32)
  const long  gmtOffset_sec = 7200; // GMT +2
  const int   daylightOffset_sec = 3600;
  sntp_set_time_sync_notification_cb(settime_cb);
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
#endif
  // wifiConnect.begin_nb(SSID, PWD, true);
  wifiConnect.begin(SSID, PWD, HOST);
  //webServer.begin(APP_VER, STR_SWITCH);
  mqttClient.begin(mqtt_server, clientId, true);
  mqttClient.setAnnouncementTopic(announcementTopic);
  mqttClient.setLastWillTopic(lastWillTopic);
  mqttClient.setTopics(topics, TOPIC_COUNT);
  mqttClient.setCallback(mqtt_received_callback);

  rSwitch = new RSwitch*[3];
  rSwitch[0] = new RSwitch(POWER_SWITCH1_GPIO, POWER_DETECT1_GPIO);
  rSwitch[1] = new RSwitch(POWER_SWITCH2_GPIO, POWER_DETECT2_GPIO);
  rSwitch[2] = new RSwitch(POWER_SWITCH3_GPIO, POWER_DETECT3_GPIO);

  pinMode(BUTTON_GPIO, INPUT);

  for (int i=0; i<3; i++) {
    if (rSwitch[i] != NULL) {
      rSwitch[i]->setCallback(power_status_change_callback);
      Serial.printf("Switch %d Switch GPIO: %d Detect GPIO: %d\n", i+1, rSwitch[i]->getSwitchGPIO(), rSwitch[i]->getDetectGPIO());
    }
  }

  otaClient.begin(HOST, OTA_PASSWD);
  
  webSServer.begin(APP_VER, rSwitch);

  serialDebug.addInfoCommandHandler(main_debug_info);
  serialDebug.addInfoCommandHandler(switch_debug_info);
  serialDebug.addInfoCommandHandler(wifiConnect_debug_info);
  serialDebug.addInfoCommandHandler(otaClient_debug_info);
  serialDebug.addInfoCommandHandler(mqttClient_debug_info);
  serialDebug.addInfoCommandHandler(test_debug_info);
  serialDebug.addCommandHandler("wifi_reconnect", wifi_reconnect);
  serialDebug.addCommandHandler("wifi_set_11b", wifi_set_11b);
  serialDebug.addCommandHandler("wifi_set_11g", wifi_set_11g);
  serialDebug.addCommandHandler("wifi_set_11n", wifi_set_11n);
  #ifdef ESP32
  serialDebug.addCommandHandler("wifi_set_lr", wifi_set_lr);
  #endif
  serialDebug.addCommandHandler("switch_info", switch_debug_info);
  serialDebug.addCommandHandler("switch_info1", switch_debug_info_1);
  serialDebug.addCommandHandler("wifi_info", wifiConnect_debug_info);
  serialDebug.addCommandHandler("mqtt_info", mqttClient_debug_info);
  serialDebug.addCommandHandler("restart", restart);

  Serial.printf("###### Button: %s\n", (digitalRead(BUTTON_GPIO) == 1 ? "not pressed" : "PRESSED"));
}

// =======================================================
// LOOP
// =======================================================
unsigned long ll = millis();

void loop() {

  // webServer.handleClient();      // Listen for HTTP requests from clients
  wifiConnect.handleClient();       // Required for WiFiConnect none-blocking version and reconnect functionality
  if (wifiConnect.connectionEstablished()) { // Required to fully configure hostname in case it generated by wifiConnect
    mqttClient.handleClient();      // Listen for MQTT events from broker
  }
  otaClient.handleClient();
  for (int i=0; i<3; i++) {
    if (rSwitch[i] != NULL) {
      rSwitch[i]->handleClient(); // handle switch events
    }
  }
  test.handleClient();            // Test WiFi and MQTT connection
  serialDebug.handleClient();
  webSServer.handleClient();

  if (millis() - 1000 > ll) {
    Serial.printf("=======>>>>> Button: %s\n", (digitalRead(BUTTON_GPIO) == 1 ? "not pressed" : "PRESSED"));
    ll = millis();
  }
}

/*
void send_status() {
  mqttClient.publish(STR_SWITCH, rSwitch.getStatus() ? "1" : "0", true);
}


bool get_power_status() {
  return !digitalRead(POWER_DETECT_GPIO);
}
*/

void send_info(const char* topic, boolean connection_info) {
  // /firmware owns this function (plumbing); the doc[...] field set below is the
  // /mqtt `info` JSON telemetry schema (C1) — coordinate field add/rename with /mqtt
  // and watch the 1024-byte DynamicJsonDocument cap.
  time_t now = time(nullptr);
  doc.clear();
  
  doc["app"] = FPSTR(APP_NAME);
  doc["ver"] = FPSTR(APP_VER);
  doc["host"] = HOST;

  for (int i=0; i<3; i++) {
    if (rSwitch[i] != NULL) {
      sprintf(BUF, "powerStatus%d", i+1);
      doc[BUF] = rSwitch[i]->getPowerStatus();
    }
  }

  doc["ts"] = (uint32_t)now;
  char *t = ctime(&now);
  t[strlen(t) -1] = '\0';
  doc["now"] = &t[4];
  doc["boottime"] = (uint32_t)boottime;
  t = ctime(&boottime);
  t[strlen(t) -1] = '\0';
  doc["boot"] = &t[4];
  doc["MAC"] = wifiConnect.getMAC();
  doc["IP"] = wifiConnect.getIP();
  doc["hostname"] = wifiConnect.getHostname();
  doc["mdns"] = otaClient.getHostname();
  doc["RSSI"] = wifiConnect.getRSSI();
  doc["phymode"] = wifiConnect.getPhyModeString();
  doc["chan"] = wifiConnect.getChannel();
  doc["BSSID"] = wifiConnect.getBSSID();
  doc["timeset"] = timeset_cnt;

  // debug:
  doc["mc"] = "";
#if defined(ESP8266)
  doc["mc"] = F("ESP8266");
#endif
#if defined(ESP32)
  doc["mc"] = F("ESP32");
#endif
  doc["freeHeap"] = ESP.getFreeHeap();
#if defined(ESP8266)
  doc["fragmentation"] = ESP.getHeapFragmentation();
  doc["maxFreeBlockSize"] = ESP.getMaxFreeBlockSize();
#endif
  doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
#if defined(ESP8266)
  doc["coreVer"] = ESP.getCoreVersion();
#endif
  doc["sdkVer"] = ESP.getSdkVersion();
  doc["sketchSize"] = ESP.getSketchSize();
  doc["freeSketchSpace"] = ESP.getFreeSketchSpace();
  doc["flashChipSize"] = ESP.getFlashChipSize();
// json size saving
//#if defined(ESP8266)
//  doc["flashChipRealSize"] = ESP.getFlashChipRealSize();
//#endif

  if (connection_info) {
    doc["wifi_lost"] = (uint32_t)test.wifi_lost_timestamp;
    doc["mqtt_lost"] = (uint32_t)test.mqtt_lost_timestamp;
    doc["wifiLostCnt"] = test.wifi_lost_cnt;
    doc["mqttLostCnt"] = test.mqtt_lost_cnt;
    doc["wifi_conn"] = (uint32_t)test.wifi_conn_timestamp;
    doc["mqtt_conn"] = (uint32_t)test.mqtt_conn_timestamp;
    if (test.wifi_lost_timestamp > 0L) {
      t = ctime(&test.wifi_lost_timestamp);
      t[strlen(t) -1] = '\0';
      doc["wifiLost"] = &t[4];
    }
    if (test.mqtt_lost_timestamp > 0L) {
      t = ctime(&test.mqtt_lost_timestamp);
      t[strlen(t) -1] = '\0';
      doc["mqttLost"] = &t[4];
    }
    if (test.wifi_conn_timestamp > 0L) {
      t = ctime(&test.wifi_conn_timestamp);
      t[strlen(t) -1] = '\0';
      doc["wifiConn"] = &t[4];
    }
    if (test.mqtt_conn_timestamp > 0L) {
      t = ctime(&test.mqtt_conn_timestamp);
      t[strlen(t) -1] = '\0';
      doc["mqttConn"] = &t[4];
    }
    doc["badIp"] = test.bad_dhcp_ip_assigned;
  }

  serializeJson(doc, BUF, BUFFER_SIZE);
  mqttClient.publish(topic, BUF);
}

void main_debug_info() {
  time_t now = time(nullptr);
  Serial.println("\nmain.cpp:\n=========");
  Serial.printf("AppName: %s\n", FPSTR(APP_NAME));
  Serial.printf("Version: %s\n", FPSTR(APP_VER));
  for (int i=0; i<3; i++) {
    Serial.printf("PowerStatus%d: ", i+1);
    if (rSwitch[i] != NULL) {
      Serial.printf("%d\n", rSwitch[i]->getPowerStatus());
    }
    else {
      Serial.println("NOT DEFINED");
    }
  }
  Serial.printf("Now timestamp: %u\n", (uint32_t)now);
  char *t = ctime(&now);
  t[strlen(t) -1] = '\0';
  Serial.printf("Now: %s\n", t);
  Serial.printf("Boot timestamp: %u\n", (uint32_t)boottime);
  t = ctime(&boottime);
  t[strlen(t) -1] = '\0';
  Serial.printf("Boot time: %s\n", t);
  Serial.printf("timeset_cnt: %d\n", timeset_cnt);
  // debug:
#if defined(ESP8266)
  Serial.printf("MC: ESP8266");
#endif
#if defined(ESP32)
  Serial.printf("MC: ESP32");
#endif
  Serial.printf("FreeHeap: %u\n", ESP.getFreeHeap());
#if defined(ESP8266)
  Serial.printf("HeapFragmentation: %u\n", ESP.getHeapFragmentation());
  Serial.printf("MaxFreeBlockSize: %u\n", ESP.getMaxFreeBlockSize());
#endif
  Serial.printf("CpuFreqMHz: %u\n", ESP.getCpuFreqMHz());
#if defined(ESP8266)
  Serial.printf("CoreVersion: %s\n", ESP.getCoreVersion().c_str());
#endif
  Serial.printf("SdkVersion: %s\n", ESP.getSdkVersion());
  Serial.printf("SketchSize: %u\n", ESP.getSketchSize());
  Serial.printf("FreeSketchSpace: %u\n", ESP.getFreeSketchSpace());
  Serial.printf("FlashChipSize: %u\n", ESP.getFlashChipSize());
#if defined(ESP8266)
  Serial.printf("FlashChipRealSize: %u\n", ESP.getFlashChipRealSize());
#endif
}


// === /mqtt section — inbound command routing (C1 decode -> C3 actuation) — owner: /mqtt ===
void mqtt_received_callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("mqtt_received_callback called: '%s'\n", topic);
  if(strcmp(topic, STR_POWER_SET) == 0) {
    int switch_idx = payload[2] - '1';
    if (switch_idx >-1 && switch_idx < 3) {
      if (rSwitch[switch_idx] != NULL) {
        if (payload[0] == '1') {
          rSwitch[switch_idx]->setSwitch(true);
        } else if (payload[0] == '0') {
          rSwitch[switch_idx]->setSwitch(false);
        } else if (payload[0] == '3') {
          rSwitch[switch_idx]->longPress();
        }
        // send_status();
      }
    }
  } else if(strcmp(topic, STR_POWER_GET) == 0) {
    // send_status();
  } else if(strcmp(topic, STR_ALIVE_GET) == 0) {
    send_info(STR_ALIVE, false);
  } else if(strcmp(topic, STR_INFO_GET) == 0) {
    send_info(STR_INFO, true);
  }
}
// === end /mqtt section ===

// test_change_event_callback / keepAlive_event_callback below are /firmware shell wiring:
// they route /net Test events (C4) to /mqtt + /power actions. Owner: /firmware.
void test_change_event_callback(uint8_t event) {
  if (event == EVENT_MQTT_RESTORED) {
    send_info(STR_CONNECTION_INFO, true);
    keepAlive_event_callback();
    refresh_all_switches_state();
  }
}

void keepAlive_event_callback() {
  if (test.wifiConnected()) {
    mqttClient.publish(STR_KEEPALIVE, "connected");
  }
}