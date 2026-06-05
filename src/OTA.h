#ifndef _SN_OTA_CLIENT_H_
#define _SN_OTA_CLIENT_H_

class OTAClient {
  
  public:
    OTAClient();
    void begin(const char *host, const char *passwd);
  
    String getHostname();
    void debug_info(void);

    void handleClient();
    
};

extern OTAClient otaClient;

#endif
