#include "network.h"

#include "log_manager.h"

void Network::init(String ssid, String password)
{
    LOG_INFO("NET", "Scanning WiFi networks...");
    int n = WiFi.scanNetworks();
    LOG_INFO("NET", "WiFi scan completed");
    if(n == 0) {
        LOG_WARN("NET", "No networks found");
    } else {
        LOG_INFO("NET", "Found " + String(n) + " networks");
        for(int i = 0; i < n; ++i) {
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }
    }
    Serial.println("");
    Serial.print("Connecting: ");
    Serial.print(ssid.c_str());
    Serial.print(" @");
    Serial.println(password.c_str());

    WiFi.begin(ssid.c_str(), password.c_str());
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

unsigned int Network::getBilibiliFans(String uid)
{
    String fansCount = "";
    HTTPClient http;
    http.begin("http://api.bilibili.com/x/relation/stat?vmid=" + uid);

    // start connection and send HTTP headerFFF
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            int pos = (payload.indexOf("follower"));
            fansCount = payload.substring(pos + 10, payload.length() - 2);
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();

    return atol(fansCount.c_str());
}
