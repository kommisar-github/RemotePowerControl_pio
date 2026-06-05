#ifndef _SN_WEB_SERVER_H_
#define _SN_WEB_SERVER_H_

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#endif
#if defined(ESP32)
#include <WebServer.h>
#endif

#include <RSwitch.h>

class WebSServer {

  public:
    WebSServer();

    void begin(PGM_P version, RSwitch **rSwitch);
    void handleClient();

  private:
    int _port = 80;
    #if defined(ESP8266)
    ESP8266WebServer *_server;    // Create a webserver object that listens for HTTP request on port 80
    #endif
    #if defined(ESP32)
    WebServer *_server;
    #endif

    PGM_P version = nullptr;
    RSwitch **rSwitch = nullptr;

    void handleRootFrame();
    void handleSetSwitch();
    void handleGetStatus();

    void send_status();
};

extern WebSServer webSServer;

#endif
