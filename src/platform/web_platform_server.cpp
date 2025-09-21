#include "interface/web_request.h"
#include "web_platform.h"
#include <WebServer.h>

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
        ERROR_PRINTLN("WebPlatform: ERROR - Failed to create web server!");
        return;
      }

      server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

      server->begin();
      running = true;
      DEBUG_PRINTF(
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
        ERROR_PRINTLN(
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

        DEBUG_PRINTF("WebPlatform: Redirecting HTTP request to HTTPS: %s\n",
                     httpsUrl.c_str());
        server->sendHeader("Location", httpsUrl);
        server->sendHeader("Connection", "close");
        server->send(301, "text/plain", "Redirecting to secure connection...");
      });

      // Register no routes on the HTTP server - everything should redirect to
      // HTTPS

      server->begin();
      running = true;
      DEBUG_PRINTF("WebPlatform: HTTPS server running on port %d with "
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
      ERROR_PRINTLN("WebPlatform: ERROR - Failed to create web server!");
      return;
    }

    server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

    server->begin();
    running = true;
    DEBUG_PRINTF("WebPlatform: HTTP server started on port %d\n", serverPort);
  }
}

void WebPlatform::configureHttpsServer() {
  if (httpsServerHandle) {
    DEBUG_PRINTLN("WebPlatform: HTTPS server already running");
    return;
  }

  // Get certificate data
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    DEBUG_PRINTLN("WebPlatform: Failed to get certificates for HTTPS");
    httpsEnabled = false;
    return;
  }

  // Clear any existing route paths
  httpsRoutePaths.clear();

  // Configure HTTPS server
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.httpd.server_port = serverPort;
  config.httpd.max_uri_handlers = platformConfig.maxUriHandlers;
  config.httpd.task_priority = 5;
  config.httpd.stack_size = platformConfig.stackSize;
  config.httpd.lru_purge_enable = true;

  // Set certificates
  config.cacert_pem = cert_data;
  config.cacert_len = cert_len;
  config.prvtkey_pem = key_data;
  config.prvtkey_len = key_len;

  // Start HTTPS server
  esp_err_t ret = httpd_ssl_start(&httpsServerHandle, &config);
  if (ret != ESP_OK) {
    DEBUG_PRINTF("WebPlatform: Failed to start HTTPS server: %d\n", ret);
    httpsServerHandle = nullptr;
    httpsEnabled = false;
    return;
  }

  DEBUG_PRINTLN("WebPlatform: HTTPS server started successfully");

  registerUnifiedHttpsRoutes();

  httpd_register_err_handler(
      httpsServerHandle, HTTPD_404_NOT_FOUND,
      [](httpd_req_t *req, httpd_err_code_t err) -> esp_err_t {
        // First, check if this is actually a wildcard route match before
        // showing 404
        WebRequest request(req);
        String requestPath = request.getPath();

        // Convert ESP-IDF method back to our WebModule method for comparison
        WebModule::Method wmMethod =
            httpMethodToWMMethod((HTTPMethod)req->method);

        // Check all routes including wildcard ones
        for (const auto &route : routeRegistry) {
          if (route.method != wmMethod || !route.handler) {
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
  DEBUG_PRINTLN("Registered 404 error handler");

  DEBUG_PRINTLN("WebPlatform: HTTPS routes registered successfully");
}

bool WebPlatform::detectHttpsCapability() {
  // Full certificate detection without build flags requirement
  // NOTE: Config portal always uses HTTP for captive portal compatibility
  if (currentMode == CONFIG_PORTAL) {
    DEBUG_PRINTLN("WebPlatform: Config portal mode - forcing HTTP for captive "
                  "portal compatibility");
    return false;
  }

  DEBUG_PRINTLN("WebPlatform: Checking for SSL certificates...");
  return areCertificatesAvailable();
}

bool WebPlatform::areCertificatesAvailable() {
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    DEBUG_PRINTLN("WebPlatform: No embedded certificates found");
    return false;
  }

  // Basic validation - check for PEM format
  if (cert_len > 27 && key_len > 27) {
    String certStart((char *)cert_data, 27);
    String keyStart((char *)key_data, 27);

    if (certStart.indexOf("-----BEGIN CERTIFICATE-----") >= 0 &&
        keyStart.indexOf("-----BEGIN") >= 0) {
      DEBUG_PRINTF("WebPlatform: SSL certificates validated (cert: %d bytes, "
                   "key: %d bytes)\n",
                   cert_len, key_len);
      return true;
    }
  }

  DEBUG_PRINTLN("WebPlatform: Invalid certificate format");
  return false;
}

bool WebPlatform::getEmbeddedCertificates(const uint8_t **cert_data,
                                          size_t *cert_len,
                                          const uint8_t **key_data,
                                          size_t *key_len) {
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
}