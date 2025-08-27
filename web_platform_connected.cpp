#include "assets/connected_home_html.h"
#include "assets/system_status_html.h"
#include "assets/wifi_management_html.h"
#include "web_platform.h"
#include <web_module_interface.h>

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// WebPlatform connected mode implementation
// This file contains functions specific to the connected mode

String WebPlatform::handleConnectedRoot() {
  String html = FPSTR(CONNECTED_HOME_HTML);

  // Replace template variables with actual values
  html.replace("{{DEVICE_NAME}}", deviceName);
  html.replace("{{WIFI_SSID}}", WiFi.SSID());
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
  html.replace("{{UPTIME}}", String(millis() / 1000));
  html.replace("{{SERVER_PROTOCOL}}",
               String(httpsEnabled ? "HTTPS (Secure)" : "HTTP"));
  html.replace("{{SERVER_PORT}}", String(serverPort));
  html.replace("{{HOSTNAME}}", getHostname());
  html.replace("{{FREE_MEMORY}}", String(ESP.getFreeHeap() / 1024));

  // Build module list HTML
  String moduleListHtml = "";
  if (registeredModules.size() > 0) {
    for (const auto &module : registeredModules) {
      moduleListHtml += R"(
            <div class="module-item">
                <div>
                    <strong>)" +
                        module.module->getModuleName() + R"(</strong> 
                    <small>v)" +
                        module.module->getModuleVersion() + R"(</small>
                </div>
                <div>
                    <a href=")" +
                        module.basePath +
                        R"(" class="btn btn-secondary">Open</a>
                </div>
            </div>)";
    }
  } else {
    moduleListHtml = "<p>No modules registered.</p>";
  }
  html.replace("{{MODULE_LIST}}", moduleListHtml);

  return IWebModule::injectNavigationMenu(html);
}

String WebPlatform::handleSystemStatus() {
  String html = FPSTR(SYSTEM_STATUS_HTML);

  // Replace template variables with actual values
  html.replace("{{DEVICE_NAME}}", deviceName);
  html.replace("{{UPTIME}}", String(millis() / 1000));
  html.replace("{{FREE_HEAP}}", String(ESP.getFreeHeap()));
  html.replace(
      "{{PLATFORM_MODE}}",
      String(currentMode == CONNECTED ? "Connected" : "Config Portal"));
  html.replace("{{WIFI_SSID}}", WiFi.SSID());
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{HOSTNAME}}", getHostname());
  html.replace("{{MAC_ADDRESS}}", WiFi.macAddress());
  html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
  html.replace("{{SERVER_PORT}}", String(serverPort));
  html.replace("{{HTTPS_STATUS}}",
               String(httpsEnabled ? "Enabled" : "Disabled"));
  html.replace("{{MODULE_COUNT}}", String(registeredModules.size()));
  html.replace("{{ROUTE_COUNT}}", String(getRouteCount()));

  // Build module table HTML
  String moduleTableHtml = "";
  for (const auto &module : registeredModules) {
    moduleTableHtml += R"(
                <tr>
                    <td>)" +
                       module.module->getModuleName() + R"(</td>
                    <td>)" +
                       module.module->getModuleVersion() + R"(</td>
                    <td>)" +
                       module.basePath + R"(</td>
                </tr>)";
  }
  html.replace("{{MODULE_TABLE}}", moduleTableHtml);

  return IWebModule::injectNavigationMenu(html);
}

String WebPlatform::handleWiFiManagement() {
  String html = FPSTR(WIFI_MANAGEMENT_HTML);

  // Replace template variables with actual values
  html.replace("{{DEVICE_NAME}}", deviceName);
  html.replace("{{CURRENT_SSID}}", WiFi.SSID());
  html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{MAC_ADDRESS}}", WiFi.macAddress());

  return IWebModule::injectNavigationMenu(html);
}

void WebPlatform::setupConnectedMode() {
  Serial.println("WebPlatform: Setting up connected mode routes");

  // Register core platform routes FIRST (before overrides are processed)
  registerConnectedModeRoutes();

  // Convert legacy module routes to unified system
  convertModuleRoutesToUnified();

  // Register all unified routes with servers (this processes overrides)
  registerUnifiedRoutes();

#if defined(ESP32)
  if (httpsEnabled && httpsServerHandle) {
    registerUnifiedHttpsRoutes();
  }
#endif
}

void WebPlatform::registerConnectedModeRoutes() {
  Serial.println(
      "WebPlatform: Registering core platform routes with unified system");

  // Home page
  this->registerRoute(
      "/",
      [this](WebRequest &req, WebResponse &res) {
        IWebModule::setCurrentPath("/");
        String response = handleConnectedRoot();
        res.setContent(response, "text/html");
      },
      WebModule::WM_GET);

  // System status
  this->registerRoute(
      "/status",
      [this](WebRequest &req, WebResponse &res) {
        IWebModule::setCurrentPath("/status");
        String response = handleSystemStatus();
        res.setContent(response, "text/html");
      },
      WebModule::WM_GET);

  // WiFi management page
  this->registerRoute(
      "/wifi",
      [this](WebRequest &req, WebResponse &res) {
        IWebModule::setCurrentPath("/wifi");
        String response = handleWiFiManagement();
        res.setContent(response, "text/html");
      },
      WebModule::WM_GET);

  // WiFi API endpoints
  auto connectHandler = [this](WebRequest &req, WebResponse &res) {
    String ssid = req.getParam("ssid");
    String password = req.getParam("password");

    if (ssid.length() > 0) {
      saveWiFiCredentials(ssid, password);

      res.setContent("{\"status\": \"restarting\", \"message\": \"Connecting "
                     "to new network...\"}",
                     "application/json");

      // Schedule restart after response is sent
      // Note: This is a simplified approach - in a real implementation you
      // might want to use a timer
      delay(100); // Give time for response to be sent
      ESP.restart();
    } else {
      res.setStatus(400);
      res.setContent(
          "{\"status\": \"error\", \"message\": \"Invalid SSID provided\"}",
          "application/json");
    }
  };

  // WiFi scan API
  this->registerRoute(
      "/wifi/api/scan",
      [this](WebRequest &req, WebResponse &res) {
        String response = handleWiFiScanAPI();
        res.setContent(response, "application/json");
      },
      WebModule::WM_GET);

  this->registerRoute(
      "/api/scan",
      [this](WebRequest &req, WebResponse &res) {
        String response = handleWiFiScanAPI();
        res.setContent(response, "application/json");
      },
      WebModule::WM_GET);

  // WiFi status API
  this->registerRoute(
      "/wifi/api/status",
      [this](WebRequest &req, WebResponse &res) {
        String response = handleWiFiStatusAPI();
        res.setContent(response, "application/json");
      },
      WebModule::WM_GET);

  this->registerRoute(
      "/api/status",
      [this](WebRequest &req, WebResponse &res) {
        String response = handleWiFiStatusAPI();
        res.setContent(response, "application/json");
      },
      WebModule::WM_GET);

  // WiFi connect API
  this->registerRoute("/wifi/api/connect", connectHandler, WebModule::WM_POST);
  this->registerRoute("/api/connect", connectHandler, WebModule::WM_POST);

  // WiFi reset API
  this->registerRoute(
      "/wifi/api/reset",
      [this](WebRequest &req, WebResponse &res) {
        String response = handleWiFiResetAPI();
        res.setContent(response, "application/json");
      },
      WebModule::WM_POST);

  this->registerRoute(
      "/api/reset",
      [this](WebRequest &req, WebResponse &res) {
        String response = handleWiFiResetAPI();
        res.setContent(response, "application/json");
      },
      WebModule::WM_POST);
}

void WebPlatform::registerModuleRoutes() {
  // Skip if no server or if we're in HTTPS-only mode with a redirect server
  // (port 80)
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

  if (!server || isHttpRedirectServer) {
    Serial.println("WebPlatform: No HTTP server to register module routes on "
                   "or running in HTTP redirect mode");
    return;
  }

  // Register routes for all registered modules
  Serial.println("WebPlatform: Registering module routes...");

  for (const auto &regModule : registeredModules) {
    Serial.printf("WebPlatform: Processing module '%s' at path: %s\n",
                  regModule.module->getModuleName().c_str(),
                  regModule.basePath.c_str());

    auto routes = regModule.module->getHttpRoutes();
    Serial.printf("WebPlatform: Module has %d routes\n", routes.size());

    for (const auto &route : routes) {
      // Create both versions of the path - with and without trailing slash
      String basePath = regModule.basePath;
      String routePath = route.path;

      // Handle base path and route path slash combinations
      String fullPath;
      if (basePath.endsWith("/") && routePath.startsWith("/")) {
        fullPath = basePath + routePath.substring(1); // Remove duplicate slash
      } else if (!basePath.endsWith("/") && !routePath.startsWith("/")) {
        fullPath = basePath + "/" + routePath; // Add missing slash
      } else {
        fullPath = basePath + routePath; // Perfect match
      }

      // Also create a version with trailing slash for non-root paths
      String fullPathWithTrailing = fullPath;
      if (!fullPath.endsWith("/") && routePath != "/") {
        fullPathWithTrailing += "/";
      }

      String methodStr = (route.method == WebModule::WM_GET) ? "GET" : "POST";
      Serial.printf("WebPlatform: Registering %s %s\n", methodStr.c_str(),
                    fullPath.c_str());

      // Make a copy of the module and route for lambda capture
      IWebModule *module = regModule.module;
      WebRoute routeCopy = route;
      String moduleBasePath = regModule.basePath;

      auto handlerFunction = [this, module, routeCopy, moduleBasePath]() {
        // Set current path for navigation highlighting
        IWebModule::setCurrentPath(moduleBasePath);

        // Extract URL parameters
        std::map<String, String> params;
        for (int i = 0; i < server->args(); i++) {
          params[server->argName(i)] = server->arg(i);
        }

        // Call the module's handler with appropriate data
        String response;
        if (routeCopy.method == WebModule::WM_POST) {
          response = routeCopy.handler(server->arg("plain"), params);
        } else {
          response = routeCopy.handler("", params);
        }

        server->send(200, routeCopy.contentType.c_str(), response);
      };

      // Register both versions of the path
      if (route.method == WebModule::WM_GET) {
        server->on(fullPath.c_str(), HTTP_GET, handlerFunction);

        // Register trailing slash version if different
        if (fullPath != fullPathWithTrailing) {
          server->on(fullPathWithTrailing.c_str(), HTTP_GET, handlerFunction);
          Serial.printf("WebPlatform: Also registered %s %s\n",
                        methodStr.c_str(), fullPathWithTrailing.c_str());
        }
      } else if (route.method == WebModule::WM_POST) {
        server->on(fullPath.c_str(), HTTP_POST, handlerFunction);

        // Register trailing slash version if different
        if (fullPath != fullPathWithTrailing) {
          server->on(fullPathWithTrailing.c_str(), HTTP_POST, handlerFunction);
          Serial.printf("WebPlatform: Also registered %s %s\n",
                        methodStr.c_str(), fullPathWithTrailing.c_str());
        }
      }
    }
  }

  Serial.println("WebPlatform: Module routes registered");
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

  // Debug: Show module routes
  auto httpRoutes = module->getHttpRoutes();
  auto httpsRoutes = module->getHttpsRoutes();
  Serial.printf("  HTTP routes: %d, HTTPS routes: %d\n", httpRoutes.size(),
                httpsRoutes.size());

  for (const auto &route : httpsRoutes) {
    Serial.printf("    HTTPS: %s %s\n",
                  (route.method == WebModule::WM_GET ? "GET" : "POST"),
                  route.path.c_str());
  }

  // Register HTTP routes if HTTP server exists
  registerModuleRoutes();

// Register HTTPS routes for this module
#if defined(ESP32)
  if (httpsServerHandle && httpsEnabled) {
    Serial.printf("WebPlatform: Registering HTTPS routes for module: %s\n",
                  module->getModuleName().c_str());
    registerHttpsModuleRoutesForModule(basePath, module);
  }
#endif

  return true;
}

size_t WebPlatform::getRouteCount() const {
  // Basic route counting - will be enhanced in later phases
  size_t count = 0;
  for (const auto &regModule : registeredModules) {
    count += regModule.module->getHttpRoutes().size();
  }
  return count;
}