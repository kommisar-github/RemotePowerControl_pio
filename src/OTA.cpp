// OTA
//
// Versions:
//   v0.1 20220115
//   v0.2 20220712 - Enhancements
//   v0.3 20221014 - Added debug_info(), log fixes
//   v0.4 20221219 - Added MDNS to debug_info()
//   v0.5 20230526 - Added MdnsClient.h

#include <ArduinoOTA.h>
#include "OTA.h"

#if defined(ESP8266) && !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MDNS)
#include <ESP8266mDNS.h>
#endif
#if defined(ESP32) && !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MDNS)
#include <ESPmDNS.h>
#endif

#include "MdnsClient.h"
#include "Log.h"

OTAClient otaClient; // Created OTAClient instance

OTAClient::OTAClient()
{
}

void OTAClient::begin(const char *host, const char *passwd)
{
  ArduinoOTA.setHostname(host);
  ArduinoOTA.setPassword(passwd);

  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    LOG("Start OTA updating ", type.c_str()); });

  ArduinoOTA.onEnd([]()
                   { LOG("\nEnd OTA"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
    char buf[20];
    sprintf(buf, "Progress: %u%%", (progress / (total / 100)));
    LOG(buf); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    LOG_ERROR("Error: ", String(error));
    if (error == OTA_AUTH_ERROR) LOG_ERROR("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) LOG_ERROR("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) LOG_ERROR("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) LOG_ERROR("Receive Failed");
    else if (error == OTA_END_ERROR) LOG_ERROR("End Failed"); });

  ArduinoOTA.begin();

  LOG("OTA ready");
#if defined(ESP8266) && !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MDNS)
  if (MDNS.isRunning())
  {
    LOG("OTA: mDNS: ", getHostname() + ".local");
  }
#endif
#ifdef ESP32
  // if (MDNS.hostname() != NULL) {
  LOG("OTA: mDNS: ", getHostname() + ".local");
//}
#endif

}

void OTAClient::handleClient()
{
  ArduinoOTA.handle();
}

String OTAClient::getHostname()
{
  return ArduinoOTA.getHostname();
}

void OTAClient::debug_info(void)
{
  Serial.println("\nOTAClient.cpp:\n==============");
  Serial.printf("OTA mDNS: %s\n", ArduinoOTA.getHostname().c_str());
  // Serial.printf("MDNS isRunning: %s\n", MDNS.isRunning() ? "true" : "false");
  mdnsClient.debug_info();
}
