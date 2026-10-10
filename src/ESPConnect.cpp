#include "ESPConnect.h"

#define ESPCONNECT_PREFS_NAMESPACE "espconnect"

// Per-entry JSON overhead (`{"name":"","type":""},`) + longest auth name, and the SSID limit
#define SCAN_JSON_ENTRY_OVERHEAD   48
#define SCAN_JSON_SSID_MAX_LEN     32
#define SCAN_JSON_HIDDEN_ENTRY     "{\"name\":\"\",\"type\": \"WPA2_PSK\",\"hidden\":true}"

#define JSON_CONTROL_CHAR_LIMIT    0x20
#define JSON_UNICODE_ESCAPE_DIGITS 4

/*
  Auth-mode name shown by the portal. The portal only distinguishes OPEN (no password field) and
  WPA2_ENTERPRISE (username field), so every enterprise variant is reported as WPA2_ENTERPRISE
  (the firmware connects with PEAP for any stored username). Unknown values fall back to WPA2_PSK.
*/
static const char *auth_mode_name(wifi_auth_mode_t mode)
{
    switch (mode)
    {
    case WIFI_AUTH_OPEN:
        return "OPEN";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
    case WIFI_AUTH_WPA3_ENT_192:
    case WIFI_AUTH_WPA3_ENTERPRISE:
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
    case WIFI_AUTH_WPA_ENTERPRISE:
        return "WPA2_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:
    case WIFI_AUTH_WPA3_EXT_PSK:
    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
        return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2_WPA3_PSK";
    case WIFI_AUTH_WAPI_PSK:
        return "WAPI_PSK";
    case WIFI_AUTH_OWE:
        return "OWE";
    default:
        return "WPA2_PSK";
    }
}

/*
  Append `{"name":"<ssid>","type":"<type>"},` to json, escaping the SSID for JSON
*/
static void append_ap_json(String &json, const String &ssid, const char *type)
{
    json += "{\"name\":\"";
    for (unsigned int i = 0; i < ssid.length(); ++i)
    {
        const char ch = ssid[i];
        if (ch == '\\' || ch == '"')
        {
            json += '\\';
            json += ch;
        }
        else if ((uint8_t)ch < JSON_CONTROL_CHAR_LIMIT)
        {
            char escaped[JSON_UNICODE_ESCAPE_DIGITS + 3];
            snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned int)(uint8_t)ch);
            json += escaped;
        }
        else
        {
            json += ch;
        }
    }
    json += "\",\"type\":\"";
    json += type;
    json += "\"},";
}

/*
  Check if ESPConnect was configured before
*/
bool ESPConnectClass::isConfigured()
{
    if (!_sta_loaded)
        _sta_loaded = load_sta_credentials();
    return _sta_ssid.length() > 0;
}

/*
  Loads STA Credentials into memory. Returns false if the NVS namespace could not be opened.
*/
bool ESPConnectClass::load_sta_credentials()
{
    // Get saved WiFi Credentials
    Preferences preferences;
    if (!preferences.begin(ESPCONNECT_PREFS_NAMESPACE, false))
        return false;
    _sta_ssid = preferences.getString("ssid", "");
    _sta_username = preferences.getString("username", "");
    _sta_password = preferences.getString("password", "");
    preferences.end();
    return true;
}

/*
  Saves STA Credentials into flash
*/
bool ESPConnectClass::save_sta_credentials(const String &ssid, const String &username, const String &password)
{
    // Save saved WiFi Credentials
    Preferences preferences;
    if (!preferences.begin(ESPCONNECT_PREFS_NAMESPACE, false))
        return false;
    preferences.putString("ssid", ssid);
    preferences.putString("username", username);
    preferences.putString("password", password);
    preferences.end();
    return true;
}

/*
  Manually Start Captive Portal and Attach DNS & Webserver
*/
bool ESPConnectClass::startPortal(bool isStoreCredential)
{
    ESPCONNECT_SERIAL("Starting Captive Portal\n");
    _portal_configured = false;
    _portal_store_credential = isStoreCredential;

    // Try Connecting Station
    WiFi.mode(WIFI_AP_STA);
    // Start Access Point
    WiFi.softAP(_auto_connect_ssid.c_str(), _auto_connect_password.c_str());

    // Start our DNS Server
    _dns = new DNSServer();
    _dns->setErrorReplyCode(DNSReplyCode::NoError);
    _dns->start(53, "*", WiFi.softAPIP());

    auto &scanGET = _server->on(
        "/espconnect/scan",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
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

            ESPCONNECT_SERIAL("Scan complete: %d ap\n", n);
            String json;
            json.reserve(1 + n * (SCAN_JSON_ENTRY_OVERHEAD + SCAN_JSON_SSID_MAX_LEN) + sizeof(SCAN_JSON_HIDDEN_ENTRY) + 1);
            json += '[';
            for (uint8_t i = 0; i < n; ++i)
                append_ap_json(json, WiFi.SSID(i), auth_mode_name(WiFi.encryptionType(i)));
            json += SCAN_JSON_HIDDEN_ENTRY;
            json += ']';
            WiFi.scanDelete();
            if (WiFi.scanComplete() == WIFI_SCAN_FAILED)
            {
                WiFi.scanNetworks(true);
            }
            request->send(200, "application/json", json);
        });

    // Accept incomming WiFi Credentials
    auto &connectPOST = _server->on(
        "/espconnect/connect",
        HTTP_POST,
        [this](AsyncWebServerRequest *request)
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
            if (!_portal_store_credential || save_sta_credentials(ssid, username, password))
            {
                _sta_ssid = ssid;
                _sta_username = username;
                _sta_password = password;
                _sta_loaded = true;

                request->send(200, "application/json", "{\"message\":\"Credentials Saved. Rebooting...\"}");
                // Last: publishes the credentials above to the thread polling in startPortal()
                _portal_configured = true;
            }
            else
            {
                request->send(500, "application/json", "{\"message\":\"Error while saving WiFi Credentials\"}");
            }
        });

    auto &indexGET = _server->on(
        "/espconnect",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html", ESPCONNECT_HTML, ESPCONNECT_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

    _server->onNotFound(
        [](AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/html", ESPCONNECT_HTML, ESPCONNECT_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });

    auto &portalRewrite = _server->rewrite("/", "/espconnect").setFilter(ON_AP_FILTER);

    // Begin Webserver
    _server->begin();

    const unsigned long lastMillis = millis();
    while ((unsigned long)(millis() - lastMillis) < _auto_connect_timeout && !_portal_configured)
    {
        _dns->processNextRequest();
        delay(PORTAL_POLL_INTERVAL_MS);
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
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    return _portal_configured;
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
    const bool status = save_sta_credentials("", "", "");
    if (status)
    {
        _sta_ssid = "";
        _sta_username = "";
        _sta_password = "";
        _sta_loaded = true;
    }
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
    _sta_loaded = true;

    return save_sta_credentials(ssid, username, password);
}

/*
  Return Connection Status
*/
bool ESPConnectClass::isConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

ESPConnectClass ESPConnect;
