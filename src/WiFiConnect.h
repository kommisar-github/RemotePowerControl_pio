#ifndef _SN_WIFI_CONNECT_H_
#define _SN_WIFI_CONNECT_H_

#include "IPAddress.h"

#if defined(ESP8266)
// #include <ESP8266WiFiType.h>
// #include <ESP8266WiFiGeneric.h>
#endif

#if defined(ESP32)
//#include <WiFiType.h>

// Cloned from ESP8266 <lwip/ip4_addr.h>
/** 255.255.255.255 */
#define IPADDR_NONE         ((u32_t)0xffffffffUL)
/** 127.0.0.1 */
#define IPADDR_LOOPBACK     ((u32_t)0x7f000001UL)
/** 0.0.0.0 */
#define IPADDR_ANY          ((u32_t)0x00000000UL)
/** 255.255.255.255 */
#define IPADDR_BROADCAST    ((u32_t)0xffffffffUL)
#endif


class WiFiConnect {

  public:
    WiFiConnect();

    boolean begin(const char* ssid, const char* pwd);
    boolean begin_nb(const char* ssid, const char* pwd);
    boolean begin(const char* ssid, const char* pwd, const char* hostname);
    boolean begin_nb(const char* ssid, const char* pwd, const char* hostname);
    boolean begin(const char* ssid, const char* pwd, boolean generate_hostname);
    boolean begin_nb(const char* ssid, const char* pwd, boolean generate_hostname);
    boolean begin(const char* ssid, const char* pwd, boolean generate_hostname, boolean mdns_enabled);
    boolean begin_nb(const char* ssid, const char* pwd, boolean generate_hostname, boolean mdns_enabled);
    boolean begin(const char* ssid, const char* pwd, const char *hostname, boolean mdns_enabled);
    boolean begin_nb(const char* ssid, const char* pwd, const char *hostname, boolean mdns_enabled);
    boolean begin(const char* ssid, const char* pwd, boolean generate_hostname, void (*custom_callback)(void));
    boolean begin_nb(const char* ssid, const char* pwd, boolean generate_hostname, void (*custom_callback)(void));
    boolean begin(const char* ssid, const char* pwd, const char *hostname, void (*custom_callback)(void));
    boolean begin_nb(const char* ssid, const char* pwd, const char *hostname, void (*custom_callback)(void));
    boolean begin(const char* ssid, const char* pwd, const char *hostname, void (*custom_callback)(void), boolean generate_hostname, boolean mdns_enabled, boolean none_blocking);


    boolean connected();
    boolean connectionEstablished();
    String getIP();
    String getHostname();
    String getMAC();
    int8_t getRSSI();
    int8_t getChannel();
    String getBSSID();
    String getSSID();
    String getNetworkID();
    IPAddress getNetworkIDAddress();
    String getGatewayIP();
    IPAddress getGatewayIPAddress();
    boolean isIPAddressSet(IPAddress ip);

    void handleClient();

    void reconnect();
    
    void debug_info();
    void try_reconnect();
    void set_phy_11b();
    void set_phy_11g();
    void set_phy_11n();
    void set_phy_lr();

    const char* getStatusString();
    const char* getStatusString(uint8_t status);
    const char* getModeString();
    const char* getModeString(uint8_t mode);
    uint8_t getSleepMode();
    const char* getSleepModeString();
    const char* getSleepModeString(uint8_t mode);
    uint8_t getPhyMode();
    const char* getPhyModeString();
    const char* getPhyModeString(uint8_t mode);

  private:
    const char* ssid = nullptr;
    const char* pwd = nullptr;
    const char* hostname = nullptr;
    boolean generate_hostname = false;
    boolean mdns_enabled = false;           // Enable mDNS by WiFiClient. Not required in case OTA Client is used. (default flse)
    boolean none_blocking = false;
    boolean connection_established = false;
    unsigned long last_reconnect_attempt = 0L;
    unsigned long test_try_N_lastActivity = 0L;
    void (*custom_callback)(void) = nullptr;
    uint8_t reconnect_attempt_count = 0;
    unsigned long test_connection_lastActivity = 0L;
    unsigned long test_connection_cb_lastActivity = 0L;

    void initial_connection_setup();
    void test_connection();

    //WiFiEventHandler gotIpEventHandler;
    //WiFiEventHandler connectedEventHandler;
    //WiFiEventHandler disconnectedEventHandler;
    //WiFiEventHandler dhcpTimeoutEventHandler;


};

extern WiFiConnect wifiConnect;

#endif
