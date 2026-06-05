#ifndef _SN_TEST_H_
#define _SN_TEST_H_

// Test WiFi & MQTT connection indication
#define TESTCONNECTION_INTERVAL 1000  // 1 sec
#define TEST_IP_ASSIGNED_INTERVAL 60000  // 60 sec
#define TEST_KEEP_ALIVE_DEFAULT_INTERVAL 60000  // 60 sec

#define EVENT_WIFI_RESTORED 1
#define EVENT_MQTT_RESTORED 2
#define EVENT_WIFI_LOST     3
#define EVENT_MQTT_LOST     4

#define TEST_LED_PULLUP     0
#define TEST_LED_PULLDOWN   1


class Test {

  public:
    Test();

    void begin();
    void begin(boolean test_led_connection);
    void begin(void (*change_event_callback_ptr)(uint8_t event));
    void begin(void (*change_event_callback_ptr)(uint8_t event), boolean test_led_connection);
    void setChangeEventCallback(void (*change_event_callback_ptr)(uint8_t event));
    void setTestEventCallback(void (*test_event_callback_ptr)());
    void setKeepAliveEventCallback(void (*keepalive_event_callback_ptr)(), unsigned long interval = 60000L);
    void handleClient();

    boolean wifiConnected();
    boolean mqttConnected();

    void debug_info(void);

  private:
    bool test_led_connection;
    bool LED_ON;
    bool LED_OFF;
    unsigned long test_connection_lastActivity = TESTCONNECTION_INTERVAL * -2;
    unsigned long test_ip_assigned_lastActivity = TEST_IP_ASSIGNED_INTERVAL * -2;
    unsigned long test_keepalive_lastActivity = 0L;
    void test_connection();
  
    void (*change_event_callback_ptr)(uint8_t event) = 0;
    void change_event_callback(uint8_t event);
    void (*test_event_callback_ptr)() = 0;
    void test_event_callback();
    void (*keepalive_event_callback_ptr)() = 0;
    void keepalive_event_callback();
    unsigned long keepalive_event_interval = TEST_KEEP_ALIVE_DEFAULT_INTERVAL;

  public:
    unsigned int wifi_lost_cnt = 0;
    unsigned int mqtt_lost_cnt = 0;
    time_t wifi_lost_timestamp = 0L;
    time_t mqtt_lost_timestamp = 0L;
    time_t wifi_conn_timestamp = 0L;
    time_t mqtt_conn_timestamp = 0L;
    unsigned int bad_dhcp_ip_assigned = 0;
};

extern Test test;

#endif
