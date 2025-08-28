#ifndef WEB_PLATFORM_H
#define WEB_PLATFORM_H

#include <DNSServer.h>
#include <EEPROM.h>
#include <functional>
#include <map>
#include <vector>
#include <web_module_interface.h>
#include <web_request.h>
#include <web_response.h>
#include <web_ui_styles.h>

#if defined(ESP32)
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_https_server.h>
#ifdef CONFIG_IDF_TARGET_ESP32S3
#include <WiFiClientSecure.h>
#endif
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

// WebServerClass is now defined in web_module_interface/webserver_typedefs.h
// No need for a local definition

#include "route_entry.h"

// Platform operation modes
enum PlatformMode {
  CONFIG_PORTAL, // WiFi configuration portal mode
  CONNECTED      // Connected to WiFi, serving application
};

// WiFi connection states
enum WiFiConnectionState {
  WIFI_CONNECTING,       // Attempting to connect to WiFi
  WIFI_CONNECTED,        // Connected to WiFi network
  WIFI_CONFIG_PORTAL,    // Running in configuration portal mode
  WIFI_CONNECTION_FAILED // Failed to connect to WiFi
};

// Callback function types
typedef std::function<void()> WiFiSetupCompleteCallback;

/**
 * WebPlatform - Unified web server handling both WiFi configuration and
 * application serving
 *
 * This class merges the functionality of web_router and wifi_ap into a single,
 * secure platform that can serve HTTPS configuration portals when certificates
 * are available, and seamlessly transition between config and application
 * modes.
 *
 * Key features:
 * - Single server instance (HTTP or HTTPS based on certificate availability)
 * - HTTPS config portal for secure WiFi credential transmission
 * - Restart-based mode switching (simple and reliable)
 * - Module registration system for connected mode
 * - Automatic certificate detection without build flags
 */
class WebPlatform : public IWebModule {
public:
  WebPlatform();
  ~WebPlatform();

  // Primary initialization - auto-detects mode and capabilities
  void begin(const char *deviceName = "Device", bool forceHttpsOnly = false);

  // Module registration (only works in CONNECTED mode)
  bool registerModule(const char *basePath, IWebModule *module);

  // Route registration - unified handler system
  void registerRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                     WebModule::Method method = WebModule::WM_GET);
  void overrideRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                     WebModule::Method method = WebModule::WM_GET);
  void disableRoute(const String &path,
                    WebModule::Method method = WebModule::WM_GET);

  // Handle all web requests and WiFi operations
  void handle();

  // WiFi state queries
  bool isConnected() const { return currentMode == CONNECTED; }
  WiFiConnectionState getConnectionState() const { return connectionState; }
  PlatformMode getCurrentMode() const { return currentMode; }

  // Server capabilities
  bool isHttpsEnabled() const { return httpsEnabled; }
  String getBaseUrl() const;
  int getPort() const { return serverPort; }

  // Device information
  const char *getDeviceName() const { return deviceName; }
  const char *getAPName() const { return apSSIDBuffer; }
  String getHostname() const { return String(deviceName) + ".local"; }

  // WiFi management
  void resetWiFiCredentials();
  void startConfigPortal();
  void onSetupComplete(WiFiSetupCompleteCallback callback);

  // IWebModule interface (for consistency, though not used by web_router)
  std::vector<WebRoute> getHttpRoutes() override;
  std::vector<WebRoute> getHttpsRoutes() override;
  String getModuleName() const override { return "WebPlatform"; }
  String getModuleVersion() const override { return "1.0.0"; }

  // Debug and monitoring
  size_t getRouteCount() const;
  void printRoutes() const;
  void printUnifiedRoutes() const;

private:                            // Core server components
  WebServerClass *server = nullptr; // HTTP/HTTPS server pointer
  DNSServer dnsServer;              // For captive portal functionality

  // Platform state
  PlatformMode currentMode;
  WiFiConnectionState connectionState;
  bool httpsEnabled;
  bool running;
  int serverPort;

  // Device configuration
  const char *deviceName;
  char apSSIDBuffer[64];       // Buffer for AP SSID generation
  const char *apPassword = ""; // Open AP (no password)

  // Callbacks and state tracking
  WiFiSetupCompleteCallback setupCompleteCallback;
  bool callbackCalled;

  // Module registry (for CONNECTED mode)
  struct RegisteredModule {
    String basePath;
    IWebModule *module;
  };
  std::vector<RegisteredModule> registeredModules;

  // EEPROM configuration
  static const int EEPROM_SIZE = 512;
  static const int WIFI_SSID_ADDR = 0;
  static const int WIFI_PASS_ADDR = 64;
  static const int WIFI_CONFIG_FLAG_ADDR = 128;

  // Platform initialization methods
  void initializeEEPROM();
  void determinePlatformMode();
  bool detectHttpsCapability();
  void startServer();
  void setupRoutes();

  // Mode-specific setup
  void setupConfigPortalMode();
  void setupConnectedMode();

  // WiFi management (internal)
  bool loadWiFiCredentials(String &ssid, String &password);
  void saveWiFiCredentials(const String &ssid, const String &password);
  bool connectToStoredWiFi();
  void setupAccessPoint();
  void setupmDNS();

  // Route handlers for CONFIG_PORTAL mode
  String handleConfigPortalRoot();
  String handleConfigPortalSave(const String &postBody);
  String handleWiFiStatusAPI();
  String handleWiFiScanAPI();
  String handleWiFiResetAPI();

  // Route handlers for CONNECTED mode
  String handleConnectedRoot();
  String handleWiFiManagement();
  String handleSystemStatus();

  // Route management
  void registerConfigPortalRoutes();
  void registerConnectedModeRoutes();
  void registerUnifiedRoutes();
  void convertModuleRoutesToUnified();

#if defined(ESP32)
  void registerUnifiedHttpsRoutes();
#endif

  // Certificate detection and HTTPS setup
  bool areCertificatesAvailable();
  void configureHttpsServer();
  bool getEmbeddedCertificates(const uint8_t **cert_data, size_t *cert_len,
                               const uint8_t **key_data, size_t *key_len);

  // HTTPS server components (ESP32 only)
#if defined(ESP32)
  httpd_handle_t httpsServerHandle = nullptr;

  // Route registration methods (implemented in web_platform_https_routes.cpp)
  void registerHttpsRoutes();
  // Request handling methods (implemented in web_platform_https_handlers.cpp)
  static esp_err_t httpsGenericHandler(httpd_req_t *req);
  esp_err_t handleHttpsConfigPortal(httpd_req_t *req);
  esp_err_t handleHttpsConnected(httpd_req_t *req);

  std::vector<String> httpsRoutePaths; // Permanent path storage
#endif

public:
  // Make httpsInstance accessible to external handlers
#if defined(ESP32)
  static WebPlatform *httpsInstance; // For ESP-IDF callbacks (defined in core)
#endif

  // HTTP request handling helpers
  void handleNotFound();
  String extractPostParameter(const String &postBody, const String &paramName);
  std::map<String, String> parseQueryParams(const String &query);

  // Captive portal helpers
  void setupCaptivePortal();
  bool isCaptivePortalRequest(const String &host);

  // Status and monitoring
  void updateConnectionState();
  unsigned long lastConnectionCheck = 0;
  static const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5 seconds
};

// Global instance
extern WebPlatform webPlatform;

#endif // WEB_PLATFORM_H