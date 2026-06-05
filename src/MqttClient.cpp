// MqttClient
// 
// v0.4 20220503 ESP8266 & ESP32 supported
// v0.5 20220608 Fire connect event
// v0.6 20220613 Last Will Testament support
// v0.7 20220614 Last Will Testament support fix
// v0.8 20221014 Test WiFi connection when reconnect, added debug_info()
// v0.9 20221202 Small fixes
// v0.10 20230403 Additional getters
// v0.11 20230501 Debug info added
// v0.12 20230602 publish() add sent/not sent to message
// v0.13 20231206 removing members initialization defects
//
// TODO:
//   > Replace Serial.print() with LOG()
//
// Info:
//  Max lengt of the message <= MQTT_MAX_PACKET_SIZE
//  The lengt of the message = 5 (header len) + 2 (topic len) + strlen(topic) + strlen(payload)

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#if defined(ESP32)
#include <WiFi.h>
#endif
#include "MqttClient.h"
#include "log.h"

#define STR_CONNECTED    "connected"
#define STR_DISCONNECTED "disconnected"

MqttClient mqttClient;  // Created MqttClient instance

MqttClient::MqttClient() {
  _wifiClient = new WiFiClient();
  _wifiClient->setTimeout(1000);
  _client = new PubSubClient(*_wifiClient);
  _client->setBufferSize(MQTT_MAX_PACKET_SIZE);
  m_lastWillTopic = nullptr;
  m_lastWillRetain = false;
  m_lastWillMessage = STR_DISCONNECTED;
}

void MqttClient::setTimeOut(long timeout) {
  _wifiClient->setTimeout(timeout);
}

void MqttClient::setRecconectInterval(long reconnectInterval) {
  _reconnectInterval = reconnectInterval;
}

void MqttClient::setAnnouncementTopic(const char *topic) {
  m_announcementTopic = topic;
}

void MqttClient::setLastWillTopic(const char *topic) {
  m_lastWillTopic = topic;
}

void MqttClient::setLastWillTopic(const char *topic, boolean retain) {
  m_lastWillRetain = retain;
  setLastWillTopic(topic);
}

void MqttClient::setLastWillTopic(const char *topic, boolean retain, const char *message) {
  m_lastWillMessage = message;
  setLastWillTopic(topic, retain);
}

void MqttClient::setTopics(const char **topics, int count) {
  _topics = topics;
  _topics_cnt = count;
}

void MqttClient::setCallback(void (*custom_callback)(char*, uint8_t*, unsigned int)) {
  this->custom_callback = custom_callback;
}

void MqttClient::setConnectEvent(void (*connect_event)(boolean connected)) {
  this->connect_event = connect_event;
}

void MqttClient::begin(const char *mqtt_server, const char* clientId) {
  _enabled = true;
  begin(mqtt_server, clientId, NULL, NULL);
}

void MqttClient::begin(const char *mqtt_server, const char* clientId, boolean enabled) {
  _enabled = enabled;
  begin(mqtt_server, clientId, NULL, NULL);
}

void MqttClient::begin(const char *mqtt_server, const char* clientId, const char *user, const char *passwd) {
  this->_enabled = true;
  this->_mqtt_server = mqtt_server;
  this->_clientId = clientId;
  this->_user = user;
  this->_passwd = passwd;
  _client->setServer(mqtt_server, 1883);
  _client->setCallback([this](char* topic, byte * payload, unsigned int length) {
    callback(topic, payload, length);
  });
}

boolean MqttClient::connected() {
  return (_enabled && _client->connected());
}

const char*  MqttClient::getClientId() {
  return this->_clientId;
}

const char*  MqttClient::getUser() {
  return this->_user;
}

const char*  MqttClient::getBroker() {
  return this->_mqtt_server;
}


void MqttClient::handleClient() {

  if (!_enabled) {
    return;
  }
  if (!_client->connected()) {
    unsigned long now = millis();
    if (now - _lastReconnectAttempt > _reconnectInterval) {
      _lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        _lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    _client->loop();
  }
}

boolean MqttClient::reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    LOG("MQTT reconnect bypassed: WiFi not connected. Status:", (uint8_t) WiFi.status());
    return false;
  }
  LOG("MQTT reconnecting...");
  unsigned long start = millis();
  boolean con;
  if (m_lastWillTopic != nullptr) {
    con = _client->connect(_clientId, _user, _passwd, m_lastWillTopic, WILLQOS, m_lastWillRetain, m_lastWillMessage);
  }
  else {
    con = _client->connect(_clientId, _user, _passwd);
  }
  if (con) {
    Serial.printf("#### MQTT CONNECTED clientId: %s Connection time: %lu ms\n", _clientId, millis() - start);
    // Once connected, publish an announcement & resubscribe
    publish_announcement();
    resubscribe();
    fire_connect_event(true);
  }
  else {
    Serial.printf("#### MQTT CONNECT TIMEOUT %lu ms clientId: %s \n", millis() - start, _clientId);
  }
  boolean ret = _client->connected();
  Serial.printf("#### MQTT reconnection time used: %lu ms\n", millis() - start);
  return ret;
}

boolean MqttClient::subscribe(const char* topic) {
  if (!_enabled) 
    return false;
  return _client->subscribe(topic);
}

boolean MqttClient::publish(const char* topic, const char* payload) {
  return publish(topic, payload, false);
}

boolean MqttClient::publish(const char* topic, const char* payload, boolean retained) {
  if (!_enabled) 
    return false;
    unsigned long start = millis();
  if (_client->connected()) {
    boolean ret = _client->publish(topic, payload, retained);
    unsigned long time_used = millis() - start;
    Serial.print("Publish: <");
    Serial.print(topic);
    Serial.print("> :[");
    Serial.print(payload);
    Serial.print("] ");
    Serial.print(time_used);
    Serial.print(retained ? " ms (RETAINED) " : " ms ");
    Serial.println(ret ? " sent" : " - NOT SENT !!!");
    if (!ret) {
      if (MQTT_MAX_PACKET_SIZE < MQTT_MAX_HEADER_SIZE + 2 + strlen(topic) + strlen(payload)) {
        Serial.printf("WARN: topic: %d payload: %d; Total: %d > Max Len: %d\n", 
          strlen(topic),  strlen(payload), 5+2 + strlen(topic) +  strlen(payload), MQTT_MAX_PACKET_SIZE);
      }
    }
    return ret;
  }
  else {
    unsigned long time_used = millis() - start;
    Serial.print("Publish: NOTCONNECTED ");
    Serial.print(time_used);
    Serial.println(" ms");
    return false;
  }
}

void MqttClient::publish_announcement() {
  if (m_announcementTopic !=nullptr) {
    const char *announcement = STR_CONNECTED;
    _client->publish(m_announcementTopic, announcement);
    Serial.print("MQTT announcement sent");
    Serial.print("<");
    Serial.print(m_announcementTopic);
    Serial.print(">: [");
    Serial.print(announcement);
    Serial.println("]");
  }
}

void MqttClient::resubscribe() {
  if (_topics_cnt > 0) {
    Serial.println("MQTT subscribing");
    for(int i=0; i<_topics_cnt; i++) {
      Serial.print("MQTT subscribing for ");
      Serial.println(_topics[i]);
      subscribe(_topics[i]);
    }
    _client->subscribe("inTopic");
    Serial.println("MQTT subscribed");
  }
}

void MqttClient::callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived <");
  Serial.print(topic);
  Serial.print(">:[");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("]");
  if (custom_callback) {
    custom_callback(topic, payload, length);
  }

}

void MqttClient::fire_connect_event(boolean connected) {
  if (connect_event != 0) {
    connect_event(connected);
  }
}

void MqttClient::debug_info() {
  Serial.println("\nMqttClient.cpp:\n===============");
  Serial.printf("MQTT Broker: %s\n", this->_mqtt_server!=nullptr?this->_mqtt_server:"NULL");
  Serial.printf("MQTT clientId: %s\n", this->_clientId!=nullptr?this->_clientId:"NULL");
  Serial.printf("MQTT User: %s\n", this->_user!=nullptr?this->_user:"NULL");
  Serial.printf("MQTT Password: %s\n", this->_passwd!=nullptr?this->_passwd:"NULL");
  Serial.printf("MQTT Client: %sCONNECTED\n", _client->connected() ? "" : "NOT ");
  Serial.printf("WiFi Client: %sCONNECTED or %sAVAILABLE\n", _wifiClient->connected() ? "" : "NOT ", _wifiClient->available() ? "" : "NOT ");
  Serial.printf("WiFi Client local IP: %s port: %d\n", _wifiClient->localIP().toString().c_str(), _wifiClient->localPort());
  Serial.printf("WiFi Client remote IP: %s port: %d\n", _wifiClient->remoteIP().toString().c_str(), _wifiClient->remotePort());
}


/* Blocking example
  void MqttClient::reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  }
*/
