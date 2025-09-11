#ifndef WEB_PLATFORM_H
#define WEB_PLATFORM_H

#include "interface/auth_types.h"
#include "interface/web_module_interface.h"
#include "interface/web_request.h"
#include "interface/web_response.h"
#include "platform/ntp_client.h"
#include <DNSServer.h>
#include <EEPROM.h>
#include <functional>
#include <map>
#include <vector>

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
class WebPlatform : public IWebModule, public IPlatformService {
public:
  WebPlatform();
  ~WebPlatform();

  // Primary initialization - auto-detects mode and capabilities
  void begin(const char *deviceName = "Device", bool forceHttpsOnly = false);

  // Module registration (only works in CONNECTED mode)
  bool registerModule(const char *basePath, IWebModule *module);

  // Route registration - unified handler system with auth requirements
  void registerRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                     const AuthRequirements &auth = {AuthType::NONE},
                     WebModule::Method method = WebModule::WM_GET);
  void overrideRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                     const AuthRequirements &auth = {AuthType::NONE},
                     WebModule::Method method = WebModule::WM_GET);
  void disableRoute(const String &path,
                    WebModule::Method method = WebModule::WM_GET);
  void clearRouteRegistry();

  // Handle all web requests and WiFi operations
  void handle();

  // WiFi state queries
  bool isConnected() const { return currentMode == CONNECTED; }
  WiFiConnectionState getConnectionState() const { return connectionState; }
  PlatformMode getCurrentMode() const { return currentMode; }

  // Server capabilities
  String getBaseUrl() const;
  int getPort() const { return serverPort; }

  // Device information
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
  void printUnifiedRoutes(const String *moduleBasePath = nullptr,
                          IWebModule *module = nullptr) const;
  void validateRoutes() const;

  // IPlatformService implementation
  String getDeviceName() const override { return deviceName; }
  bool isHttpsEnabled() const override { return httpsEnabled; }

  String prepareHtml(String html, WebRequest req, const String &csrfToken = "");

private:                            // Core server components
  WebServerClass *server = nullptr; // HTTP/HTTPS server pointer
  DNSServer dnsServer;              // For captive portal functionality

  // Authentication system
  bool authenticateRequest(WebRequest &req, WebResponse &res,
                           const AuthRequirements &requirements);
  void registerAuthRoutes();

  // CSRF token processing
  void addCsrfCookie(WebResponse &res, const String &token);
  void processCsrfForResponse(WebRequest &req, WebResponse &res);

  // Handlers
  void rootPageHandler(WebRequest &req, WebResponse &res);
  void statusPageHandler(WebRequest &req, WebResponse &res);
  void wifiPageHandler(WebRequest &req, WebResponse &res);
  void resetApiHandler(WebRequest &req, WebResponse &res);
  void statusApiHandler(WebRequest &req, WebResponse &res);
  void scanApiHandler(WebRequest &req, WebResponse &res);
  void wifiConfigHandler(WebRequest &req, WebResponse &res);
  void loginPageHandler(WebRequest &req, WebResponse &res);
  void loginApiHandler(WebRequest &req, WebResponse &res);
  void logoutPageHandler(WebRequest &req, WebResponse &res);
  void accountPageHandler(WebRequest &req, WebResponse &res);
  void accountPageJSAssetHandler(WebRequest &req, WebResponse &res);
  void configPortalSavePageHandler(WebRequest &req, WebResponse &res);
  void configPortalPageHandler(WebRequest &req, WebResponse &res);
  void configPortalSuccessJSAssetHandler(WebRequest &req, WebResponse &res);
  void webPlatformCSSAssetHandler(WebRequest &req, WebResponse &res);
  void webPlatformJSAssetHandler(WebRequest &req, WebResponse &res);
  void wifiJSAssetHandler(WebRequest &req, WebResponse &res);
  void styleCSSAssetHandler(WebRequest &req, WebResponse &res);
  void makerApiStyleCSSAssetHandler(WebRequest &req, WebResponse &res);
  void webPlatformFaviconHandler(WebRequest &req, WebResponse &res);
  void systemStatusJSAssetHandler(WebRequest &req, WebResponse &res);
  void homePageJSAssetHandler(WebRequest &req, WebResponse &res);

  // RESTful API handlers - User management
  void getUsersApiHandler(WebRequest &req, WebResponse &res);
  void createUserApiHandler(WebRequest &req, WebResponse &res);
  void getUserByIdApiHandler(WebRequest &req, WebResponse &res);
  void updateUserByIdApiHandler(WebRequest &req, WebResponse &res);
  void deleteUserByIdApiHandler(WebRequest &req, WebResponse &res);

  // Current user convenience handlers
  void getCurrentUserApiHandler(WebRequest &req, WebResponse &res);
  void updateCurrentUserApiHandler(WebRequest &req, WebResponse &res);

  // Token management handlers
  void getUserTokensApiHandler(WebRequest &req, WebResponse &res);
  void createUserTokenApiHandler(WebRequest &req, WebResponse &res);
  void deleteTokenApiHandler(WebRequest &req, WebResponse &res);

  // Legacy handlers (for compatibility with account page)
  void updateUserApiHandler(WebRequest &req, WebResponse &res);
  void createTokenApiHandler(WebRequest &req, WebResponse &res);

  // System status API handlers
  void getSystemStatusApiHandler(WebRequest &req, WebResponse &res);
  void getNetworkStatusApiHandler(WebRequest &req, WebResponse &res);
  void getModulesApiHandler(WebRequest &req, WebResponse &res);

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
  void initializeAuth();

  // Mode-specific setup
  void setupConfigPortalMode();
  void setupConnectedMode();

  // WiFi management (internal)
  bool loadWiFiCredentials(String &ssid, String &password);
  void saveWiFiCredentials(const String &ssid, const String &password);
  bool connectToStoredWiFi(String &ssid, String &password);
  void setupAccessPoint();
  void setupmDNS();

  // Route management
  void registerConfigPortalRoutes();
  void registerConnectedModeRoutes();
  void registerModuleRoutesForModule(const String &basePath,
                                     IWebModule *module);
  void bindRegisteredRoutes();
  bool dispatchRoute(const String &path, WebModule::Method wmMethod,
                     WebRequest &request, WebResponse &response,
                     const char *protocol);

#if defined(ESP32)
  void registerUnifiedHttpsRoutes();
#endif

  // Route registration helper methods (shared between HTTP and HTTPS)
  bool shouldSkipRoute(const RouteEntry &route, const String &serverType);
  void executeRouteWithAuth(const RouteEntry &route, WebRequest &request,
                            WebResponse &response, const String &serverType);
  bool pathMatchesRoute(const String &routePath, const String &requestPath);

  // Template processing helpers
  bool shouldProcessResponse(const WebResponse &response);
  void processResponseTemplates(WebRequest &request, WebResponse &response);

  // Certificate detection and HTTPS setup
  bool areCertificatesAvailable();
  void configureHttpsServer();
  bool getEmbeddedCertificates(const uint8_t **cert_data, size_t *cert_len,
                               const uint8_t **key_data, size_t *key_len);

  // HTTPS server components (ESP32 only)
#if defined(ESP32)
  httpd_handle_t httpsServerHandle = nullptr;

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
  bool isCaptivePortalRequest(const String &host);

  // Status and monitoring
  void updateConnectionState();
  unsigned long lastConnectionCheck = 0;
  static const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5 seconds
};

// Global instance
extern WebPlatform webPlatform;

#endif // WEB_PLATFORM_H