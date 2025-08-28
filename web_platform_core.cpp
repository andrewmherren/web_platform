#include "web_platform.h"
#include <web_ui_styles.h>

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

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
      // HTTPS is working - also create HTTP server to handle redirects to HTTPS
      serverPort = 443;

      // Create a separate HTTP server on port 80 for redirects
      if (server) {
        server->stop();
        delete server;
        server = nullptr;
      }

      server = new WebServerClass(80); // Always use port 80 for HTTP redirects
      if (!server) {
        Serial.println(
            "WebPlatform: ERROR - Failed to create HTTP redirect server!");
        return;
      }

      // Clear any existing routes from the server
      // and set up a single catch-all handler for HTTP→HTTPS redirection
      server->onNotFound([this]() {
        // Extract the hostname without port
        String host = server->hostHeader();
        int colonPos = host.indexOf(":");
        if (colonPos >= 0) {
          host = host.substring(0, colonPos);
        }

        // Build the HTTPS URL
        String httpsUrl =
            "https://" + host + ":" + String(serverPort) + server->uri();

        // Add query parameters if present
        if (server->args() > 0) {
          httpsUrl += "?";
          for (int i = 0; i < server->args(); i++) {
            if (i > 0) {
              httpsUrl += "&";
            }
            httpsUrl += server->argName(i) + "=" + server->arg(i);
          }
        }

        Serial.printf("WebPlatform: Redirecting HTTP request to HTTPS: %s\n",
                      httpsUrl.c_str());
        server->sendHeader("Location", httpsUrl);
        server->sendHeader("Connection", "close");
        server->send(301, "text/plain", "Redirecting to secure connection...");
      });

      // Register no routes on the HTTP server - everything should redirect to
      // HTTPS

      server->begin();
      running = true;
      Serial.printf("WebPlatform: HTTPS server running on port %d with "
                    "HTTP-to-HTTPS redirection on port 80\n",
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
    registerAuthRoutes(); // Add authentication routes
  }

  // Print final route registry for debugging
  printUnifiedRoutes();
  validateRoutes();

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