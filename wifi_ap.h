#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <DNSServer.h>
#include <EEPROM.h>
#include <functional>

#if defined(ESP32)
#include <WebServer.h>
#include <WiFi.h>
typedef WebServer WebServerClass;
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
typedef ESP8266WebServer WebServerClass;
#endif

// WiFi connection states
enum WiFiConnectionState {
  WIFI_CONNECTING,       // Attempting to connect to WiFi
  WIFI_CONNECTED,        // Connected to WiFi network
  WIFI_CONFIG_PORTAL,    // Running in configuration portal mode
  WIFI_CONNECTION_FAILED // Failed to connect to WiFi
};

// Callback function type for when WiFi setup is complete
typedef std::function<void()> WiFiSetupCompleteCallback;

// Class to manage WiFi connection and configuration
class WiFiManager {
public:
  WiFiManager();

  // Initialize WiFi functionality with optional custom AP name and web
  // interface
  void begin(const char *apName = "DeviceSetup",
             bool enableWebInterface = true);

  // Set callback to be called when WiFi connection is established
  void onSetupComplete(WiFiSetupCompleteCallback callback);

  // Setup WiFi management web handlers (API endpoints)
  void setupWebHandlers(WebServerClass &server);

  // Handle WiFi operations (must be called in loop)
  void handle();

  // Get the current connection state
  WiFiConnectionState getConnectionState() const;

  // Get a reference to the web server (for adding custom handlers)
  WebServerClass &getWebServer();

  // Force start the configuration portal
  void startConfigPortal();

  // Reset stored WiFi credentials
  void resetSettings();

  // Get AP name
  const char *getAPName() const { return apSSID; }

private: // Web server for both config portal and normal operation
  WebServerClass server;

  // DNS server for captive portal
  DNSServer dnsServer;

  // Current WiFi state
  WiFiConnectionState connectionState;

  // Setup completion callback
  WiFiSetupCompleteCallback setupCompleteCallback;

  // Access Point settings
  const char *apSSID;
  const char *apPassword = ""; // No password for open AP

  // Flag to track if callback has been called
  bool callbackCalled;

  // Web interface configuration
  bool webInterfaceEnabled;

  // EEPROM memory addresses
  static const int EEPROM_SIZE = 512;
  static const int WIFI_SSID_ADDR = 0;
  static const int WIFI_PASS_ADDR = 64;
  static const int WIFI_CONFIG_FLAG_ADDR = 128;

  // Internal methods
  void setupConfigPortal();
  bool connectToStoredWiFi();
  void saveWiFiCredentials(const String &ssid, const String &password);
  bool loadWiFiCredentials(String &ssid, String &password);

  // API handler methods (always available)
  void handleWiFiStatus();
  void handleWiFiScan();
  void handleWiFiConnect();
  void handleWiFiDisconnect();
  void handleWiFiReset();

  // Web handler methods
  void handleRoot();
  void handleSave();
  void handleNotFound();
};

// Global instance that can be accessed from other modules
extern WiFiManager wifiManager;

#endif // WIFI_AP_H