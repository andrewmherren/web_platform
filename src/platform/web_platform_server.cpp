#include "../../include/interface/web_request.h"
#include "../../include/web_platform.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

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

      server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

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

      server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

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

    server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

    server->begin();
    running = true;
    Serial.printf("WebPlatform: HTTP server started on port %d\n", serverPort);
  }
}

void WebPlatform::configureHttpsServer() {
#if defined(ESP32)
  if (httpsServerHandle) {
    Serial.println("WebPlatform: HTTPS server already running");
    return;
  }

  // Get certificate data
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    Serial.println("WebPlatform: Failed to get certificates for HTTPS");
    httpsEnabled = false;
    return;
  }

  // Clear any existing route paths
  httpsRoutePaths.clear();

  // Configure HTTPS server
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.httpd.server_port = serverPort;
  config.httpd.max_uri_handlers = 50; // Generous limit
  config.httpd.task_priority = 5;
  config.httpd.stack_size = 8192;
  config.httpd.lru_purge_enable = true;

  // Set certificates
  config.cacert_pem = cert_data;
  config.cacert_len = cert_len;
  config.prvtkey_pem = key_data;
  config.prvtkey_len = key_len;

  // Start HTTPS server
  esp_err_t ret = httpd_ssl_start(&httpsServerHandle, &config);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to start HTTPS server: %d\n", ret);
    httpsServerHandle = nullptr;
    httpsEnabled = false;
    return;
  }

  Serial.println("WebPlatform: HTTPS server started successfully");

  registerUnifiedHttpsRoutes();

  httpd_register_err_handler(
      httpsServerHandle, HTTPD_404_NOT_FOUND,
      [](httpd_req_t *req, httpd_err_code_t err) -> esp_err_t {
        // First, check if this is actually a wildcard route match before
        // showing 404
        WebRequest request(req);
        String requestPath = request.getPath();

        // Convert ESP-IDF method back to our WebModule method for comparison
        WebModule::Method wmMethod = WebModule::WM_GET; // default
        if (req->method == HTTP_POST)
          wmMethod = WebModule::WM_POST;
        else if (req->method == HTTP_PUT)
          wmMethod = WebModule::WM_PUT;
        else if (req->method == HTTP_DELETE)
          wmMethod = WebModule::WM_DELETE;
        else if (req->method == HTTP_PATCH)
          wmMethod = WebModule::WM_PATCH;

        // Check all routes including wildcard ones
        for (const auto &route : routeRegistry) {
          if (route.method != wmMethod || route.disabled || !route.handler) {
            continue;
          }

          // Check both exact and wildcard routes
          bool pathMatches = WebPlatform::httpsInstance->pathMatchesRoute(
              route.path, requestPath);
          if (pathMatches) {

            // Set the matched route pattern on the request for parameter
            // extraction
            request.setMatchedRoute(route.path);

            WebResponse response;
            // Use shared execution logic with authentication and CSRF
            WebPlatform::httpsInstance->executeRouteWithAuth(route, request,
                                                             response, "HTTPS");

            return response.sendTo(req);
          }
        }

        // No wildcard match found, show actual 404 page
        String errorPage = IWebModule::getErrorPage(404);
        if (errorPage.length() > 0) {
          // Process error page through template system for bookmark replacement
          String processedErrorPage =
              WebPlatform::httpsInstance->prepareHtml(errorPage, request);

          WebResponse response;
          response.setStatus(404);
          response.setContent(processedErrorPage, "text/html");
          return response.sendTo(req);
        } else {
          // Use a simple default response
          httpd_resp_set_status(req, "404 Not Found");
          httpd_resp_send(req, "Page Not Found", 14);
        }
        return ESP_OK;
      });
  Serial.println("Registered 404 error handler");

  Serial.println("WebPlatform: HTTPS routes registered successfully");
#else
  Serial.println("WebPlatform: HTTPS not supported on this platform");
  httpsEnabled = false;
#endif
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

bool WebPlatform::areCertificatesAvailable() {
#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    Serial.println("WebPlatform: No embedded certificates found");
    return false;
  }

  // Basic validation - check for PEM format
  if (cert_len > 27 && key_len > 27) {
    String certStart((char *)cert_data, 27);
    String keyStart((char *)key_data, 27);

    if (certStart.indexOf("-----BEGIN CERTIFICATE-----") >= 0 &&
        keyStart.indexOf("-----BEGIN") >= 0) {
      Serial.printf("WebPlatform: SSL certificates validated (cert: %d bytes, "
                    "key: %d bytes)\n",
                    cert_len, key_len);
      return true;
    }
  }

  Serial.println("WebPlatform: Invalid certificate format");
  return false;
#else
  Serial.println("WebPlatform: Certificates not supported on this platform");
  return false;
#endif
}

bool WebPlatform::getEmbeddedCertificates(const uint8_t **cert_data,
                                          size_t *cert_len,
                                          const uint8_t **key_data,
                                          size_t *key_len) {
#if defined(ESP32)
  // Check for embedded certificates - these symbols may not exist if
  // certificates weren't embedded
  extern const uint8_t server_cert_pem_start[] asm(
      "_binary_src_server_cert_pem_start") __attribute__((weak));
  extern const uint8_t server_cert_pem_end[] asm(
      "_binary_src_server_cert_pem_end") __attribute__((weak));
  extern const uint8_t server_key_pem_start[] asm(
      "_binary_src_server_key_pem_start") __attribute__((weak));
  extern const uint8_t server_key_pem_end[] asm(
      "_binary_src_server_key_pem_end") __attribute__((weak));

  // Check if certificates are available (weak symbols may be NULL)
  if (!server_cert_pem_start || !server_cert_pem_end || !server_key_pem_start ||
      !server_key_pem_end) {
    return false;
  }

  // Calculate sizes and set pointers
  *cert_len = server_cert_pem_end - server_cert_pem_start;
  *key_len = server_key_pem_end - server_key_pem_start;
  *cert_data = server_cert_pem_start;
  *key_data = server_key_pem_start;

  // Basic sanity check
  return (*cert_len > 100 && *key_len > 100);
#else
  // Not supported on this platform
  return false;
#endif
}