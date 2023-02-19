#ifndef ESPConnect_h
#define ESPConnect_h

#include <Arduino.h>
#include <functional>

#if defined(ESP8266)
#include "ESP8266WiFi.h"
#include "ESPAsyncTCP.h"
#include "WiFiClient.h"
#elif defined(ESP32)
#include "AsyncTCP.h"
#include "Preferences.h"
#include "WiFi.h"
#include "WiFiClient.h"
#endif

#include "DNSServer.h"
#include "ESPAsyncWebServer.h"
#include "espconnect_webpage.h"

/* Library Default Settings */
#define ESPCONNECT_DEBUG           1

#define DEFAULT_CONNECTION_TIMEOUT 30000
#define DEFAULT_PORTAL_TIMEOUT     180000

#if ESPCONNECT_DEBUG == 1
#define ESPCONNECT_SERIAL(fmt, ...) log_i("[ESPConnect][%lu] " fmt, millis(), ##__VA_ARGS__)
#else
#define ESPCONNECT_SERIAL(x)
#endif

class ESPConnectClass
{

private:
    DNSServer *_dns = nullptr;
    AsyncWebServer *_server = nullptr;

    String _auto_connect_ssid = "";
    String _auto_connect_password = "";
    unsigned long _auto_connect_timeout = DEFAULT_PORTAL_TIMEOUT;

    String _sta_ssid = "";
    String _sta_password = "";

private:
    void load_sta_credentials();
    int save_sta_credentials(const String &ssid, const String &password);

    // Start Captive portal
    bool start_portal(bool isStoreCredential = true);

public:
    // Check if ESPConnect was configured before
    bool isConfigured();

    // Set Custom AP
    void autoConnect(const char *ssid, const char *password = "", unsigned long timeout = DEFAULT_PORTAL_TIMEOUT);

    // Connect to Saved WiFi Credentials
    bool begin(AsyncWebServer *server, unsigned long timeout = DEFAULT_CONNECTION_TIMEOUT);

    // Manually start portal config
    bool startPortal(AsyncWebServer *server);

    // Erase Saved WiFi Credentials
    bool erase();

    /*
      Data Getters
    */

    // Return true / false depending of connection status
    bool isConnected();

    // Gets SSID of connected endpoint
    String getSSID();
    String getPassword();
};

extern ESPConnectClass ESPConnect;

#endif
