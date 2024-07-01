#include "ESPConnect.h"

/*
  Check if ESPConnect was configured before
*/
bool ESPConnectClass::isConfigured()
{
    load_sta_credentials();
    return _sta_ssid.length() > 0;
}

/*
  Loads STA Credentials into memory
*/
void ESPConnectClass::load_sta_credentials()
{
    // Get saved WiFi Credentials
    Preferences preferences;
    preferences.begin("espconnect", false);
    _sta_ssid = preferences.getString("ssid", "");
    _sta_username = preferences.getString("username", "");
    _sta_password = preferences.getString("password", "");
    preferences.end();
}

/*
  Saves STA Credentials into memory
*/
bool ESPConnectClass::save_sta_credentials(const String &ssid, const String &username, const String &password)
{
    // Save saved WiFi Credentials
    Preferences preferences;
    bool status = preferences.begin("espconnect", false);
    preferences.putString("ssid", ssid.c_str());
    preferences.putString("username", username.c_str());
    preferences.putString("password", password.c_str());
    preferences.end();
    return status;
}

/*
  Manually Start Captive Portal and Attach DNS & Webserver
*/
bool ESPConnectClass::startPortal(bool isStoreCredential)
{
    bool configured = false;
    ESPCONNECT_SERIAL("Starting Captive Portal\n");
    // Try Connecting Station
    WiFi.mode(WIFI_AP_STA);
    // Start Access Point
    WiFi.softAP(_auto_connect_ssid.c_str(), _auto_connect_password.c_str());

    // Start our DNS Server
    _dns = new DNSServer();
    _dns->setErrorReplyCode(DNSReplyCode::NoError);
    _dns->start(53, "*", WiFi.softAPIP());

    auto scanGET = _server->on(
        "/espconnect/scan",
        HTTP_GET,
        [&](AsyncWebServerRequest *request)
        {
            String json = "[";
            int n = WiFi.scanComplete();
            if (n == WIFI_SCAN_FAILED)
            {
                WiFi.scanNetworks(true);
                return request->send(202);
            }
            else if (n == WIFI_SCAN_RUNNING)
            {
                return request->send(202);
            }
            else
            {
                ESPCONNECT_SERIAL("Scan complete: %d ap\n", n);
                for (uint8_t i = 0; i < n; ++i)
                {
                    String ssid = WiFi.SSID(i);
                    // Escape invalid characters
                    ssid.replace("\\", "\\\\");
                    ssid.replace("\"", "\\\"");
                    json += "{";
                    json += "\"name\":\"" + ssid + "\",";
                    json += "\"type\":\"" + String(AUTH_MODE_NAMES[WiFi.encryptionType(i)]) + "\"";
                    json += "},";
                }
                json += "{\"name\":\"\",\"type\": \"WPA2_PSK\",\"hidden\":true}";
                WiFi.scanDelete();
                if (WiFi.scanComplete() == WIFI_SCAN_FAILED)
                {
                    WiFi.scanNetworks(true);
                }
            }
            json += "]";
            request->send(200, "application/json", json);
            json = String();
        });

    // Accept incomming WiFi Credentials
    auto connectPOST = _server->on(
        "/espconnect/connect",
        HTTP_POST,
        [&](AsyncWebServerRequest *request)
        {
            // Get FormData
            String ssid = request->hasParam("ssid", true) ? request->getParam("ssid", true)->value().c_str() : "";
            String username = request->hasParam("username", true) ? request->getParam("username", true)->value().c_str() : "";
            String password = request->hasParam("password", true) ? request->getParam("password", true)->value().c_str() : "";

            if (ssid.length() <= 0)
            {
                return request->send(403, "application/json", "{\"message\":\"Invalid SSID\"}");
            }

            if (ssid.length() > 32 || password.length() > 64)
            {
                return request->send(403, "application/json", "{\"message\":\"Credentials exceed character limit of 32 & 64 respectively.\"}");
            }

            // Save WiFi Credentials in Flash
            bool stored_status = false;
            if (isStoreCredential)
                stored_status = save_sta_credentials(ssid, username, password);
            if (stored_status)
            {
                configured = true;
                _sta_ssid = ssid;
                _sta_username = username;
                _sta_password = password;

                request->send(200, "application/json", "{\"message\":\"Credentials Saved. Rebooting...\"}");
            }
            else
            {
                request->send(500, "application/json", "{\"message\":\"Error while saving WiFi Credentials\"}");
            }
        });

    auto indexGET = _server->on(
        "/espconnect",
        HTTP_GET,
        [&](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html", ESPCONNECT_HTML, ESPCONNECT_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

    _server->onNotFound(
        [&](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html", ESPCONNECT_HTML, ESPCONNECT_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

    auto portalRewrite = _server->rewrite("/", "/espconnect").setFilter(ON_AP_FILTER);

    // Begin Webserver
    _server->begin();

    unsigned long lastMillis = millis();
    while ((unsigned long)(millis() - lastMillis) < _auto_connect_timeout && !configured)
    {
        _dns->processNextRequest();
        yield();
    }

    _server->removeHandler(&indexGET);
    _server->removeHandler(&scanGET);
    _server->removeHandler(&connectPOST);
    _server->removeRewrite(&portalRewrite);
    _server->onNotFound(
        [](AsyncWebServerRequest *request)
        {
            request->send(404);
        });

    _dns->stop();
    delete _dns;
    _dns = nullptr;

    ESPCONNECT_SERIAL("Closed Portal\n");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    return configured;
}

/*
  Set Custom AP Credentials
*/
void ESPConnectClass::setupPortal(AsyncWebServer *server, const char *ssid, const char *password, unsigned long timeout)
{
    _server = server;
    _auto_connect_ssid = ssid;
    _auto_connect_password = password;
    _auto_connect_timeout = timeout;
}

/*
  Erase Stored WiFi Credentials
*/
bool ESPConnectClass::erase()
{
    Preferences preferences;
    bool status = preferences.begin("espconnect", false);
    preferences.putString("ssid", "");
    preferences.putString("username", "");
    preferences.putString("password", "");
    preferences.end();
    WiFi.disconnect(true, true);
    return status;
}

/*
  Manually set wifi credentials and store in flash
*/
bool ESPConnectClass::setCredentials(const String &ssid, const String &username, const String &password)
{
    _sta_ssid = ssid;
    _sta_username = username;
    _sta_password = password;

    return save_sta_credentials(ssid, username, password);
}

/*
  Return Connection Status
*/
bool ESPConnectClass::isConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

String ESPConnectClass::getSSID()
{
    return _sta_ssid;
}

String ESPConnectClass::getUsername()
{
    return _sta_username;
}

String ESPConnectClass::getPassword()
{
    return _sta_password;
}

ESPConnectClass ESPConnect;
