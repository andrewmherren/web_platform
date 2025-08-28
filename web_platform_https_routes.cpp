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

// Legacy HTTPS module route registration methods removed as part of Phase 1
// migration All module registration now happens through the unified route
// system

#endif // ESP32