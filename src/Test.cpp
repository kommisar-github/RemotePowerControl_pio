// Test WiFi & MQTT connection:
//
//  * LED OFF      - WiFi not connected
//  * LED ON       - WiFi and MQTT connected 
//  * LED blinking - WiFi connected, MQTT not connected
// 
// Define TEST_WIFI_LED <LED_GPIO>
//
// Default LED connection: pull-down
//             +-----------+    LED
//     GPIO ---| 220R-510R |----|>|----+
//             +-----------+           |
//                                    --- GND
// pull-up LED connection:
//                                    --- VCC
//             +-----------+    LED    |
//     GPIO ---| 220R-510R |----|<|----+
//             +-----------+           
//
//  Suggested LED connection method:
//   1) pull-down: D0
//   2)   pull-up: D3, D4, D9, D10
//
// v0.2 20220129 
// v0.3 20220614 - Fix not updated wifi_conn_timestamp value
// v0.4 20220616 - Test and KeepAlive event
// v0.5 20221014 - Fire WiFi and MQTT connect and lost events
// v0.6 20221219 - Fix mqtt connection time when NTP received firs time
// v0.7 20230304 - If wifi connected, but mqtt is not connected and network id is 192.168.66.0, try reconnect
// v0.8 20230519 - If wifi connected, but mqtt is not connected and GetwayIP is not set, try reconnect
// v0.9 20231009 - Added default output LED level to begin(). 
// v0.10 20231206 - Initialization fix

#include <time.h>                       // time() ctime()
#include "Application.h"
#include "Test.h"
#include "WiFiConnect.h"
#include "MqttClient.h"
#include "log.h"

#define MIN_TIMESTAMP 1640998800L  // Timestamp in sec. 1640998800  January 1, 2022 3:00:00 AM GMT+02:00

Test test;

Test::Test() {
  test_led_connection = TEST_LED_PULLDOWN;
  begin();
}

void Test::begin() {
  pinMode(TEST_WIFI_LED, OUTPUT);
  if (test_led_connection == TEST_LED_PULLDOWN) {
    LED_ON = HIGH;
  }
  else {
    LED_ON = LOW;
  }
  LED_OFF = !LED_ON;
  digitalWrite(TEST_WIFI_LED, LED_OFF);
}

void Test::begin(void (*change_event_callback_ptr)(uint8_t event)) {
  setChangeEventCallback(change_event_callback_ptr);
  begin();
}

void Test::begin(boolean test_led_connection) {
  this->test_led_connection = test_led_connection;
  begin();
}

void Test::begin(void (*change_event_callback_ptr)(uint8_t event), boolean test_led_connection) {
  this->test_led_connection = test_led_connection;
  begin(change_event_callback_ptr);
}


void Test::setChangeEventCallback(void (*change_event_callback_ptr)(uint8_t event)) {
  this->change_event_callback_ptr = change_event_callback_ptr;
}

void Test::setTestEventCallback(void (*test_event_callback_ptr)()) {
  this->test_event_callback_ptr = test_event_callback_ptr;
}

void Test::setKeepAliveEventCallback(void (*keepalive_event_callback_ptr)(), unsigned long interval) {
  this->keepalive_event_callback_ptr = keepalive_event_callback_ptr;
  this->keepalive_event_interval = interval;
  this->test_keepalive_lastActivity = millis(); // run event next cycle
}

void Test::handleClient() {
  test_connection();
}

boolean Test::wifiConnected() {
  return wifiConnect.connected();
}

boolean Test::mqttConnected() {
  return wifiConnect.connected() && mqttClient.connected();
}

void Test::change_event_callback(uint8_t event) {
  if (change_event_callback_ptr != 0) {
    change_event_callback_ptr(event);
  }
}

void Test::test_event_callback() {
  if (test_event_callback_ptr != 0) {
    test_event_callback_ptr();
  }
}

void Test::keepalive_event_callback() {
  if (keepalive_event_callback_ptr != 0) {
    keepalive_event_callback_ptr();
  }
}


/***
 * LED OFF      - WiFi not connected
 * LED ON       - WiFi and MQTT connected 
 * LED blinking - WiFi connected, MQTT not connected
 */
void Test::test_connection() {

  unsigned long t = millis();
  if (t - test_connection_lastActivity >= TESTCONNECTION_INTERVAL) {
    //  LOG(test_led_connection == TEST_LED_PULLDOWN ? "TEST_LED_PULLDOWN" : "TEST_LED_PULLUP");
    test_connection_lastActivity = t;
    if (wifiConnect.connected()) {
      if (wifi_conn_timestamp == 0) {
        wifi_conn_timestamp = time(nullptr); 
        change_event_callback(EVENT_WIFI_RESTORED);
        LOG("EVENT_WIFI_RESTORED: ", (ulong) wifi_conn_timestamp);
      } 
      else {
        if (wifi_conn_timestamp <= MIN_TIMESTAMP) { // This should fix wifi conection timestamp for late SNTP update 
          time_t tm = time(nullptr); 
          if (tm > MIN_TIMESTAMP) {
            LOG("wifi_conn_timestamp changed from: ", (ulong) wifi_conn_timestamp);
            wifi_conn_timestamp = tm - wifi_conn_timestamp; 
            LOG("wifi_conn_timestamp changed to: ", (ulong) wifi_conn_timestamp);
          }
        }
      }
      if (mqttClient.connected()) {
        digitalWrite(TEST_WIFI_LED, LED_ON);

        if (mqtt_conn_timestamp == 0L) {
          mqtt_conn_timestamp = time(nullptr);
          // MQTT Restored. Notify it
          change_event_callback(EVENT_MQTT_RESTORED);
          LOG("EVENT_MQTT_RESTORED: ", (ulong)mqtt_conn_timestamp);
        } 
        else {
          if (mqtt_conn_timestamp <= MIN_TIMESTAMP) { // This should fix mqtt conection timestamp for late SNTP update 
            time_t tm = time(nullptr); 
            if (tm > MIN_TIMESTAMP) {
              LOG("mqtt_conn_timestamp changed from: ", (ulong) mqtt_conn_timestamp);
              mqtt_conn_timestamp = tm - mqtt_conn_timestamp; 
              LOG("mqtt_conn_timestamp changed to: ", (ulong) mqtt_conn_timestamp);
            }
          }
        }
      }
      else {
        digitalWrite(TEST_WIFI_LED, !digitalRead(TEST_WIFI_LED));
        if (mqtt_conn_timestamp > 0L) {
          mqtt_lost_cnt++;
          mqtt_conn_timestamp = 0L;
          mqtt_lost_timestamp = time(nullptr); 
          change_event_callback(EVENT_MQTT_LOST);
          LOG("EVENT_MQTT_LOST: ", (ulong) mqtt_lost_timestamp);
        }
        if (t - test_ip_assigned_lastActivity >= TEST_IP_ASSIGNED_INTERVAL) {
          test_ip_assigned_lastActivity = t;
          if (!wifiConnect.isIPAddressSet(wifiConnect.getGatewayIPAddress()) || wifiConnect.getNetworkID().compareTo("192.168.66.0")==0) {
            bad_dhcp_ip_assigned++;
            wifiConnect.try_reconnect();
          }
        }
      }
    }
    else {
      digitalWrite(TEST_WIFI_LED, LED_OFF);
      if (wifi_conn_timestamp > 0L) {
        wifi_lost_cnt++;
        wifi_conn_timestamp = 0L;
        wifi_lost_timestamp = time(nullptr);
        change_event_callback(EVENT_WIFI_LOST);
        LOG("EVENT_WIFI_LOST: ", (ulong) wifi_lost_timestamp);
      }
    }
    test_event_callback();
  }
  if (t - test_keepalive_lastActivity >= keepalive_event_interval) {
    test_keepalive_lastActivity = t;
    keepalive_event_callback();
  }
}

void Test::debug_info(void) {
  time_t now = time(nullptr);
  char *t = ctime(&now);
  Serial.println("\nTest.cpp:\n=========");
  Serial.printf("wifi_lost_timestamp: %u\n", (uint32_t)wifi_lost_timestamp);
  Serial.printf("mqtt_lost_timestamp: %u\n", (uint32_t)mqtt_lost_timestamp);
  Serial.printf("wifi_lost_cnt: %d\n", wifi_lost_cnt);
  Serial.printf("mqtt_lost_cnt: %d\n", mqtt_lost_cnt);
  Serial.printf("wifi_conn_timestamp: %u\n", (uint32_t)wifi_conn_timestamp);
  Serial.printf("mqtt_conn_timestamp: %u\n", (uint32_t)mqtt_conn_timestamp);
  if (wifi_lost_timestamp > 0L) {
    t = ctime(&wifi_lost_timestamp);
    t[strlen(t) -1] = '\0';
    Serial.printf("wifi_lost: %s\n", t);
  }
  if (mqtt_lost_timestamp > 0L) {
    t = ctime(&mqtt_lost_timestamp);
    t[strlen(t) -1] = '\0';
    Serial.printf("mqtt_lost: %s\n", t);
  }
  if (wifi_conn_timestamp > 0L) {
    t = ctime(&wifi_conn_timestamp);
    t[strlen(t) -1] = '\0';
    Serial.printf("wifi_conn: %s\n", t);
  }
  if (mqtt_conn_timestamp > 0L) {
    t = ctime(&mqtt_conn_timestamp);
    t[strlen(t) -1] = '\0';
    Serial.printf("mqtt_conn: %s\n", t);
  }
  Serial.printf("bad_dhcp_ip_assigned: %d\n", bad_dhcp_ip_assigned);
}