#ifndef ESPConnect_h
#define ESPConnect_h

#include <Arduino.h>
#include <atomic>
#include <functional>

#include "AsyncTCP.h"
#include "Preferences.h"
#include "WiFi.h"
#include "WiFiClient.h"
#include "esp_eap_client.h"

#include "DNSServer.h"
#include "ESPAsyncWebServer.h"
#include "espconnect_webpage.h"

/* Library Default Settings */
#define ESPCONNECT_DEBUG           1

#define DEFAULT_CONNECTION_TIMEOUT 30000
#define DEFAULT_PORTAL_TIMEOUT     180000

// Sleep between DNS polls while the portal runs; lets lower-priority tasks and the idle task run.
#define PORTAL_POLL_INTERVAL_MS    1

#if ESPCONNECT_DEBUG == 1
#define ESPCONNECT_SERIAL(fmt, ...) log_i("[ESPConnect][%lu] " fmt, millis(), ##__VA_ARGS__)
#else
#define ESPCONNECT_SERIAL(fmt, ...)
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
    String _sta_username = "";
    String _sta_password = "";
    bool _sta_loaded = false;

    // Set by the async web server task once credentials are accepted; polled by startPortal()
    std::atomic<bool> _portal_configured{false};
    bool _portal_store_credential = true;

private:
    bool load_sta_credentials();
    bool save_sta_credentials(const String &ssid, const String &username, const String &password);

public:
    // Check if ESPConnect was configured before
    bool isConfigured();

    // Manually start captive portal
    bool startPortal(bool isStoreCredential = true);

    // Set Custom AP
    void setupPortal(AsyncWebServer *server, const char *ssid, const char *password = "", unsigned long timeout = DEFAULT_PORTAL_TIMEOUT);

    // Erase Saved WiFi Credentials
    bool erase();

    // Manually config wifi credentials
    bool setCredentials(const String &ssid, const String &username, const String &password);
    bool setCredentials(const String &ssid, const String &password) { return setCredentials(ssid, "", password); }

    /*
      Data Getters
    */

    // Return true / false depending of connection status
    bool isConnected();

    // Gets SSID of connected endpoint
    const String &getSSID() const { return _sta_ssid; }
    const String &getUsername() const { return _sta_username; }
    const String &getPassword() const { return _sta_password; }
};

extern ESPConnectClass ESPConnect;

#endif
