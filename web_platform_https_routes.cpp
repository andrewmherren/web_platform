#include "web_platform.h"

// HTTPS route registration and management
// This file handles registering routes with the ESP-IDF HTTPS server

#if defined(ESP32)

void WebPlatform::registerHttpsRoutes() {
  if (!httpsServerHandle)
    return;

  Serial.println("WebPlatform: Registering HTTPS routes...");

  // Register specific routes BEFORE catch-all (ESP-IDF matches in order)

  // Root route
  httpd_uri_t root_route = {.uri = "/",
                            .method = HTTP_GET,
                            .handler = httpsGenericHandler,
                            .user_ctx = nullptr};
  esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &root_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register root route: %d\n", ret);
  } else {
    Serial.println("WebPlatform: Root route / registered");
  }

  // Status route - register both with and without trailing slash
  httpd_uri_t status_route = {.uri = "/status",
                              .method = HTTP_GET,
                              .handler = httpsGenericHandler,
                              .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &status_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register status route: %d\n", ret);
  } else {
    Serial.println("WebPlatform: Status route /status registered");
  }

  // Also register with trailing slash
  httpd_uri_t status_route_trailing = {.uri = "/status/",
                                       .method = HTTP_GET,
                                       .handler = httpsGenericHandler,
                                       .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &status_route_trailing);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register status route with trailing "
                  "slash: %d\n",
                  ret);
  } else {
    Serial.println("WebPlatform: Status route /status/ registered");
  }

  // WiFi route (specially added here because it's critical)
  // Register both with and without trailing slash
  httpd_uri_t wifi_route = {.uri = "/wifi",
                            .method = HTTP_GET,
                            .handler = httpsGenericHandler,
                            .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &wifi_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register wifi route: %d\n", ret);
  } else {
    Serial.println("WebPlatform: WiFi route /wifi registered");
  }

  // Also register with trailing slash
  httpd_uri_t wifi_route_trailing = {.uri = "/wifi/",
                                     .method = HTTP_GET,
                                     .handler = httpsGenericHandler,
                                     .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &wifi_route_trailing);
  if (ret != ESP_OK) {
    Serial.printf(
        "WebPlatform: Failed to register wifi route with trailing slash: %d\n",
        ret);
  } else {
    Serial.println("WebPlatform: WiFi route /wifi/ registered");
  }

  // Static assets are registered at the end of the registration process

  // WiFi API routes (critical for both config portal and connected modes)
  httpd_uri_t wifi_scan_route = {.uri = "/api/scan",
                                 .method = HTTP_GET,
                                 .handler = httpsGenericHandler,
                                 .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &wifi_scan_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register /api/scan route: %d\n", ret);
  } else {
    Serial.println("WebPlatform: WiFi scan route /api/scan registered");
  }

  httpd_uri_t wifi_status_route = {.uri = "/api/status",
                                   .method = HTTP_GET,
                                   .handler = httpsGenericHandler,
                                   .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &wifi_status_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register /api/status route: %d\n",
                  ret);
  } else {
    Serial.println("WebPlatform: WiFi status route /api/status registered");
  }

  httpd_uri_t wifi_connect_route = {.uri = "/api/connect",
                                    .method = HTTP_POST,
                                    .handler = httpsGenericHandler,
                                    .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &wifi_connect_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register /api/connect route: %d\n",
                  ret);
  } else {
    Serial.println("WebPlatform: WiFi connect route /api/connect registered");
  }

  httpd_uri_t wifi_reset_route = {.uri = "/api/reset",
                                  .method = HTTP_POST,
                                  .handler = httpsGenericHandler,
                                  .user_ctx = nullptr};
  ret = httpd_register_uri_handler(httpsServerHandle, &wifi_reset_route);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to register /api/reset route: %d\n",
                  ret);
  } else {
    Serial.println("WebPlatform: WiFi reset route /api/reset registered");
  }

  // Register module routes (in connected mode)
  if (currentMode == CONNECTED) {
    registerHttpsModuleRoutes();
  }

  // Register all static assets for HTTPS (implemented in
  // web_platform_https_assets.cpp)
  registerHttpsStaticAssets(); // Verify error page is available
  String testPage = IWebModule::getErrorPage(404);
  Serial.printf("Custom error page (404) available: %d bytes\n",
                testPage.length()); // Register 404 error handler for ESP-IDF
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

  // First register the module's root path
  String moduleRootPath = basePath;
  if (moduleRootPath.endsWith("/")) {
    // If it already has a trailing slash, leave it
  } else {
    // Make sure we also register with trailing slash
    httpsRoutePaths.push_back(moduleRootPath + "/");
    const char *pathWithSlash = httpsRoutePaths.back().c_str();

    httpd_uri_t root_config = {.uri = pathWithSlash,
                               .method = HTTP_GET,
                               .handler = httpsGenericHandler,
                               .user_ctx = nullptr};

    esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &root_config);
    if (ret != ESP_OK) {
      Serial.printf("  Failed to register module root path: %s, error: %d\n",
                    (moduleRootPath + "/").c_str(), ret);
    } else {
      Serial.printf("  Registered module root path: %s\n",
                    (moduleRootPath + "/").c_str());
    }
  }

  // Also register without trailing slash (will redirect)
  httpsRoutePaths.push_back(moduleRootPath);
  const char *pathWithoutSlash = httpsRoutePaths.back().c_str();

  httpd_uri_t root_config_no_slash = {.uri = pathWithoutSlash,
                                      .method = HTTP_GET,
                                      .handler = httpsGenericHandler,
                                      .user_ctx = nullptr};

  esp_err_t ret =
      httpd_register_uri_handler(httpsServerHandle, &root_config_no_slash);
  if (ret != ESP_OK) {
    Serial.printf("  Failed to register module path: %s, error: %d\n",
                  moduleRootPath.c_str(), ret);
  } else {
    Serial.printf("  Registered module path: %s\n", moduleRootPath.c_str());
  }

  // Register each module route
  auto routes = module->getHttpsRoutes();
  for (const auto &route : routes) {
    String fullPath = basePath;
    // Make sure we don't double slash
    if (fullPath.endsWith("/") && route.path.startsWith("/")) {
      fullPath += route.path.substring(1); // Skip leading slash in route.path
    } else if (!fullPath.endsWith("/") && !route.path.startsWith("/")) {
      fullPath += "/" + route.path; // Add slash between
    } else {
      fullPath += route.path; // One has slash, one doesn't
    }

    // Store path permanently for ESP-IDF
    httpsRoutePaths.push_back(fullPath);
    const char *pathPtr = httpsRoutePaths.back().c_str();

    Serial.printf("  Registering HTTPS route: %s %s\n",
                  (route.method == WebModule::WM_GET ? "GET" : "POST"),
                  fullPath.c_str());

    httpd_uri_t uri_config = {
        .uri = pathPtr,
        .method = (route.method == WebModule::WM_GET ? HTTP_GET : HTTP_POST),
        .handler = httpsGenericHandler,
        .user_ctx = nullptr};

    esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &uri_config);
    if (ret != ESP_OK) {
      Serial.printf("    Failed to register: %d\n", ret);
    } else {
      Serial.printf("    Successfully registered: %s\n", fullPath.c_str());
    }
  }
}

void WebPlatform::registerHttpsStaticAssets() {
  if (!httpsServerHandle) {
    Serial.println("WebPlatform: No HTTPS server for static assets");
    return;
  }

  Serial.println("WebPlatform: Registering HTTPS static assets...");

  // Get all static assets from IWebModule
  auto assetRoutes = IWebModule::getStaticAssetRoutes();

  for (const auto &route : assetRoutes) {
    // Store path permanently for ESP-IDF
    httpsRoutePaths.push_back(route.path);
    const char *pathPtr = httpsRoutePaths.back().c_str();

    httpd_uri_t asset_config = {.uri = pathPtr,
                                .method = HTTP_GET,
                                .handler = httpsGenericHandler,
                                .user_ctx = nullptr};

    esp_err_t ret =
        httpd_register_uri_handler(httpsServerHandle, &asset_config);
    if (ret != ESP_OK) {
      Serial.printf("  Failed to register asset %s: %d\n", route.path.c_str(),
                    ret);
    } else {
      Serial.printf("  Registered asset: %s\n", route.path.c_str());
    }
  }

  httpsRoutePaths.push_back("/assets/style.css");
  const char *stylePathPtr = httpsRoutePaths.back().c_str();

  httpd_uri_t style_alias = {.uri = stylePathPtr,
                             .method = HTTP_GET,
                             .handler = httpsGenericHandler,
                             .user_ctx = nullptr};

  esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &style_alias);
  if (ret != ESP_OK) {
    Serial.printf("  Failed to register style.css alias: %d\n", ret);
  } else {
    Serial.println("  Registered style.css alias");
  }

  Serial.printf("WebPlatform: Registered %d HTTPS static assets\n",
                assetRoutes.size() + 1);
}

#endif // ESP32