#include "../web_platform.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// Core implementation of WebPlatform class
// Basic initialization, setup, and handling functions

// Global instance
WebPlatform webPlatform;

#if defined(ESP32)
// Static instance pointer for ESP-IDF callbacks
WebPlatform *WebPlatform::httpsInstance = nullptr;
#endif

WebPlatform::WebPlatform()
    : currentMode(CONFIG_PORTAL), connectionState(WIFI_CONFIG_PORTAL),
      httpsEnabled(false), running(false), serverPort(80), deviceName("Device"),
      callbackCalled(false) {

  memset(apSSIDBuffer, 0, sizeof(apSSIDBuffer));

#if defined(ESP32)
  httpsInstance = this;
#endif
}

WebPlatform::~WebPlatform() {
  if (running) {
    if (server) {
      server->stop();
      delete server;
      server = nullptr;
    }
#if defined(ESP32)
    if (httpsServerHandle) {
      httpd_ssl_stop(httpsServerHandle);
      httpsServerHandle = nullptr;
    }
#endif
    dnsServer.stop();
  }
#if defined(ESP32)
  if (httpsInstance == this) {
    httpsInstance = nullptr;
  }
#endif
}

void WebPlatform::begin(const char *deviceName, bool forceHttpsOnly) {
  Serial.println("WebPlatform: Starting initialization...");

  this->deviceName = deviceName;

  // Generate AP SSID
  snprintf(apSSIDBuffer, sizeof(apSSIDBuffer), "%sSetup", deviceName);

  // Initialize EEPROM
  initializeEEPROM();

  // Determine platform mode based on stored WiFi credentials
  determinePlatformMode();

  // Detect HTTPS capability
  httpsEnabled = detectHttpsCapability();

  // Force HTTPS-only mode if requested
  if (forceHttpsOnly && httpsEnabled) {
    Serial.println(
        "WebPlatform: Forcing HTTPS-only mode with HTTP→HTTPS redirection");
  }

  // Start server with appropriate configuration
  startServer();

  // Setup routes based on current mode
  setupRoutes();

  Serial.printf("WebPlatform: Initialized in %s mode\n",
                currentMode == CONFIG_PORTAL ? "CONFIG_PORTAL" : "CONNECTED");
  Serial.printf("WebPlatform: HTTPS %s\n",
                httpsEnabled ? "enabled" : "disabled");
  Serial.printf("WebPlatform: Server running on port %d\n", serverPort);
}

void WebPlatform::handle() {
  if (server) {
    server->handleClient();
  }

  if (currentMode == CONFIG_PORTAL) {
    dnsServer.processNextRequest();
  }

  // Periodic connection state updates
  unsigned long now = millis();
  if (now - lastConnectionCheck > CONNECTION_CHECK_INTERVAL) {
    updateConnectionState();
    lastConnectionCheck = now;
  }
}

void WebPlatform::handleNotFound() {
  // Only handle Arduino WebServer 404s - HTTPS 404s are handled in
  // handleHttpsConnected
  if (!server)
    return;

  if (currentMode == CONFIG_PORTAL) {
    if (isCaptivePortalRequest(server->hostHeader())) {
      // Redirect to configuration page for captive portal
      server->sendHeader("Location",
                         "http://" + WiFi.softAPIP().toString() + "/");
      server->send(302, "text/html", "");
      return;
    }
  }

  // Check for any redirect rules
  String redirectTarget = IWebModule::getRedirectTarget(server->uri());
  if (redirectTarget.length() > 0) {
    Serial.printf("WebPlatform: Redirecting %s to %s\n", server->uri().c_str(),
                  redirectTarget.c_str());
    server->sendHeader("Location", redirectTarget);
    server->send(302, "text/html", "");
    return;
  }

  // Use IWebModule error page system
  String errorPage = IWebModule::getErrorPage(404);
  if (errorPage.length() > 0) {
    server->send(404, "text/html", errorPage);
  } else {
    server->send(404, "text/plain", "Not Found");
  }
}

bool WebPlatform::isCaptivePortalRequest(const String &host) {
  // Simple captive portal detection
  return (host.indexOf("captive") != -1 || host.indexOf("generate") != -1 ||
          host.indexOf("connectivitycheck") != -1);
}

void WebPlatform::setupRoutes() {
  initializeAuth(); // Initialize the auth system

  // First register connected mode routes (lower priority)
  setupConnectedMode();

  // If in config portal mode, add/override with portal routes
  if (currentMode == CONFIG_PORTAL) {
    setupConfigPortalMode();
  }

  // Print final route registry for debugging
  printUnifiedRoutes(); // TODO: eventually we can remove this
  validateRoutes(); // TODO: eventaully can remove not only this call but the
                    // whole method

  // For HTTPS-only mode with redirection server, we don't register normal
  // routes on HTTP server We know we're in redirect mode if server exists and
  // HTTPS is enabled with serverPort 443
  bool isRedirectServer =
      (httpsEnabled && serverPort == 443 && server != nullptr);
  // Check if we know this is the HTTP server running on port 80
  bool isHttpRedirectServer = false;
#if defined(ESP8266)
  // For ESP8266, we can check the server port directly
  isHttpRedirectServer = isRedirectServer && serverPort == 80;
#elif defined(ESP32)
  // For ESP32, we can determine this based on our setup logic
  // If HTTPS is enabled and we have a server, it must be the redirect server on
  // port 80
  isHttpRedirectServer = isRedirectServer;
#endif

  if (server && !isHttpRedirectServer) {
    // Setup 404 handler
    server->onNotFound([this]() { handleNotFound(); });
  }
}

void WebPlatform::setupConfigPortalMode() {
  Serial.println("WebPlatform: Setting up config portal routes");

  // Register main portal routes
  registerConfigPortalRoutes();

  // Register unified routes with the server
  registerUnifiedRoutes();

  // Setup captive portal
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("WebPlatform: Captive portal DNS started");
}

void WebPlatform::setupConnectedMode() {
  Serial.println("WebPlatform: Setting up connected mode routes");

  // Register core platform routes FIRST (before overrides are processed)
  registerConnectedModeRoutes();

  // Register all unified routes with servers (this processes overrides)
  registerUnifiedRoutes();

#if defined(ESP32)
  if (httpsEnabled && httpsServerHandle) {
    registerUnifiedHttpsRoutes();
  }
#endif
}

void WebPlatform::startConfigPortal() {
  currentMode = CONFIG_PORTAL;
  connectionState = WIFI_CONFIG_PORTAL;
  setupAccessPoint();
  Serial.println("WebPlatform: Config portal started");
}

String WebPlatform::getBaseUrl() const {
  if (currentMode == CONFIG_PORTAL) {
    return "http://" + WiFi.softAPIP().toString() + ":" + String(serverPort);
  } else {
    String protocol = httpsEnabled ? "https" : "http";
    return protocol + "://" + WiFi.localIP().toString() + ":" +
           String(serverPort);
  }
}

bool WebPlatform::registerModule(const char *basePath, IWebModule *module) {
  if (currentMode != CONNECTED) {
    Serial.println(
        "WebPlatform: Cannot register modules in CONFIG_PORTAL mode");
    return false;
  }

  if (!module) {
    Serial.println("WebPlatform: Cannot register null module");
    return false;
  }

  // Check if module already registered
  for (const auto &regModule : registeredModules) {
    if (regModule.basePath == basePath) {
      Serial.printf("WebPlatform: Module already registered at path: %s\n",
                    basePath);
      return false;
    }
  }

  registeredModules.push_back({basePath, module});
  Serial.printf("WebPlatform: Registered module '%s' at path: %s\n",
                module->getModuleName().c_str(), basePath);

  // Register this module's routes immediately
  registerModuleRoutesForModule(basePath, module);

  // Re-register with the server
  registerUnifiedRoutes();
#if defined(ESP32)
  if (httpsEnabled && httpsServerHandle) {
    registerUnifiedHttpsRoutes();
  }
#endif

  return true;
}

// Register routes for a specific module
void WebPlatform::registerModuleRoutesForModule(const String &basePath,
                                                IWebModule *module) {
  Serial.printf("  Processing module: %s at path: %s\n",
                module->getModuleName().c_str(), basePath.c_str());

  // Process HTTP routes
  auto httpRoutes = module->getHttpRoutes();
  Serial.printf("  Module has %d HTTP routes\n", httpRoutes.size());

  for (const auto &route : httpRoutes) {
    // Create full path
    String fullPath = basePath;
    String routePath = route.path;

    // Special case for root path
    if (routePath == "/" || routePath.isEmpty()) {
      // For root path, ensure the base path ends with a slash
      if (!fullPath.endsWith("/")) {
        fullPath += "/";
      }
      Serial.printf("  Module root path: %s\n", fullPath.c_str());
    } else if (!fullPath.endsWith("/") && !routePath.startsWith("/")) {
      // Neither has slash, add one between
      fullPath += "/" + routePath;
      Serial.printf("  Added slash between paths: %s\n", fullPath.c_str());
    } else if (fullPath.endsWith("/") && routePath.startsWith("/")) {
      // Both have slash, remove duplicate
      fullPath += routePath.substring(1);
      Serial.printf("  Removed duplicate slash: %s\n", fullPath.c_str());
    } else {
      // One has slash, just concatenate
      fullPath += routePath;
      Serial.printf("  Standard path concatenation: %s\n", fullPath.c_str());
    }

    // Register the route directly with the unified system
    if (route.unifiedHandler) {
      Serial.printf("  Registering unified route: %s %s\n",
                    httpMethodToString(route.method).c_str(), fullPath.c_str());

      // Pass the route's auth requirements when registering
      registerRoute(fullPath, route.unifiedHandler, route.authRequirements,
                    route.method);
    } else if (route.handler) {
      // Convert legacy handler to unified handler
      // Lambda to adapt the legacy handler format
      auto unifiedHandler = [route, basePath](WebRequest &req,
                                              WebResponse &res) {
        // Set current path for navigation highlighting
        IWebModule::setCurrentPath(basePath);

        // Extract parameters from request
        std::map<String, String> params = req.getAllParams();

        // Call legacy handler
        String result = route.handler(req.getBody(), params);

        // Set response
        res.setContent(result, route.contentType);
      };

      // By default, legacy routes have no auth requirements
      registerRoute(fullPath, unifiedHandler, {AuthType::NONE}, route.method);
    }
  }

  // Similar process for HTTPS routes
  auto httpsRoutes = module->getHttpsRoutes();
  // Skip detailed HTTPS processing as it's redundant with the unified system
}

void WebPlatform::onSetupComplete(WiFiSetupCompleteCallback callback) {
  setupCompleteCallback = callback;
}

// IWebModule interface implementation (for consistency)
std::vector<WebRoute> WebPlatform::getHttpRoutes() {
  // Return empty routes since WebPlatform manages its own routing
  return std::vector<WebRoute>();
}

std::vector<WebRoute> WebPlatform::getHttpsRoutes() { return getHttpRoutes(); }
