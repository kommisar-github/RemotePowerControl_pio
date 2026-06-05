#ifndef _SN_MQTT_CLIENT_H_
#define _SN_MQTT_CLIENT_H_

#define MQTT_MAX_PACKET_SIZE 1024
#define WILLQOS 0

#include <WiFiClient.h>
#include <PubSubClient.h>

class MqttClient {

  public:
    MqttClient();

    void setTimeOut(long timeout);
    void setRecconectInterval(long ms);

    void setAnnouncementTopic(const char *);
    void setLastWillTopic(const char *);
    void setLastWillTopic(const char *, boolean);
    void setLastWillTopic(const char *, boolean, const char *);
    void setTopics(const char **, int);
    void setCallback(void (*custom_callback)(char*, uint8_t*, unsigned int));
    void setConnectEvent(void (*connect_event)(boolean connected));

    void begin(const char *mqtt_server, const char* clientId);
    void begin(const char *mqtt_server, const char* clientId, boolean enabled);
    void begin(const char *mqtt_server, const char* clientId, const char *user, const char *passwd);
    
    boolean connected();

    void handleClient();

    boolean subscribe(const char* topic);
    boolean publish(const char* topic, const char* payload);
    boolean publish(const char* topic, const char* payload, boolean retained);

    const char* getClientId();
    const char* getUser();
    const char* getBroker();

    void debug_info();

  private:
    WiFiClient *_wifiClient;
    PubSubClient *_client;

    const char* m_announcementTopic = nullptr;
    const char* m_lastWillTopic;
    boolean m_lastWillRetain;
    const char* m_lastWillMessage;

    const char** _topics = nullptr;;
    int _topics_cnt = 0;

    boolean _enabled = false;
    const char* _clientId = nullptr;
    const char* _user = nullptr;
    const char* _passwd = nullptr;
    const char* _mqtt_server = nullptr;

    unsigned long _lastReconnectAttempt = 0;
    unsigned long _reconnectInterval = 10000;
    
    boolean reconnect();
    void publish_announcement();
    void resubscribe();
    void callback(char* topic, byte* payload, unsigned int length);
    void (*custom_callback)(char*, uint8_t*, unsigned int) = 0;

    void fire_connect_event(boolean connected);
    void (*connect_event)(boolean connected) = 0;

    void dbglog(char* msg, boolean flag);

};

extern MqttClient mqttClient;

#endif
