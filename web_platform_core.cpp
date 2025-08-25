#include "web_platform.h"

// Core implementation of WebPlatform class
// Basic initialization, setup, and handling functions

// Global instance
WebPlatform webPlatform;

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

void WebPlatform::begin(const char *deviceName) {
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

bool WebPlatform::detectHttpsCapability() {
  // Phase 2: Full certificate detection without build flags requirement
  // NOTE: Config portal always uses HTTP for captive portal compatibility
  if (currentMode == CONFIG_PORTAL) {
    Serial.println("WebPlatform: Config portal mode - forcing HTTP for captive "
                   "portal compatibility");
    return false;
  }

#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
  Serial.println("WebPlatform: Checking for SSL certificates...");
  return areCertificatesAvailable();
#else
  Serial.println("WebPlatform: HTTPS not supported on this platform");
  return false;
#endif
}
void WebPlatform::startServer() {
  if (httpsEnabled) {
    serverPort = 443;
    configureHttpsServer();
    // If HTTPS failed, fall back to HTTP
    if (!httpsEnabled) {
      serverPort = 80;
      // Start Arduino WebServer for fallback
      if (server) {
        server->stop();
        delete server;
        server = nullptr;
      }

      server = new WebServerClass(serverPort);
      if (!server) {
        Serial.println("WebPlatform: ERROR - Failed to create web server!");
        return;
      }

      server->begin();
      running = true;
      Serial.printf(
          "WebPlatform: HTTP server started on port %d (HTTPS fallback)\n",
          serverPort);
    } else {
      // HTTPS is working - don't create Arduino WebServer
      server = nullptr;
      running = true;
      Serial.printf("WebPlatform: HTTPS-only server running on port %d\n",
                    serverPort);
    }
  } else {
    // HTTP only mode
    serverPort = 80;
    if (server) {
      server->stop();
      delete server;
      server = nullptr;
    }

    server = new WebServerClass(serverPort);
    if (!server) {
      Serial.println("WebPlatform: ERROR - Failed to create web server!");
      return;
    }

    server->begin();
    running = true;
    Serial.printf("WebPlatform: HTTP server started on port %d\n", serverPort);
  }
}
void WebPlatform::setupRoutes() {
  if (currentMode == CONFIG_PORTAL) {
    setupConfigPortalMode();
  } else {
    setupConnectedMode();
  }

  // Only register Arduino WebServer routes if we have an HTTP server
  if (server) {
    // Register static assets in both modes
    registerStaticAssetRoutes();

    // Setup 404 handler
    server->onNotFound([this]() { handleNotFound(); });
  }
}
void WebPlatform::registerStaticAssetRoutes() {
  if (!server) {
    Serial.println("WebPlatform: No HTTP server to register static asset "
                   "routes on (HTTPS-only mode)");
    return;
  }

  // Special case for theme CSS - make it available at both paths
  server->on("/assets/tickertape-theme.css", HTTP_GET, [this]() {
    StaticAsset cssAsset =
        IWebModule::getStaticAsset("/assets/tickertape-theme.css");
    if (cssAsset.path.length() > 0) {
      String content = cssAsset.useProgmem ? FPSTR(cssAsset.content.c_str())
                                           : cssAsset.content;
      server->send(200, "text/css", content);
    } else {
      // Fall back to default CSS
      server->send(200, "text/css", FPSTR(WEB_UI_DEFAULT_CSS));
    }
  });

  server->on("/assets/style.css", HTTP_GET, [this]() {
    // First check for the specific style.css asset
    StaticAsset cssAsset = IWebModule::getStaticAsset("/assets/style.css");

    // If not found, fall back to tickertape-theme.css
    if (cssAsset.path.length() == 0) {
      cssAsset = IWebModule::getStaticAsset("/assets/tickertape-theme.css");
    }

    // If either was found, serve it
    if (cssAsset.path.length() > 0) {
      String content = cssAsset.useProgmem ? FPSTR(cssAsset.content.c_str())
                                           : cssAsset.content;
      server->send(200, "text/css", content);
    } else {
      // If no custom CSS is found, serve the default theme from IWebModule
      server->send(200, "text/css", FPSTR(WEB_UI_DEFAULT_CSS));
    }
  });

  // Get all static assets from IWebModule
  auto assetRoutes = IWebModule::getStaticAssetRoutes();

  for (const auto &route : assetRoutes) {
    // Copy the route path for lambda capture
    String routePath = route.path;

    server->on(routePath.c_str(), HTTP_GET, [this, routePath]() {
      // Get the static asset content
      StaticAsset asset = IWebModule::getStaticAsset(routePath);

      if (asset.path.length() > 0) {
        // Serve the asset with correct MIME type
        String content =
            asset.useProgmem ? FPSTR(asset.content.c_str()) : asset.content;
        server->send(200, asset.mimeType.c_str(), content);
      } else {
        // Asset not found
        server->send(404, "text/plain", "Asset not found");
      }
    });
  }
  Serial.printf("WebPlatform: Registered %d static asset routes\n",
                assetRoutes.size());
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

    // Check for any redirect rules
    String redirectTarget = IWebModule::getRedirectTarget(server->uri());
    if (redirectTarget.length() > 0) {
      Serial.printf("WebPlatform: Redirecting %s to %s\n",
                    server->uri().c_str(), redirectTarget.c_str());
      server->sendHeader("Location", redirectTarget);
      server->send(302, "text/html", "");
      return;
    }
  } else {
    // In connected mode, also check for redirects
    String redirectTarget = IWebModule::getRedirectTarget(server->uri());
    if (redirectTarget.length() > 0) {
      Serial.printf("WebPlatform: Redirecting %s to %s\n",
                    server->uri().c_str(), redirectTarget.c_str());
      server->sendHeader("Location", redirectTarget);
      server->send(302, "text/html", "");
      return;
    }
  }

  // Use IWebModule error page system
  String errorPage = IWebModule::getErrorPage(404);
  if (errorPage.length() > 0) {
    server->send(404, "text/html", errorPage);
  } else {
    server->send(404, "text/plain", "Not Found");
  }
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

void WebPlatform::printRoutes() const {
  Serial.println("WebPlatform: Registered routes:");
  for (const auto &regModule : registeredModules) {
    auto routes = regModule.module->getHttpRoutes();
    for (const auto &route : routes) {
      String methodStr = (route.method == WebModule::WM_GET) ? "GET" : "POST";
      Serial.printf("  %s %s%s - %s\n", methodStr.c_str(),
                    regModule.basePath.c_str(), route.path.c_str(),
                    route.description.c_str());
    }
  }
}