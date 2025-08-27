#include "web_platform.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// HTTPS route registration and management
// This file handles registering routes with the ESP-IDF HTTPS server

// Forward declare RouteEntry struct and routeRegistry since it's now in global
// namespace in web_platform_routes.cpp
struct RouteEntry;
extern std::vector<RouteEntry> routeRegistry;

#if defined(ESP32)

void WebPlatform::registerHttpsRoutes() {
  if (!httpsServerHandle)
    return;

  Serial.println("WebPlatform: Registering HTTPS routes...");

  // In Phase 1 migration, we now use a unified route registration system
  registerUnifiedHttpsRoutes();

  // Register 404 error handler for ESP-IDF
  String testPage = IWebModule::getErrorPage(404);
  Serial.printf("Custom error page (404) available: %d bytes\n",
                testPage.length());

  httpd_register_err_handler(
      httpsServerHandle, HTTPD_404_NOT_FOUND,
      [](httpd_req_t *req, httpd_err_code_t err) -> esp_err_t {
        // Try to render custom 404 page
        String errorPage = IWebModule::getErrorPage(404);
        if (errorPage.length() > 0) {
          // Set navigation context and inject menu
          IWebModule::setCurrentPath("/404");
          errorPage = IWebModule::injectNavigationMenu(errorPage);

          httpd_resp_set_status(req, "404 Not Found");
          httpd_resp_set_type(req, "text/html");
          httpd_resp_send(req, errorPage.c_str(), errorPage.length());
        } else {
          // Use a simple default response
          httpd_resp_set_status(req, "404 Not Found");
          httpd_resp_send(req, "Page Not Found", 14);
        }
        return ESP_OK;
      });
  Serial.println("Registered 404 error handler");

  Serial.println("WebPlatform: HTTPS routes registered successfully");
}

// Removed - 404 handling now done through wildcard route in httpsGenericHandler

// Function moved to web_platform_https_assets.cpp

void WebPlatform::registerHttpsModuleRoutes() {
  if (!httpsServerHandle) {
    Serial.println("WebPlatform: No HTTPS server for module routes");
    return;
  }

  Serial.printf("WebPlatform: Registering HTTPS module routes for %d modules\n",
                registeredModules.size());

  for (const auto &regModule : registeredModules) {
    registerHttpsModuleRoutesForModule(regModule.basePath, regModule.module);
  }

  Serial.println("WebPlatform: Module HTTPS routes registration complete");
}

void WebPlatform::registerHttpsModuleRoutesForModule(const String &basePath,
                                                     IWebModule *module) {
  if (!httpsServerHandle || !module) {
    Serial.println("WebPlatform: Cannot register module routes (no server or "
                   "null module)");
    return;
  }

  Serial.printf("Processing module for HTTPS: %s at %s\n",
                module->getModuleName().c_str(), basePath.c_str());

  // No need to register routes manually here, as we now use the unified route
  // system Instead, convert module routes to unified routes

  // Process HTTP routes
  auto httpRoutes = module->getHttpRoutes();
  for (const auto &route : httpRoutes) {
    // Create full path
    String fullPath = basePath;
    if (!fullPath.endsWith("/") && !route.path.startsWith("/")) {
      fullPath += "/";
    } else if (fullPath.endsWith("/") && route.path.startsWith("/")) {
      fullPath = fullPath.substring(0, fullPath.length() - 1);
    }
    fullPath += route.path;

    // Check if this is a unified route or legacy route
    if (route.isUnified && route.unifiedHandler) {
      // Register unified handler directly
      registerRoute(fullPath, route.unifiedHandler, route.method);
    } else if (route.handler) {
      // Convert legacy handler to unified handler
      auto unifiedHandler = [route, module](WebRequest &req, WebResponse &res) {
        // Extract parameters from request
        std::map<String, String> params = req.getAllParams();

        // Call legacy handler
        String result = route.handler(req.getBody(), params);

        // Set response
        res.setContent(result, route.contentType);
      };

      registerRoute(fullPath, unifiedHandler, route.method);
    }
  }

  // Process HTTPS routes (similar logic)
  auto httpsRoutes = module->getHttpsRoutes();
  for (const auto &route : httpsRoutes) {
    String fullPath = basePath;
    if (!fullPath.endsWith("/") && !route.path.startsWith("/")) {
      fullPath += "/";
    } else if (fullPath.endsWith("/") && route.path.startsWith("/")) {
      fullPath = fullPath.substring(0, fullPath.length() - 1);
    }
    fullPath += route.path;

    if (route.isUnified && route.unifiedHandler) {
      registerRoute(fullPath, route.unifiedHandler, route.method);
    } else if (route.handler) {
      auto unifiedHandler = [route, module](WebRequest &req, WebResponse &res) {
        std::map<String, String> params = req.getAllParams();
        String result = route.handler(req.getBody(), params);
        res.setContent(result, route.contentType);
      };

      registerRoute(fullPath, unifiedHandler, route.method);
    }
  }
}

#endif // ESP32