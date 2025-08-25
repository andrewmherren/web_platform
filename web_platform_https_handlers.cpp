#include "web_platform.h"

// HTTPS request handling and processing
// This file handles all HTTPS request processing and response generation

#if defined(ESP32)

// Static instance pointer for ESP-IDF callbacks
WebPlatform *WebPlatform::httpsInstance = nullptr;

esp_err_t WebPlatform::httpsGenericHandler(httpd_req_t *req) {
  if (!httpsInstance) {
    Serial.println("ERROR: httpsInstance is null!");
    const char *error = "Internal server error - no instance";
    httpd_resp_send(req, error, strlen(error));
    return ESP_FAIL;
  }

  if (!req) {
    Serial.println("ERROR: req is null!");
    const char *error = "Internal server error - no request";
    httpd_resp_send(req, error, strlen(error));
    return ESP_FAIL;
  }

  String uri = String(req->uri);
  String method = String(http_method_str((httpd_method_t)req->method));

  Serial.printf("HTTPS Request: %s %s\n", method.c_str(), uri.c_str());

  // Handle CSS and JS first - make sure styling works
  if (uri.indexOf(".css") > 0 || uri.indexOf("/assets/") >= 0) {
    // Try to serve as static asset
    StaticAsset asset = IWebModule::getStaticAsset(uri);
    if (asset.path.length() > 0) {
      Serial.printf("Serving static asset: %s (%s)\n", uri.c_str(),
                    asset.mimeType.c_str());
      String content =
          asset.useProgmem ? FPSTR(asset.content.c_str()) : asset.content;
      httpd_resp_set_type(req, asset.mimeType.c_str());
      httpd_resp_send(req, content.c_str(), content.length());
      return ESP_OK;
    }

    // Special case for theme CSS
    if (uri == "/assets/tickertape-theme.css" || uri == "/assets/style.css") {
      // First check for the specific file
      StaticAsset cssAsset = IWebModule::getStaticAsset(uri);

      // If style.css not found, try tickertape-theme.css
      if (cssAsset.path.length() == 0 && uri == "/assets/style.css") {
        cssAsset = IWebModule::getStaticAsset("/assets/tickertape-theme.css");
      }

      if (cssAsset.path.length() > 0) {
        String content = cssAsset.useProgmem ? FPSTR(cssAsset.content.c_str())
                                             : cssAsset.content;
        httpd_resp_set_type(req, "text/css");
        httpd_resp_send(req, content.c_str(), content.length());
        return ESP_OK;
      } else {
        // No custom CSS found, serve default theme
        httpd_resp_set_type(req, "text/css");
        httpd_resp_send(req, WEB_UI_DEFAULT_CSS, strlen(WEB_UI_DEFAULT_CSS));
        return ESP_OK;
      }
    }
  }

  // Handle other static assets
  StaticAsset asset = IWebModule::getStaticAsset(uri);
  if (asset.path.length() > 0) {
    Serial.printf("Serving static asset: %s (%s)\n", uri.c_str(),
                  asset.mimeType.c_str());
    String content =
        asset.useProgmem ? FPSTR(asset.content.c_str()) : asset.content;
    httpd_resp_set_type(req, asset.mimeType.c_str());
    httpd_resp_send(req, content.c_str(), content.length());
    return ESP_OK;
  }

  // Check for redirects before routing
  String redirectTarget = IWebModule::getRedirectTarget(uri);
  if (redirectTarget.length() > 0) {
    Serial.printf("Redirecting %s to %s\n", uri.c_str(),
                  redirectTarget.c_str());
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", redirectTarget.c_str());
    httpd_resp_send(req, "", 0);
    return ESP_OK;
  }

  // Handle dynamic routes based on current mode
  if (httpsInstance->currentMode == CONFIG_PORTAL) {
    return httpsInstance->handleHttpsConfigPortal(req);
  } else {
    return httpsInstance->handleHttpsConnected(req);
  }
}

esp_err_t WebPlatform::handleHttpsConfigPortal(httpd_req_t *req) {
  String uri = String(req->uri);
  String method = String(http_method_str((httpd_method_t)req->method));

  if (uri == "/" && method == "GET") {
    String response = handleConfigPortalRoot();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
  }

  if (uri == "/save" && method == "POST") {
    // Extract POST body
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret > 0) {
      buf[ret] = '\0';
      String response = handleConfigPortalSave(String(buf));
      httpd_resp_set_type(req, "text/html");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }
  }

  if (uri.startsWith("/api/")) {
    if (uri == "/api/status") {
      String response = handleWiFiStatusAPI();
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }

    if (uri == "/api/scan") {
      String response = handleWiFiScanAPI();
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }

    if (uri == "/api/reset" && req->method == HTTP_POST) {
      String response = handleWiFiResetAPI();
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }

    // API not found
    httpd_resp_set_status(req, "404 Not Found");
    const char *error = "{\"error\":\"API endpoint not found\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, error, strlen(error));
    return ESP_OK;
  }

  // Not found
  httpd_resp_set_status(req, "404 Not Found");
  httpd_resp_send(req, "Not Found", 9);
  return ESP_OK;
}

esp_err_t WebPlatform::handleHttpsConnected(httpd_req_t *req) {
  String uri = String(req->uri);
  String method = String(http_method_str((httpd_method_t)req->method));

  Serial.printf("HTTPS Request: %s %s\n", method.c_str(), uri.c_str());
  Serial.printf("Registered modules: %d\n", registeredModules.size());

  // Debug: List all registered modules
  for (const auto &regModule : registeredModules) {
    Serial.printf("  Module: %s at path: %s\n",
                  regModule.module->getModuleName().c_str(),
                  regModule.basePath.c_str());
  }

  if (uri == "/") {
    IWebModule::setCurrentPath("/");
    String response = handleConnectedRoot();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
  }

  if (uri == "/status") {
    IWebModule::setCurrentPath("/status");
    String response = handleSystemStatus();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
  }

  if (uri == "/wifi") {
    IWebModule::setCurrentPath("/wifi");
    String response = handleWiFiManagement();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
  }

  // Handle WiFi API endpoints - support both /api/* and /wifi/api/* patterns
  if (uri.startsWith("/wifi/api/") || uri.startsWith("/api/")) {
    Serial.printf("WiFi API endpoint detected: %s\n", uri.c_str());

    if ((uri == "/wifi/api/scan" || uri == "/api/scan") && method == "GET") {
      Serial.println("Handling WiFi scan request via HTTPS");
      String response = handleWiFiScanAPI();
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }

    if ((uri == "/wifi/api/status" || uri == "/api/status") &&
        method == "GET") {
      String response = handleWiFiStatusAPI();
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }

    if ((uri == "/wifi/api/connect" || uri == "/api/connect") &&
        method == "POST") {
      // Extract POST body
      char buf[512];
      int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
      if (ret > 0) {
        buf[ret] = '\0';
        String postBody = String(buf);

        String ssid = extractPostParameter(postBody, "ssid");
        String password = extractPostParameter(postBody, "password");

        if (ssid.length() > 0) {
          saveWiFiCredentials(ssid, password);

          const char *response = "{\"status\": \"restarting\", \"message\": "
                                 "\"Connecting to new network...\"}";
          httpd_resp_set_type(req, "application/json");
          httpd_resp_send(req, response, strlen(response));

          // Schedule restart to apply new credentials
          delay(1000);
          ESP.restart();
        } else {
          const char *response =
              "{\"status\": \"error\", \"message\": \"Invalid SSID provided\"}";
          httpd_resp_set_status(req, "400 Bad Request");
          httpd_resp_set_type(req, "application/json");
          httpd_resp_send(req, response, strlen(response));
        }
        return ESP_OK;
      }
    }

    if ((uri == "/wifi/api/reset" || uri == "/api/reset") && method == "POST") {
      String response = handleWiFiResetAPI();
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, response.c_str(), response.length());
      return ESP_OK;
    }

    // WiFi API endpoint not found
    const char *error = "{\"error\":\"WiFi API endpoint not found\"}";
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, error, strlen(error));
    return ESP_OK;
  }

  // Try to find matching module route
  for (const auto &regModule : registeredModules) {
    Serial.printf("Checking if %s starts with %s\n", uri.c_str(),
                  regModule.basePath.c_str());

    if (uri.startsWith(regModule.basePath)) {
      String relativePath = uri.substring(regModule.basePath.length());
      if (relativePath.isEmpty())
        relativePath = "/";

      Serial.printf("Module %s matched! Relative path: %s\n",
                    regModule.module->getModuleName().c_str(),
                    relativePath.c_str());

      // Find matching route in module
      auto routes = regModule.module->getHttpsRoutes();
      Serial.printf("Module has %d HTTPS routes\n", routes.size());

      for (const auto &route : routes) {
        Serial.printf("  Checking route: %s (method: %d)\n", route.path.c_str(),
                      route.method);

        // Check both path and method
        bool pathMatches = (route.path == relativePath);
        bool methodMatches =
            ((route.method == WebModule::WM_GET && req->method == HTTP_GET) ||
             (route.method == WebModule::WM_POST && req->method == HTTP_POST));

        Serial.printf("  Path matches: %s, Method matches: %s\n",
                      pathMatches ? "YES" : "NO", methodMatches ? "YES" : "NO");

        if (pathMatches && methodMatches) {
          Serial.printf("Found matching route: %s %s\n", method.c_str(),
                        route.path.c_str());

          // Set current path for navigation
          IWebModule::setCurrentPath(regModule.basePath);

          // Extract parameters and body as needed
          std::map<String, String> params;
          String body = "";

          if (req->method == HTTP_POST) {
            char buf[512];
            int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
            if (ret > 0) {
              buf[ret] = '\0';
              body = String(buf);
            }
          }

          String response = route.handler(body, params);
          httpd_resp_set_type(req, route.contentType.c_str());
          httpd_resp_send(req, response.c_str(), response.length());
          return ESP_OK;
        }
      }

      Serial.printf("No matching route found in module %s\n",
                    regModule.module->getModuleName().c_str());
    }
  }

  // Check for redirects
  String redirectTarget = IWebModule::getRedirectTarget(uri);
  if (redirectTarget.length() > 0) {
    Serial.printf("Redirecting %s to %s\n", uri.c_str(),
                  redirectTarget.c_str());
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", redirectTarget.c_str());
    httpd_resp_send(req, "", 0);
    return ESP_OK;
  }

  Serial.printf("No route found for %s %s\n", method.c_str(), uri.c_str());

  // Not found - use custom error page
  String errorPage = IWebModule::getErrorPage(404);
  if (errorPage.length() > 0) {
    Serial.println("Using custom 404 error page");

    // Make sure CSS is included in the error page
    if (errorPage.indexOf("/assets/tickertape-theme.css") == -1 &&
        errorPage.indexOf("/assets/style.css") == -1) {
      int headEnd = errorPage.indexOf("</head>");
      if (headEnd > 0) {
        errorPage =
            errorPage.substring(0, headEnd) +
            "<link rel=\"stylesheet\" href=\"/assets/tickertape-theme.css\">" +
            errorPage.substring(headEnd);
      }
    }

    // Add navigation
    errorPage = IWebModule::injectNavigationMenu(errorPage);

    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, errorPage.c_str(), errorPage.length());
  } else {
    Serial.println("Using default 404 response");
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_send(req, "Not Found", 9);
  }
  return ESP_OK;
}

#endif // ESP32