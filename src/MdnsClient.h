#pragma once

class MdnsClient {

  public:
    MdnsClient();

    void findAllServices();
    // void printService(stcMDNSService *pService);
    void findService(const char * service, const char * proto);

    void debug_info();

};

extern MdnsClient mdnsClient;

