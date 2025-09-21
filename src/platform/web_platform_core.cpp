#include "interface/platform_service.h"
#include "platform/ntp_client.h"
#include "platform/route_string_pool.h"
#include "route_entry.h"
#include "web_platform.h"

#include <WebServer.h>

// Core implementation of WebPlatform class
// Basic initialization, setup, and handling functions

// Static instance pointer for ESP-IDF callbacks
WebPlatform *WebPlatform::httpsInstance = nullptr;

IPlatformService *g_platformService = nullptr;

IPlatformService *getPlatformService() { return g_platformService; }

WebPlatform::WebPlatform()
    : currentMode(CONFIG_PORTAL), connectionState(WIFI_CONFIG_PORTAL),
      httpsEnabled(false), running(false), serverPort(80), deviceName("Device"),
      callbackCalled(false) {

  memset(apSSIDBuffer, 0, sizeof(apSSIDBuffer));
  httpsInstance = this;
}

WebPlatform::~WebPlatform() {
  if (running) {
    if (server) {
      server->stop();
      delete server;
      server = nullptr;
    }
    if (httpsServerHandle) {
      httpd_ssl_stop(httpsServerHandle);
      httpsServerHandle = nullptr;
    }
    dnsServer.stop();
  }

  if (httpsInstance == this) {
    httpsInstance = nullptr;
  }
}

void WebPlatform::begin(const char *deviceName, const PlatformConfig &config) {
  this->platformConfig = config;
  begin(deviceName, config.forceHttpsOnly);
}

void WebPlatform::begin(const char *deviceName, bool forceHttpsOnly) {
  DEBUG_PRINTLN("WebPlatform: Starting initialization...");

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
    DEBUG_PRINTLN(
        "WebPlatform: Forcing HTTPS-only mode with HTTP→HTTPS redirection");
  }

  // Start server with appropriate configuration
  startServer();

  // Initialize pre-registered modules if in CONNECTED mode
  if (currentMode == CONNECTED) {
    // Validate pending modules before initialization
    if (!validatePendingModules()) {
      handleInitializationError("Module validation failed");
      return;
    }

    initializeRegisteredModules();
  }

  // Setup routes based on current mode
  setupRoutes();

  DEBUG_PRINTF("WebPlatform: Initialized in %s mode\n",
               currentMode == CONFIG_PORTAL ? "CONFIG_PORTAL" : "CONNECTED");
  DEBUG_PRINTF("WebPlatform: HTTPS %s\n",
               httpsEnabled ? "enabled" : "disabled");
  DEBUG_PRINTF("WebPlatform: Server running on port %d\n", serverPort);
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

    // Handle all registered modules
    handleRegisteredModules();
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
      DEBUG_PRINTF("WebPlatform: Captive portal redirect: %s%s -> %s\n",
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
    DEBUG_PRINTF("WebPlatform: Redirecting %s to %s\n", server->uri().c_str(),
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
    if (!route.handler || route.method != wmMethod) {
      continue;
    }

    String routePathStr = route.path ? String(route.path) : "";
    bool hasWildcard =
        routePathStr.indexOf('*') >= 0 || routePathStr.indexOf('{') >= 0;
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
    // Process error page through template system for bookmark replacement
    WebRequest errorRequest(server);
    String processedErrorPage = prepareHtml(errorPage, errorRequest);
    server->send(404, "text/html", processedErrorPage);
  } else {
    server->send(404, "text/plain", "Not Found");
  }
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

  // Print final route registry for debugging
  printUnifiedRoutes();

  // For HTTPS-only mode with redirection server, we don't register normal
  // routes on HTTP server We know we're in redirect mode if server exists
  // and HTTPS is enabled with serverPort 443
  bool isRedirectServer =
      (httpsEnabled && serverPort == 443 && server != nullptr);
  // Check if we know this is the HTTP server running on port 80
  bool isHttpRedirectServer = false;
  // For ESP32, we can determine this based on our setup logic
  // If HTTPS is enabled and we have a server, it must be the redirect server
  // on port 80
  isHttpRedirectServer = isRedirectServer;

  if (server && !isHttpRedirectServer) {
    // Setup 404 handler
    server->onNotFound([this]() { handleNotFound(); });
  }
}

void WebPlatform::setupConfigPortalMode() {
  DEBUG_PRINTLN("WebPlatform: Setting up config portal routes");

  // Register main portal routes (using /portal instead of /)
  registerConfigPortalRoutes();

  // register a redirect but do it directly on the server instead of the
  // registry. This means that if the user has tried to add one to the registry,
  // it wont add because this one is already on the server (cant be replace
  // donce fully regsitered)
  server->on("/", HTTP_GET, [this]() {
    server->sendHeader("Location", "/portal");
    server->send(302, "text/plain", "Redirecting to setup...");
  });

  // Bind user overrides from registry (includes asset overrides like CSS)
  bindRegisteredRoutes();

  // Setup captive portal DNS server to redirect all DNS queries to our AP IP
  // This makes PC browsers navigate to the portal when any domain is entered
  dnsServer.start(53, "*", WiFi.softAPIP());
  DEBUG_PRINTF(
      "WebPlatform: Captive portal DNS started - all domains redirect to %s\n",
      WiFi.softAPIP().toString().c_str());
}

void WebPlatform::setupConnectedMode() {
  DEBUG_PRINTLN("WebPlatform: Setting up connected mode routes");

  // Register core platform routes FIRST (before overrides are processed)
  registerConnectedModeRoutes();

  // Register all unified routes with servers (this processes overrides)
  bindRegisteredRoutes();

  if (httpsEnabled && httpsServerHandle) {
    registerUnifiedHttpsRoutes();
  }

DEBUG_PRINTF("\n=== WebPlatform OpenAPI Generation ===\n");
#if OPENAPI_ENABLED
    // Generate OpenAPI spec AFTER all routes are registered (modules + platform
    // routes)
    measureHeapUsage("before openapi spec");
    generateOpenAPISpec();
    measureHeapUsage("after openapi spec")
    DEBUG_PRINTLN("Skipping generation complete.");
#else
    DEBUG_PRINTLN("Skipping spec generation. Add build flag WEB_PLATFORM_OPENAPI=1 to generate.");
#endif
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
  return registerModule(basePath, module, JsonVariant());
}

bool WebPlatform::registerModule(const char *basePath, IWebModule *module,
                                 const JsonVariant &config) {
  if (!module) {
    DEBUG_PRINTLN("WebPlatform: Cannot register null module");
    return false;
  }

  // Only allow pre-registration (before begin() is called)
  if (running) {
    ERROR_PRINTLN("WebPlatform: ERROR - Module registration after begin() is "
                  "not supported");
    ERROR_PRINTLN(
        "WebPlatform: All modules must be registered before calling begin()");
    return false;
  }

  // Check if module already pre-registered
  for (const auto &pendingModule : pendingModules) {
    if (pendingModule.basePath == basePath) {
      DEBUG_PRINTF("WebPlatform: Module already pre-registered at path: %s\n",
                   basePath);
      return false;
    }
  }

  // Store module for initialization during begin()
  pendingModules.emplace_back(basePath, module, config);
  DEBUG_PRINTF("WebPlatform: Pre-registered module '%s' at path: %s\n",
               module->getModuleName().c_str(), basePath);
  return true;
}

// Register routes for a specific module
void WebPlatform::registerModuleRoutesForModule(const String &basePath,
                                                IWebModule *module) {
  DEBUG_PRINTF("  Processing module: %s at path: %s\n",
               module->getModuleName().c_str(), basePath.c_str());

  // Process HTTP routes (now returns RouteVariant)
  auto httpRoutes = module->getHttpRoutes();
  DEBUG_PRINTF("  Module has %d HTTP routes\n", httpRoutes.size());

  for (const auto &routeVariant : httpRoutes) {
    // Extract route info from either variant type
    const WebRoute *webRoute = nullptr;
    OpenAPIDocumentation docs; // Default empty docs

    if (holds_alternative<WebRoute>(routeVariant)) {
      webRoute = &get<WebRoute>(routeVariant);
    } else if (holds_alternative<ApiRoute>(routeVariant)) {
      const ApiRoute &apiRoute = get<ApiRoute>(routeVariant);
      webRoute = &apiRoute.webRoute;
      docs = apiRoute.docs;
    } else {
      continue;
    }

    // Normalize paths by removing leading/trailing slashes for consistent
    // processing
    String normalizedBasePath = basePath;
    String normalizedRoutePath = webRoute->path;

    // Remove leading and trailing slashes from both paths
    if (normalizedBasePath.startsWith("/")) {
      normalizedBasePath = normalizedBasePath.substring(1);
    }
    if (normalizedBasePath.endsWith("/")) {
      normalizedBasePath =
          normalizedBasePath.substring(0, normalizedBasePath.length() - 1);
    }

    if (normalizedRoutePath.startsWith("/")) {
      normalizedRoutePath = normalizedRoutePath.substring(1);
    }
    if (normalizedRoutePath.endsWith("/")) {
      normalizedRoutePath =
          normalizedRoutePath.substring(0, normalizedRoutePath.length() - 1);
    }

    // Build the final path based on whether it's an API route or regular route
    String fullPath;

    if (holds_alternative<ApiRoute>(routeVariant)) {
      // API route: /basePath/api/routePath or /api/routePath (if no basePath)
      if (normalizedBasePath.length() > 0) {
        if (normalizedRoutePath.length() > 0) {
          fullPath = "/" + normalizedBasePath + "/api/" + normalizedRoutePath;
        } else {
          fullPath = "/" + normalizedBasePath + "/api";
        }
      } else {
        if (normalizedRoutePath.length() > 0) {
          fullPath = "/api/" + normalizedRoutePath;
        } else {
          fullPath = "/api";
        }
      }

      registerApiRoute(fullPath, webRoute->unifiedHandler,
                       webRoute->authRequirements, webRoute->method, docs);
    } else {
      // Regular route: /basePath/routePath or /routePath (if no basePath)
      if (normalizedBasePath.length() > 0) {
        if (normalizedRoutePath.length() > 0) {
          fullPath = "/" + normalizedBasePath + "/" + normalizedRoutePath;
        } else {
          fullPath = "/" + normalizedBasePath + "/";
        }
      } else {
        if (normalizedRoutePath.length() > 0) {
          fullPath = "/" + normalizedRoutePath;
        } else {
          fullPath = "/";
        }
      }

      registerWebRoute(fullPath, webRoute->unifiedHandler,
                       webRoute->authRequirements, webRoute->method);
    }
  }
}

// IWebModule interface implementation (for consistency)
std::vector<RouteVariant> WebPlatform::getHttpRoutes() {
  // Return empty routes since WebPlatform manages its own routing
  return std::vector<RouteVariant>();
}

std::vector<RouteVariant> WebPlatform::getHttpsRoutes() {
  return getHttpRoutes();
}

void WebPlatform::initializeRegisteredModules() {
  DEBUG_PRINTF("WebPlatform: Initializing %d registered modules...\n",
               pendingModules.size());

  // Reserve space in registered modules vector to avoid reallocations
  registeredModules.reserve(pendingModules.size());

  // Initialize modules one by one with error handling
  for (auto &pendingModule : pendingModules) {
    DEBUG_PRINTF("  Initializing: %s at %s\n",
                 pendingModule.module->getModuleName().c_str(),
                 pendingModule.basePath.c_str());

    // Initialize module with config if provided
    if (pendingModule.config.size() > 0) {
      pendingModule.module->begin(pendingModule.config.as<JsonVariant>());
    } else {
      pendingModule.module->begin();
    }

    // Move to active registry
    registeredModules.push_back({pendingModule.basePath, pendingModule.module});

    // Register module routes immediately to spread memory usage
    registerModuleRoutesForModule(pendingModule.basePath, pendingModule.module);

    DEBUG_PRINTF("  ✓ Module %s initialized successfully\n",
                 pendingModule.module->getModuleName().c_str());
  }

  // Clear pending modules to free memory
  pendingModules.clear();
  pendingModules.shrink_to_fit(); // Force memory deallocation

  DEBUG_PRINTF("WebPlatform: Successfully initialized %d modules\n",
               registeredModules.size());
}

void WebPlatform::handleRegisteredModules() {
  // Call handle() on all registered modules
  for (const auto &regModule : registeredModules) {
    if (regModule.module) {
      regModule.module->handle();
    }
  }
}

bool WebPlatform::validatePendingModules() {
  DEBUG_PRINTF("WebPlatform: Validating %d pending modules...\n",
               pendingModules.size());

  // Check for duplicate base paths
  for (size_t i = 0; i < pendingModules.size(); i++) {
    for (size_t j = i + 1; j < pendingModules.size(); j++) {
      if (pendingModules[i].basePath == pendingModules[j].basePath) {
        ERROR_PRINTF("ERROR: Duplicate module base path detected: %s\n",
                     pendingModules[i].basePath.c_str());
        return false;
      }
    }
  }

  // Validate module pointers
  for (const auto &pending : pendingModules) {
    if (!pending.module) {
      ERROR_PRINTF("ERROR: Null module pointer for path: %s\n",
                   pending.basePath.c_str());
      return false;
    }

    // Validate base path format
    if (!pending.basePath.startsWith("/")) {
      ERROR_PRINTF("ERROR: Module base path must start with '/': %s\n",
                   pending.basePath.c_str());
      return false;
    }
  }

  DEBUG_PRINTLN("WebPlatform: Module validation passed");
  return true;
}

void WebPlatform::handleInitializationError(const String &error) {
  ERROR_PRINTF("WebPlatform: INITIALIZATION ERROR - %s\n", error.c_str());
  ERROR_PRINTLN("WebPlatform: Falling back to CONFIG_PORTAL mode");

  // Force config portal mode on initialization error
  currentMode = CONFIG_PORTAL;
  connectionState = WIFI_CONFIG_PORTAL;

  // Clear pending modules to prevent further issues
  pendingModules.clear();

  // Setup basic error page
  IWebModule::setErrorPage(
      500, F("<h1>System Initialization Error</h1>"
             "<p>The system encountered an error during startup.</p>"
             "<p>Please check the serial console for details.</p>"
             "<p><a href='/portal'>WiFi Configuration</a></p>"));
}
