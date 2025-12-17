#ifndef NETWORK_H
#define NETWORK_H

#include <HTTPClient.h>
#include <WiFi.h>

class Network
{
private:
public:
    void init(String ssid, String password);
    unsigned int getBilibiliFans(String url);
};

#endif