// WiFiConnect 
//
// Versions:
//   v0.4          - ESP8266 & ESP32 supported
//   v0.5 20220712 - Host/MDNS fixes
//   v0.6 20221014 - Added debug_info()
//   v0.7 20221015 - None blocking connect 
//   v0.8 20221203 - Reconnect implemented, more info logs
//   v0.9 20221213 - Fixes: Added recconect and initial_connection_setup() in the begin()
//   v0.10 20221219 - more data in debug_info()
//   v0.11 20221225 - set this->hostname when it was generated
//   v0.12 20230102 - Addin additional none-blocking begin's()
//   v0.13 20230403 - Addin additional getters
//   v0.14 20230501 - Verify this.hosthame is not null, getChannel() added. (ESP8266 Arduino Core 3.1.2 / ESP32 Arduino 2.0.8)
//   v0.15 20230502 - Refacroting. Remove try_reconnect() from set_phy_xxx
//   v0.16 20230519 - Added additional getters
//   v0.17 20230603 - MdnsClient, Reconnect policy changed, generate hostname fix for re-connect
//   v0.18 20230909 - Remove static member variables
//   v0.19 20231206 - Members initialization fix

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#ifdef ESP32
#include <WiFi.h>
// #include <WiFiType.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#endif

#include "log.h"
#include "WiFiConnect.h"
#include "MdnsClient.h"

// Test WiFi connection interval
#define TEST_CONNECTION_INTERVAL_INITIAL  500   // 0.5 sec
#define TEST_CONNECTION_INTERVAL_CALLBACK 500   // 0.5 sec
#define TEST_CONNECTION_INTERVAL_NB 10000       // 10 sec
#define RECONNECT_INTERVAL 60000                // 60 sec
#define TRY_PHY_N_INTERVAL 3600000              // 60 min


WiFiConnect wifiConnect;  // Created wifiConnect instance

WiFiConnect::WiFiConnect() {
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd) {
  return begin(ssid, pwd, NULL, nullptr, false, false, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd) {
  return begin(ssid, pwd, NULL, nullptr, false, false, true);
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd, const char *hostname) {
  return begin(ssid, pwd, hostname, nullptr, false, false, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd, const char *hostname) {
  return begin(ssid, pwd, hostname, nullptr, false, false, true);
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd, boolean generate_hostname) {
  return begin(ssid, pwd, NULL, nullptr, generate_hostname, false, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd, boolean generate_hostname) {
  return begin(ssid, pwd, NULL, nullptr, generate_hostname, false, true);
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd, boolean generate_hostname, boolean mdns_enabled) {
  return begin(ssid, pwd, NULL, nullptr, generate_hostname, mdns_enabled, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd, boolean generate_hostname, boolean mdns_enabled) {
  return begin(ssid, pwd, NULL, nullptr, generate_hostname, mdns_enabled, true);
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd, const char *hostname, boolean mdns_enabled) {
  return begin(ssid, pwd, hostname, nullptr, false, false, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd, const char *hostname, boolean mdns_enabled) {
  return begin(ssid, pwd, hostname, nullptr, false, false, true);
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd, boolean generate_hostname, void (*custom_callback)(void)) {
  return begin(ssid, pwd, NULL, custom_callback, generate_hostname, false, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd, boolean generate_hostname, void (*custom_callback)(void)) {
  return begin(ssid, pwd, NULL, custom_callback, generate_hostname, false, true);
}

boolean WiFiConnect::begin(const char* ssid, const char* pwd, const char *hostname, void (*custom_callback)(void)) {
  return begin(ssid, pwd, hostname, custom_callback, false, mdns_enabled, false);
}

boolean WiFiConnect::begin_nb(const char* ssid, const char* pwd, const char *hostname, void (*custom_callback)(void)) {
  return begin(ssid, pwd, hostname, custom_callback, false, mdns_enabled, true);
}

// If host specified - mDNS to be used (uncomment mDNS)
/*
   const char*  ssid     - WiFi access point SSID
   const char*  pwd      - WiFi access point password
   const char*  hostname - if not NULL, changes WiFi Client hostname
   custom_callback - Custom callback called during WiFi connect cycle
   boolean generate_hostname - Generate WiFi Client hostname
   boolean mdns_enabled
   boolean none_blocking
*/
boolean WiFiConnect::begin(const char* ssid, const char* pwd, const char *hostname, void (*custom_callback)(void), boolean generate_hostname, boolean mdns_enabled, boolean none_blocking) {
 
  this->none_blocking = none_blocking;
  this->ssid = ssid;
  this->pwd = pwd;
  this->hostname = hostname;
  this->generate_hostname = generate_hostname;
  this->mdns_enabled = mdns_enabled;
  this->custom_callback = custom_callback;
  this->connection_established = false;
  
  this->test_try_N_lastActivity = millis();

  this->reconnect_attempt_count = 0;
  this->test_connection_lastActivity = TEST_CONNECTION_INTERVAL_NB * -2;
  this->test_connection_cb_lastActivity = TEST_CONNECTION_INTERVAL_CALLBACK * -2;

  WiFi.persistent(false);
  
  #ifdef ESP8266
  static WiFiEventHandler gotIpEventHandler, connectedEventHandler, disconnectedEventHandler, dhcpTimeoutEventHandler;

  gotIpEventHandler = WiFi.onStationModeGotIP([/*this*/](const WiFiEventStationModeGotIP& event)
  {
    Serial.printf("## WiFi EVENT: Station got IP: %s\n", WiFi.localIP().toString().c_str());
    //Serial.printf("### Alex DEBUG: Station got IP this->hostname: %s\n", this->hostname != NULL ? this->hostname : "NULL");
  });

  connectedEventHandler = WiFi.onStationModeConnected ([/*this*/](const WiFiEventStationModeConnected &event)
  {
    Serial.println("## WiFi EVENT: Station connected");
    //Serial.printf("### Alex DEBUG: Station connected this->hostname: %s\n", this->hostname != NULL ? this->hostname : "NULL");
  });

  disconnectedEventHandler = WiFi.onStationModeDisconnected([/*this*/](const WiFiEventStationModeDisconnected& event)
  {
    Serial.println("## WiFi EVENT: Station disconnected");
    //Serial.printf("### Alex DEBUG: Station disconnected this->hostname: %s\n", this->hostname != NULL ? this->hostname : "NULL");
  });

  dhcpTimeoutEventHandler = WiFi.onStationModeDHCPTimeout ([/*this*/](void)
  {
    Serial.println("## WiFi EVENT: DHCP Timeout");
    //Serial.printf("### Alex DEBUG: DHCP Timeout this->hostname: %s\n", this->hostname != NULL ? this->hostname : "NULL");
  });
  #endif

  WiFi.mode(WIFI_STA);
  set_phy_11n();
  
  #ifdef ESP32
  // ESP32: custom hostname to be set before WiFi.begin()
  if(generate_hostname) {
      char _hostname[13];
      uint8_t mac[6];
      WiFi.macAddress(mac);
      sprintf(_hostname, "ESP32-%02X%02X%02X", mac[3], mac[4], mac[5]);
      Serial.printf("Set genetated hostname: %s\n", _hostname);
      WiFi.setHostname(_hostname);
      this->hostname = WiFi.getHostname(); // Alex: TODO: Investigate if I need to keep in the this->hostname to generated hostname
  }
  else {
    if (this->hostname != NULL) {
      if (this->hostname != WiFi.getHostname()) {
        Serial.printf("Set provided hostname: %s\n", this->hostname);
        WiFi.setHostname(this->hostname);
      }
    }
    WiFi.setHostname(hostname);
  }
  #endif

  WiFi.begin(ssid, pwd);        
  LOG("WiFiConnect: Connecting to ", ssid, " ...");
  if (!none_blocking) {
    last_reconnect_attempt = millis();
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
      if (custom_callback != nullptr) {
        custom_callback();
      }
      #ifdef ESP8266
      WiFi.setSleepMode(WIFI_NONE_SLEEP);
      #endif
      #ifdef ESP32
      WiFi.setSleep(WIFI_PS_NONE);
      #endif
      long period = millis() - last_reconnect_attempt;
      bool reconnect_fl = (period >= RECONNECT_INTERVAL);
      if (reconnect_fl) {
        reconnect();
        last_reconnect_attempt = millis();
        Serial.printf("WiFiConnect: Try to re-connect WiFi AP. WiFi status: %d - %s\n", 
          WiFi.status(), getStatusString());
      }
      else {
        delay(TEST_CONNECTION_INTERVAL_INITIAL);
        LOGX(".");
      }
    }
    initial_connection_setup();
  }

  return true;
}

void WiFiConnect::reconnect() {
  boolean py11n;

  #ifdef ESP8266
  py11n = (getPhyMode() != WIFI_PHY_MODE_11N) ? false : true;
  #endif
  #ifdef ESP32
  py11n = ((getPhyMode()&WIFI_PROTOCOL_11N) != WIFI_PROTOCOL_11N) ? false : true;
  #endif

  if (py11n && reconnect_attempt_count == 3) {   // Try it 3+1 times on 11N before entering here
      LOG("WiFiConnect: Setting WIFI_PHY_MODE_11G");
      set_phy_11g();
      try_reconnect();
      //py11n = false;
       reconnect_attempt_count = 0;
       return;
  }
  if (!py11n && reconnect_attempt_count == 2) {   // Try it 2+1 times on 11G before entering here
      LOG("WiFiConnect: Setting WIFI_PHY_MODE_11N");
      set_phy_11n();
      try_reconnect();
      //py11n = true;
      reconnect_attempt_count = 0;
      return;
  }
  reconnect_attempt_count++;
  Serial.print("WiFiConnect: Using ");
  Serial.print(getPhyModeString());
  Serial.print(" attempt: ");
  Serial.println(reconnect_attempt_count);
  WiFi.reconnect();
}

void WiFiConnect::initial_connection_setup() {
  #ifdef ESP8266
  // ESP8266: custom hostname to be set after WiFi.begin()
  if (generate_hostname) {
      char _hostname[15];
      uint8_t mac[6];
      WiFi.macAddress(mac);
      sprintf(_hostname, "ESP8266-%02X%02X%02X", mac[3], mac[4], mac[5]);
      Serial.printf("Set genetated hostname: %s\n", _hostname);
      WiFi.setHostname(_hostname);
      this->hostname = WiFi.getHostname();  // Alex: TODO: Investigate if I need to keep in the this->hostname to generated hostname
  }
  else {
    if (this->hostname != NULL) {
      if (this->hostname != WiFi.getHostname()) {
        Serial.printf("Set provided hostname: %s\n", this->hostname);
        WiFi.setHostname(this->hostname);
      }
    }
  }
  #endif

  LOG("\nConnection established!");
  LOG("Connected to: ", WiFi.SSID()); 
  LOG("IP address: ", WiFi.localIP().toString().c_str()); // .c_str()
  LOG("WiFi Hostname: ", getHostname());
  LOG("AutoReconnect: ", WiFi.getAutoReconnect());

  if (mdns_enabled) {
     if (!MDNS.begin(getHostname().c_str())) { // Start the mDNS responder for <host>.local
        LOG_ERROR("Error setting up mDNS responder for ", getHostname().c_str(), ".local");
     }
     else{
        LOG("mDNS: ", MDNS.hostname(0) + ".local");
        // Add service to MDNS-SD
        // MDNS.addService("http", "tcp", 80);
     }
  }
  connection_established = true;
}


boolean WiFiConnect::connected() {
  return (WiFi.status() == WL_CONNECTED);
}

boolean WiFiConnect::connectionEstablished() {
  return (WiFi.status() == WL_CONNECTED) && connection_established;
}

String WiFiConnect::getIP() {
  // LOG("IP address: ", WiFi.localIP().toString().c_str());
    return WiFi.localIP().toString();
}

String WiFiConnect::getHostname() {
#ifdef ESP8266
  return WiFi.hostname();
#endif
#ifdef ESP32
  return WiFi.getHostname();
#endif
}

String WiFiConnect::getMAC() {
  return WiFi.macAddress();
}

int8_t WiFiConnect::getRSSI() {
  return WiFi.RSSI();
}

int8_t WiFiConnect::getChannel() {
  return WiFi.channel();
}

String WiFiConnect::getSSID() {
  return WiFi.SSID();
}

String WiFiConnect::getBSSID() {
  return WiFi.BSSIDstr();
}

void WiFiConnect::handleClient() {
  #ifdef ESP8266
  if (mdns_enabled) {
    MDNS.update();
  }
  #endif

  unsigned long t = millis();
  if (!connection_established) {
    long period = t - test_connection_lastActivity;
    if (period >= TEST_CONNECTION_INTERVAL_NB) {
      test_connection_lastActivity = t;
      if (WiFi.status() == WL_CONNECTED) {
        initial_connection_setup();
      }
      else {
        //if (custom_callback != nullptr) {
        //  custom_callback();
        //}
        #ifdef ESP8266
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        #endif
        #ifdef ESP32
        WiFi.setSleep(WIFI_PS_NONE);
        #endif
        period = t - last_reconnect_attempt;
        bool reconnect_fl = (period >= RECONNECT_INTERVAL || period < 0 );
        if (reconnect_fl) {
          reconnect();
          last_reconnect_attempt = t;
        }
        Serial.printf("WiFiConnect: Try to %sconnect WiFi AP. WiFi status: %d - %s\n", 
          reconnect_fl ? "re-":"", WiFi.status(), getStatusString());
      }
    }
    period = t - test_connection_cb_lastActivity;
    if (period >= TEST_CONNECTION_INTERVAL_CALLBACK) {
      test_connection_cb_lastActivity = t;
        if (custom_callback != nullptr) {
          custom_callback();
        }
    }
  }
  else {
    if (WiFi.status() != WL_CONNECTED) {
        try_reconnect();
    }
    else {
      long period = t - test_try_N_lastActivity;
      if (period >= TRY_PHY_N_INTERVAL) {
        #ifdef ESP8266
        if (getPhyMode() != WIFI_PHY_MODE_11N) {
        #endif
        #ifdef ESP32
        if ((getPhyMode()&WIFI_PROTOCOL_11N) != WIFI_PROTOCOL_11N) {
        #endif
          Serial.printf("WiFiConnect: Try to re-connect WiFi AP to 11N. Current WiFi status: %d - %s, PhyMode: %d - %s\n", 
            WiFi.status(), getStatusString(), getPhyMode(), getPhyModeString());
          set_phy_11n();
          try_reconnect();
          test_try_N_lastActivity = t;
        }
      }
    }
  }
}


/*
typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
} wl_status_t;
*/

const char* WiFiConnect::getStatusString() {
  return getStatusString(WiFi.status());
}

const char* WiFiConnect::getStatusString(uint8_t status) {
   switch(status) {
    case WL_NO_SHIELD:
      return "WL_NO_SHIELD";
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
  default:
    return "UNKNOWN";
  }
}

const char* WiFiConnect::getModeString() {
  return getModeString(WiFi.getMode());
}

const char* WiFiConnect::getModeString(uint8_t mode) {
  switch(mode) {
    case WIFI_OFF:
      return "WIFI_OFF";
    case WIFI_STA:
      return "WIFI_STA";
    case WIFI_AP:
      return "WIFI_AP";
    case WIFI_AP_STA:
      return "WIFI_AP_STA";
    default:
      return "UNKNOWN";
  }
}

uint8_t WiFiConnect::getSleepMode() {
  #ifdef ESP8266
  return WiFi.getSleepMode();
  #endif
  #ifdef ESP32
  return WiFi.getSleep();
  #endif
}

const char* WiFiConnect::getSleepModeString() {
  return getSleepModeString(getSleepMode());
}

const char* WiFiConnect::getSleepModeString(uint8_t mode) {
  switch(mode) {
    #ifdef ESP8266
    case WIFI_NONE_SLEEP:
      return "WIFI_NONE_SLEEP";
    case WIFI_LIGHT_SLEEP:
      return "WIFI_LIGHT_SLEEP";
    case WIFI_MODEM_SLEEP:
      return "WIFI_MODEM_SLEEP";
    #endif
    #ifdef ESP32
    case WIFI_PS_NONE: // No power save
      return "WIFI_PS_NONE";
    case WIFI_PS_MIN_MODEM: // Minimum modem power saving. In this mode, station wakes up to receive beacon every DTIM period
      return "WIFI_PS_MIN_MODEM";
    case WIFI_PS_MAX_MODEM: //Maximum modem power saving. In this mode, interval to receive beacons is determined by the listen_interval parameter in wifi_sta_config_t
      return "WIFI_PS_MAX_MODEM";
    #endif
    default:
      return "UNKNOWN";
  }
}

uint8_t WiFiConnect::getPhyMode() {
  #ifdef ESP8266
  return WiFi.getPhyMode();
  #endif
  #ifdef ESP32
  uint8_t protocol_bitmap = 0;
  esp_err_t error_code = esp_wifi_get_protocol((wifi_interface_t)ESP_IF_WIFI_STA, &protocol_bitmap);
  if (error_code != 0) {
    char buf[100];
    buf[0]=0;
    esp_err_to_name_r(error_code, buf, 100);
    Serial.print("esp_wifi_get_protocol error code: ");
    Serial.print(error_code);
    Serial.print(" : ");
    Serial.println(buf);
    return 0;
  }
  else {
    // Serial.print("Current protocol bitmap is ");
    // Serial.println(protocol_bitmap); 
    return protocol_bitmap;
  }
  #endif
}


const char* WiFiConnect::getPhyModeString() {
  return getPhyModeString(getPhyMode());
}

const char* WiFiConnect::getPhyModeString(uint8_t mode) {
    #ifdef ESP8266
    switch(mode) {
    case WIFI_PHY_MODE_11B:
      return "WIFI_PHY_MODE_11B";
    case WIFI_PHY_MODE_11G:
      return "WIFI_PHY_MODE_11G";
    case WIFI_PHY_MODE_11N:
      return "WIFI_PHY_MODE_11N";
    default:
      return "UNKNOWN";
    }
    #endif
    #ifdef ESP32
    if ((mode&WIFI_PROTOCOL_LR) == WIFI_PROTOCOL_LR)
      return "11LR";
    if ((mode&WIFI_PROTOCOL_11N) == WIFI_PROTOCOL_11N && (mode&WIFI_PROTOCOL_11G) == WIFI_PROTOCOL_11G && (mode&WIFI_PROTOCOL_11B) == WIFI_PROTOCOL_11B)
      return "11B|11G|11N";
    if ((mode&WIFI_PROTOCOL_11G) == WIFI_PROTOCOL_11G && (mode&WIFI_PROTOCOL_11B) == WIFI_PROTOCOL_11B)
      return "11B|11G";
    if ((mode&WIFI_PROTOCOL_11B) == WIFI_PROTOCOL_11B)
      return "11B";
    return "not decoded";
    #endif

}

String WiFiConnect::getGatewayIP() {
  return WiFi.gatewayIP().toString();
}

IPAddress WiFiConnect::getGatewayIPAddress() {
  return WiFi.gatewayIP();
}

String WiFiConnect::getNetworkID() {

  #ifdef ESP32
  return  WiFi.networkID().toString();
  #endif

  #ifdef ESP8266
	IPAddress networkID;
  IPAddress gatewayIp = WiFi.gatewayIP();
  IPAddress subnet = WiFi.subnetMask();
	for (size_t i = 0; i < 4; i++) {
		networkID[i] = subnet[i] & gatewayIp[i];
  }
  return  networkID.toString();
  #endif

}

IPAddress WiFiConnect::getNetworkIDAddress() {

  #ifdef ESP32
  return  WiFi.networkID();
  #endif

  #ifdef ESP8266
	IPAddress networkID;
  IPAddress gatewayIp = WiFi.gatewayIP();
  IPAddress subnet = WiFi.subnetMask();
	for (size_t i = 0; i < 4; i++) {
		networkID[i] = subnet[i] & gatewayIp[i];
  }
  return  networkID;
  #endif

}


boolean WiFiConnect::isIPAddressSet(IPAddress ip) {
  #ifdef ESP32
  return  (uint32_t)ip != IPADDR_NONE && (uint32_t)ip != IPADDR_ANY;
  #endif

  #ifdef ESP8266
  return ip.isSet();
  #endif
}


void WiFiConnect::debug_info() {
  Serial.println("\nWiFiConnect.cpp:\n================");
  Serial.printf("WiFi status: %d - %s\n", WiFi.status(), getStatusString());
  Serial.printf("AutoReconnect: %d\n", WiFi.getAutoReconnect());
  Serial.printf("Mode: %d - %s\n", WiFi.getMode(), getModeString());
  Serial.printf("Sleep Mode: %d - %s\n", getSleepMode(), getSleepModeString());
  Serial.printf("Phy Mode: %d - %s\n", getPhyMode(), getPhyModeString());
  #ifdef ESP8266
  Serial.printf("ListenInterval: %d\n", WiFi.getListenInterval());
  #endif
  Serial.printf("AP SSID: %s\n", WiFi.SSID().c_str()); 
  Serial.printf("MAC: %s\n", WiFi.macAddress().c_str()); 
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  // IPAddress ip= WiFi.localIP();
  // Serial.printf("IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
  Serial.printf("DNS IP0: %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("DNS IP1: %s\n", WiFi.dnsIP(1).toString().c_str());
  Serial.printf("Gateway IP: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("Broadcast IP: %s\n", WiFi.broadcastIP().toString().c_str());
  Serial.printf("NetworkID: %s\n", getNetworkID().c_str());
  #ifdef ESP32
  Serial.printf("IP V6: %s\n", WiFi.localIPv6().toString().c_str());
  #endif
  Serial.printf("WiFi Hostname: %s\n", getHostname().c_str());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
  Serial.printf("Channel: %d\n", WiFi.channel());
  //Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
  uint8_t *bssid = WiFi.BSSID();;
  Serial.printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  Serial.printf("Connection established: %s\n", connection_established ? "true" : "false");
  Serial.printf("Last reconnect attempt: %lu\n", last_reconnect_attempt);
  Serial.printf("Module Hostname: %s\n", hostname != NULL ? hostname : "NULL");

  Serial.printf("Module generate_hostname: %s\n", generate_hostname ? "true" : "false");
  Serial.printf("Module mdns_enabled: %s\n", mdns_enabled ? "true" : "false");
  Serial.printf("Module none_blocking: %s\n", none_blocking ? "true" : "false");
  WiFi.printDiag(Serial);
  if (mdns_enabled) {
    mdnsClient.debug_info();
  }
}

void WiFiConnect::try_reconnect() {
  connection_established = false;
  last_reconnect_attempt = millis();
  test_try_N_lastActivity = last_reconnect_attempt;
  WiFi.reconnect();
}

// Only set phy mode, do not call recconect in this function
void WiFiConnect::set_phy_11b() {
  #ifdef ESP8266
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  #endif
  #ifdef ESP32
  esp_err_t error_code = esp_wifi_set_protocol((wifi_interface_t)ESP_IF_WIFI_STA,WIFI_PROTOCOL_11B);
  if (error_code != 0) {
    char buf[100];
    buf[0]=0;
    esp_err_to_name_r(error_code, buf, 100);
    Serial.print("esp_wifi_set_protocol error code: ");
    Serial.print(error_code);
    Serial.print(" : ");
    Serial.println(buf);
    return;
  }
  #endif
  // try_reconnect(); - do not call recconect in this function
}

// Only set phy mode, do not call recconect in this function
void WiFiConnect::set_phy_11g() {
  #ifdef ESP8266
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  #endif
  #ifdef ESP32
  esp_err_t error_code = esp_wifi_set_protocol((wifi_interface_t)ESP_IF_WIFI_STA, WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11B);
  if (error_code != 0) {
    char buf[100];
    buf[0]=0;
    esp_err_to_name_r(error_code, buf, 100);
    Serial.print("esp_wifi_set_protocol error code: ");
    Serial.print(error_code);
    Serial.print(" : ");
    Serial.println(buf);
    return;
  }
  #endif
  // try_reconnect(); - do not call recconect in this function
}

// Only set phy mode, do not call recconect in this function
void WiFiConnect::set_phy_11n() {
  #ifdef ESP8266
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  #endif
  #ifdef ESP32
  esp_err_t error_code = esp_wifi_set_protocol((wifi_interface_t)ESP_IF_WIFI_STA, WIFI_PROTOCOL_11N|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11B);
  if (error_code != 0) {
    char buf[100];
    buf[0]=0;
    esp_err_to_name_r(error_code, buf, 100);
    Serial.print("esp_wifi_set_protocol error code: ");
    Serial.print(error_code);
    Serial.print(" : ");
    Serial.println(buf);
    return;
  }
  #endif
  // try_reconnect(); - do not call recconect in this function
}

// Only set phy mode, do not call recconect in this function
void WiFiConnect::set_phy_lr() {
  #ifdef ESP8266
  Serial.print("LR not supported ESP8266");
  #endif
  #ifdef ESP32
  esp_err_t error_code = esp_wifi_set_protocol((wifi_interface_t)ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR);
  if (error_code != 0) {
    char buf[100];
    buf[0]=0;
    esp_err_to_name_r(error_code, buf, 100);
    Serial.print("esp_wifi_set_protocol error code: ");
    Serial.print(error_code);
    Serial.print(" : ");
    Serial.println(buf);
    return;
  }
  #endif
  // try_reconnect(); - do not call recconect in this function
}