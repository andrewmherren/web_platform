#include "platform/ntp_client.h"
#include "platform/wifi_credentials_store.h"
#include "web_platform.h"

bool WebPlatform::connectToStoredWiFi(String &ssid, String &password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait up to 10 seconds for connection
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN();

  return WiFi.status() == WL_CONNECTED;
}

void WebPlatform::resetWiFiCredentials() { WiFiCredentialsStore::reset(); }

void WebPlatform::setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSIDBuffer, apPassword);

  DEBUG_PRINTF("WebPlatform: Access Point started: %s\n", apSSIDBuffer);
  DEBUG_PRINTF("WebPlatform: AP IP address: %s\n",
               WiFi.softAPIP().toString().c_str());
}

void WebPlatform::setupmDNS() {
  if (MDNS.begin(deviceName)) {
    MDNS.addService("http", "tcp", serverPort);
    DEBUG_PRINTF("WebPlatform: mDNS started: %s.local\n", deviceName);
  } else {
    DEBUG_PRINTLN("WebPlatform: mDNS failed to start");
  }
}

void WebPlatform::updateConnectionState() {
  if (currentMode == CONNECTED && WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN(
        "WebPlatform: WiFi connection lost, switching to config portal");
    connectionState = WIFI_CONNECTION_FAILED;
    // Note: In a full implementation, this might trigger a restart
    // For now, we'll maintain current behavior
  }
}

void WebPlatform::determinePlatformMode() {
  String ssid, password;

  if (WiFiCredentialsStore::load(ssid, password) && ssid.length() > 0) {
    DEBUG_PRINTLN(
        "WebPlatform: Found stored WiFi credentials, attempting connection...");
    if (connectToStoredWiFi(ssid, password)) {
      currentMode = CONNECTED;
      connectionState = WIFI_CONNECTED;
      setupmDNS();

      // Initialize NTP client after WiFi connection
      NTPClient::begin();

      DEBUG_PRINTF("WebPlatform: Connected to WiFi: %s\n", WiFi.SSID().c_str());
      DEBUG_PRINTF("WebPlatform: IP address: %s\n",
                   WiFi.localIP().toString().c_str());
    } else {
      DEBUG_PRINTLN("WebPlatform: Failed to connect to stored WiFi, starting "
                    "config portal");
      currentMode = CONFIG_PORTAL;
      connectionState = WIFI_CONFIG_PORTAL;
      setupAccessPoint();
    }
  } else {
    DEBUG_PRINTLN(
        "WebPlatform: No WiFi credentials found, starting config portal");
    currentMode = CONFIG_PORTAL;
    connectionState = WIFI_CONFIG_PORTAL;
    setupAccessPoint();
  }
}
