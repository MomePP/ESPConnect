/*
  --------------------------------
  ESPConnect - AutoConnect Example
  --------------------------------

  Connects to the saved WiFi network. If no credentials are saved, or the
  connection fails, opens a captive portal so the user can pick a network.

  Github & WiKi: https://github.com/MomePP/ESPConnect
  ESP32 only
*/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPConnect.h>

static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;
static constexpr unsigned long WIFI_CONNECT_POLL_MS = 250;
static constexpr unsigned long PORTAL_TIMEOUT_MS = 180000;

AsyncWebServer server(80);

static bool connectToSavedNetwork()
{
  if (!ESPConnect.isConfigured())
    return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ESPConnect.getSSID().c_str(), ESPConnect.getPassword().c_str());

  const unsigned long start = millis();
  while (!ESPConnect.isConnected() && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS)
    delay(WIFI_CONNECT_POLL_MS);

  return ESPConnect.isConnected();
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  /*
    Captive Portal AP
    Configure the server, SSID, optional password and portal timeout
  */
  ESPConnect.setupPortal(&server, "ESPConfig", "", PORTAL_TIMEOUT_MS);

  /*
    Try the previously saved WiFi first; if there is none or it fails,
    start the portal. startPortal() blocks until credentials are submitted,
    the timeout expires, or the optional cancel predicate returns true.
    Submitted credentials are saved, then WiFi is switched off, so connect again.
  */
  bool connected = connectToSavedNetwork();
  if (!connected && ESPConnect.startPortal())
    connected = connectToSavedNetwork();

  if (connected)
  {
    Serial.println("Connected to WiFi");
    Serial.println("IPAddress: " + WiFi.localIP().toString());
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello from ESP"); });

  server.begin();
}

void loop() {}
