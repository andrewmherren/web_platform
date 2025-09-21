#ifndef WEB_PLATFORM_H
#define WEB_PLATFORM_H

#include "interface/auth_types.h"
#include "interface/openapi_generation_context.h"
#include "interface/openapi_types.h"
#include "interface/web_module_interface.h"
#include "interface/web_request.h"
#include "interface/web_response.h"
#include "platform/ntp_client.h"
#include "utilities/debug_macros.h"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <functional>
#include <map>
#include <vector>

#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_https_server.h>

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

// Platform configuration structure
struct PlatformConfig {
  uint16_t maxUriHandlers = 60; // ESP32 HTTPS server route limit
  uint16_t stackSize = 8192;    // Server task stack size
  bool forceHttpsOnly = false;  // Force HTTPS-only mode

  // Constructor for easy initialization
  PlatformConfig() = default;

  // Convenience constructor for common case
  PlatformConfig(bool httpsOnly) : forceHttpsOnly(httpsOnly) {}
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
  void begin(const char *deviceName, const PlatformConfig &config);

  // Module pre-registration (must be called before begin())
  bool registerModule(const char *basePath, IWebModule *module);
  bool registerModule(const char *basePath, IWebModule *module,
                      const JsonVariant &config);

  // Convenience methods for common module configurations
  template <typename T>
  bool registerModule(const char *basePath, IWebModule *module,
                      const T &config) {
    DynamicJsonDocument doc(1024);
    doc.set(config);
    return registerModule(basePath, module, doc.as<JsonVariant>());
  }

  // Route registration - unified handler system with auth requirements
  void registerWebRoute(const String &path,
                        WebModule::UnifiedRouteHandler handler,
                        const AuthRequirements &auth = {AuthType::NONE},
                        WebModule::Method method = WebModule::WM_GET);

  void
  registerApiRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                   const AuthRequirements &auth, WebModule::Method method,
                   const OpenAPIDocumentation &docs = OpenAPIDocumentation());

  // Handle all web requests and WiFi operations
  void handle();

  // Module lifecycle management
  void handleRegisteredModules();

  // WiFi state queries
  bool isConnected() const { return currentMode == CONNECTED; }
  WiFiConnectionState getConnectionState() const { return connectionState; }
  PlatformMode getCurrentMode() const { return currentMode; }

  // Server capabilities
  String getBaseUrl() const;
  // int getPort() const { return serverPort; }

  // Device information
  const char *getAPName() const { return apSSIDBuffer; }
  String getHostname() const { return String(deviceName) + ".local"; }

  // WiFi management
  void resetWiFiCredentials();

  // IWebModule interface (for consistency, though not used by web_router)
  std::vector<RouteVariant> getHttpRoutes() override;
  std::vector<RouteVariant> getHttpsRoutes() override;
  String getModuleName() const override { return "WebPlatform"; }
  String getModuleVersion() const override { return "1.0.0"; }

  // Debug and monitoring
  size_t getRouteCount() const;
  void printUnifiedRoutes() const;
  // void validateRoutes() const;

  // Memory analysis functions
  void measureHeapUsage(const char *phase);

  // IPlatformService implementation
  String getDeviceName() const override { return deviceName; }
  bool isHttpsEnabled() const override { return httpsEnabled; }

  String prepareHtml(String html, WebRequest req, const String &csrfToken = "");

  // Pre-generated OpenAPI serving (memory efficient)
  void streamPreGeneratedOpenAPISpec(WebResponse &res) const;

  // OpenAPI generation helper methods
  String generateDefaultSummary(const String &path, const String &method) const;
  String generateOperationId(const String &method, const String &path) const;
  String inferModuleFromPath(const String &path) const;
  String formatModuleName(const String &moduleName) const;
  
  // Methods that work with temporary storage
  void addParametersToOperationFromDocs(JsonObject &operation,
                                        const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const;
  void addResponsesToOperationFromDocs(JsonObject &operation,
                                       const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const;
  void addRequestBodyToOperationFromDocs(JsonObject &operation,
                                         const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const;

private:                            // Core server components
  WebServerClass *server = nullptr; // HTTP/HTTPS server pointer
  DNSServer dnsServer;              // For captive portal functionality

  void registerRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                     const AuthRequirements &auth, WebModule::Method method,
                     const OpenAPIDocumentation &docs);

  // Authentication system
  bool authenticateRequest(WebRequest &req, WebResponse &res,
                           const AuthRequirements &requirements);
  void registerAuthRoutes();

  // CSRF token processing
  void addCsrfCookie(WebResponse &res, const String &token);

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
  void configPortalPageHandler(WebRequest &req, WebResponse &res);
  void configPortalSuccessJSAssetHandler(WebRequest &req, WebResponse &res);
  void webPlatformCSSAssetHandler(WebRequest &req, WebResponse &res);
  void webPlatformJSAssetHandler(WebRequest &req, WebResponse &res);
  void wifiJSAssetHandler(WebRequest &req, WebResponse &res);
  void styleCSSAssetHandler(WebRequest &req, WebResponse &res);
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

  // System status API handlers
  void getSystemStatusApiHandler(WebRequest &req, WebResponse &res);
  void getNetworkStatusApiHandler(WebRequest &req, WebResponse &res);
  void getModulesApiHandler(WebRequest &req, WebResponse &res);
  void getOpenAPISpecHandler(WebRequest &req, WebResponse &res);

  // Platform state
  PlatformMode currentMode;
  WiFiConnectionState connectionState;
  bool httpsEnabled;
  bool running;
  int serverPort;

  // OpenAPI generation system - stored in storage system
  bool openAPISpecReady = false;
  static const String OPENAPI_COLLECTION;
  static const String OPENAPI_SPEC_KEY;
  String preGeneratedOpenAPISpec; // Store the complete spec
  
  // Temporary documentation storage
  OpenAPIGenerationContext openAPIGenerationContext;

  // Platform configuration
  PlatformConfig platformConfig;

  // Configuration validation and error handling
  bool validatePendingModules();
  void handleInitializationError(const String &error);

  // Device configuration
  const char *deviceName;
  char apSSIDBuffer[64];       // Buffer for AP SSID generation
  const char *apPassword = ""; // Open AP (no password)

  // Callbacks and state tracking
  WiFiSetupCompleteCallback setupCompleteCallback;
  bool callbackCalled;

  // Module registry structures
  struct PendingModule {
    String basePath;
    IWebModule *module;
    DynamicJsonDocument config; // Store full config document

    // Constructor to properly initialize DynamicJsonDocument
    PendingModule() : config(512) {} // 512 bytes for config storage
    PendingModule(const String &path, IWebModule *mod,
                  const JsonVariant &configData)
        : basePath(path), module(mod), config(512) {
      if (!configData.isNull()) {
        config.set(configData);
      }
    }
  };

  struct RegisteredModule {
    String basePath;
    IWebModule *module;
  };

  std::vector<PendingModule> pendingModules; // Pre-registration storage
  std::vector<RegisteredModule>
      registeredModules; // Active modules post-begin()

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
  void initializeRegisteredModules();
  void generateOpenAPISpec();
  
  // Temporary documentation collection
  void beginOpenAPIGeneration();
  void completeOpenAPIGeneration();

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

  void registerUnifiedHttpsRoutes();

  // Route registration helper methods (shared between HTTP and HTTPS)
  bool shouldSkipRoute(const RouteEntry &route, const String &serverType);
  void executeRouteWithAuth(const RouteEntry &route, WebRequest &request,
                            WebResponse &response, const String &serverType);
  bool pathMatchesRoute(const char *routePath, const String &requestPath);

  // Template processing helpers
  bool shouldProcessResponse(const WebResponse &response);
  void processResponseTemplates(WebRequest &request, WebResponse &response);

  // Certificate detection and HTTPS setup
  bool areCertificatesAvailable();
  void configureHttpsServer();
  bool getEmbeddedCertificates(const uint8_t **cert_data, size_t *cert_len,
                               const uint8_t **key_data, size_t *key_len);

  httpd_handle_t httpsServerHandle = nullptr;

  std::vector<String> httpsRoutePaths; // Permanent path storage

public:
  // Make httpsInstance accessible to external handlers
  static WebPlatform *httpsInstance; // For ESP-IDF callbacks (defined in core)

  // HTTP request handling helpers
  void handleNotFound();
  std::map<String, String> parseQueryParams(const String &query);

  // Status and monitoring
  void updateConnectionState();
  unsigned long lastConnectionCheck = 0;
  static const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5 seconds
};

// Global instance
extern WebPlatform webPlatform;

#endif // WEB_PLATFORM_H