#include "WebSServer.h"
#include "strings1.h"
#include "log.h"
//#include "MqttClient.h"
#include <ArduinoJson.h>                // JSON parser

WebSServer webSServer;  // Created WebServer instance

WebSServer::WebSServer() {
    #if defined(ESP8266)
    _server = new ESP8266WebServer(_port);
    #endif
    #if defined(ESP32)
    _server = new WebServer(_port);
    #endif
}

void WebSServer::begin(PGM_P version, RSwitch **rSwitch) {

  this->version = version;
  this->rSwitch = rSwitch;
  
  // WebServer handlers
  _server->on("/", HTTP_GET, [this]() { handleRootFrame(); });
  _server->on("/setswitch", HTTP_GET, [this]() { handleSetSwitch(); });
  _server->on("/getstatus", HTTP_GET, [this]() { handleGetStatus(); });
  _server->onNotFound([this]() {
      _server->send(404, "text/plain", "404: Not found");
  });

  _server->begin();   // Actually start the server
  LOG("HTTP server started");
}

void WebSServer::handleClient() {
  _server->handleClient();  // Listen for HTTP requests from clients
}

void WebSServer::handleRootFrame() {
  String page;
  page += FPSTR(PAGE_BUTTONS);
  page.replace("device1", "DEVICE1");
  _server->send(200, "text/html", page);
  LOG("handleRootFrame()");
}

/*
  for (int i=0; i<3; i++) {
    if (rSwitch[i] != NULL) {
      rSwitch[i]->setCallback(power_status_change_callback);
      Serial.printf("Switch %d Switch GPIO: %d Detect GPIO: %d\n", i+1, rSwitch[i]->getSwitchGPIO(), rSwitch[i]->getDetectGPIO());
    }
  }
*/

/*
void WebSServer::handleGetRoot() {
  String page;
  page += FPSTR(PAGE_ROOT);
  page.replace(FPSTR(TAG_VER), FPSTR(version));
  if (boiler.getStatus()) {
    page.replace(FPSTR(TAG_STAT), "ON");
  }
  else {
    page.replace(FPSTR(TAG_STAT), "OFF");
  }
  // _server->sendHeader("Location","/");
  _server->send(200, "text/html", page);
  LOG("handleGetRoot()");
}
*/

void WebSServer::handleSetSwitch() {
  _server->send(200, "text/plain", "OK");
  String switchId = _server->arg("switch");
  if (!switchId.isEmpty()) {
    String status = _server->arg("status");
    if (!status.isEmpty()) {
      int switch_id = switchId[0] - '0';
      int stat = status[0] - '0';
      if (switch_id > -1 && switch_id < 3) {
        if (rSwitch[switch_id] != NULL) {
          if (stat == 0 || stat == 1) {
            LOG("WEB: setSwitch: ", (uint8_t) stat);
            rSwitch[switch_id]->setSwitch(stat);
          }
          else { 
            if (stat == 3) {
              LOG("WEB: longPress - start");
              rSwitch[switch_id]->longPress();
              LOG("WEB: longPress - end");
            }
          }
        }
      }
    }
  }
}

// JSON and temp buffers
static DynamicJsonDocument doc(256); // JSON
#define BUFFER_SIZE  (256)
static char BUF[BUFFER_SIZE];

void WebSServer::handleGetStatus() {
  doc.clear();
  // JsonObject root = doc.to<JsonObject>();
  JsonArray devs = doc.createNestedArray("devs");

  for (int i=0; i<3; i++) {
    JsonObject obj = devs.createNestedObject();
    obj["id"] = i;
    if (rSwitch[i] != NULL) {
      POWER_STATUS status = rSwitch[i]->getPowerStatus();
      char* stat = rSwitch[i]->decodePowerStat(status);
      boolean on = (status == POWER_ON || status == POWERING_ON) ? true : false;
      obj["stat"] = stat;
      obj["on"] = on;
    }
    else {
      obj["stat"] = "DISABLED";
      obj["on"] = false;
    }
  }
  serializeJson(doc, BUF, BUFFER_SIZE);
  _server->send(200, "text/plain", BUF);
  LOG("WEB: Send getStatus response:", BUF);

}


//void WebSServer::handleGetLog() {
//  _server->send(404, "text/plain", "log file...\n");
//}

void WebSServer::send_status() {
  // mqttClient.publish(status_topic, boiler.getStatus() ? "1" : "0", true);
}
