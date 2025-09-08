#include "../../include/interface/platform_service.h"
#include "../../include/platform/ntp_client.h"
#include "../../include/web_platform.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// Core implementation of WebPlatform class
// Basic initialization, setup, and handling functions

#if defined(ESP32)
// Static instance pointer for ESP-IDF callbacks
WebPlatform *WebPlatform::httpsInstance = nullptr;
#endif

IPlatformService *g_platformService = nullptr;

IPlatformService *getPlatformService() { return g_platformService; }

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

  // Set up the global service reference
  g_platformService = this;

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
  } else if (currentMode == CONNECTED) {
    // Handle NTP client updates when connected
    NTPClient::handle();
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
    // In captive portal mode, redirect ALL requests to the config page
    // This ensures PC browsers will navigate to the portal regardless of what
    // URL was entered
    String portalUrl = "http://" + WiFi.softAPIP().toString() + "/";

    // Check if this is already a request to the portal URL to avoid redirect
    // loops
    String requestHost = server->hostHeader();
    String requestUri = server->uri();
    String softAPIP = WiFi.softAPIP().toString();

    // Don't redirect if already requesting the portal directly
    bool isPortalRequest =
        (requestHost == softAPIP || requestHost.startsWith(softAPIP + ":"));
    bool isRootRequest = (requestUri == "/" || requestUri.isEmpty());

    if (!isPortalRequest || !isRootRequest) {
      Serial.printf("WebPlatform: Captive portal redirect: %s%s -> %s\n",
                    requestHost.c_str(), requestUri.c_str(), portalUrl.c_str());
      server->sendHeader("Location", portalUrl);
      server->sendHeader("Connection", "close");
      server->send(302, "text/html",
                   "<html><head><title>WiFi Setup</title></head><body>"
                   "<h1>WiFi Configuration Required</h1>"
                   "<p>Redirecting to setup page...</p>"
                   "<p><a href='" +
                       portalUrl +
                       "'>Click here if not redirected automatically</a></p>"
                       "<script>window.location.href='" +
                       portalUrl +
                       "';</script>"
                       "</body></html>");
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
  
  // Check for parameterized/wildcard routes
  WebRequest request(server);
  String requestPath = request.getPath();
  WebModule::Method wmMethod = httpMethodToWMMethod(server->method());
    
  // Check all routes for wildcard matches
  for (const auto &route : routeRegistry) {
    if (route.disabled || !route.handler || route.method != wmMethod) {
      continue;
    }

    bool hasWildcard = route.path.indexOf('*') >= 0 || route.path.indexOf('{') >= 0;
    bool pathMatches = this->pathMatchesRoute(route.path, requestPath);
    
    if (hasWildcard && pathMatches) {
      WebResponse response;
      
      // Set the matched route pattern for parameter extraction
      request.setMatchedRoute(route.path);
      
      // Process the request with full auth handling
      this->executeRouteWithAuth(route, request, response, "HTTP");
      response.sendTo(server);
      return;
    }
  }
  
  // Use IWebModule error page system if no wildcard routes matched
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

  // If in config portal mode, add/override with portal routes
  if (currentMode == CONFIG_PORTAL) {
    setupConfigPortalMode();
  } else {
    // register connected mode routes
    setupConnectedMode();
  }

  // Print final route registry for debugging (web platform routes only)
  printUnifiedRoutes();

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

  // Register main portal routes (using /portal instead of /)
  registerConfigPortalRoutes();

  // register a redirect but do it directly on the server instead of the registry. 
  // This means that if the user has tried to add one to the registry, it wont add
  // because this one is already on the server (cant be replace donce fully regsitered)
  server->on("/", HTTP_GET, [this]() {
    server->sendHeader("Location", "/portal");
    server->send(302, "text/plain", "Redirecting to setup...");
  });

  // Bind user overrides from registry (includes asset overrides like CSS)
  bindRegisteredRoutes();

  // Setup captive portal DNS server to redirect all DNS queries to our AP IP
  // This makes PC browsers navigate to the portal when any domain is entered
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.printf(
      "WebPlatform: Captive portal DNS started - all domains redirect to %s\n",
      WiFi.softAPIP().toString().c_str());
}

void WebPlatform::setupConnectedMode() {
  Serial.println("WebPlatform: Setting up connected mode routes");

  // Register core platform routes FIRST (before overrides are processed)
  registerConnectedModeRoutes();

  // Register all unified routes with servers (this processes overrides)
  bindRegisteredRoutes();

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
  bindRegisteredRoutes();
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
    }

    // Register the route directly with the unified system
    if (route.unifiedHandler) {
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

  // Print only routes for this specific module
  printUnifiedRoutes(&basePath, module);

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
