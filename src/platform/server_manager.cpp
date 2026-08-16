#include "platform/certificate_loader.h"
#include "platform/server_manager.h"
#include "utilities/debug_macros.h"
#include <interface/web_request.h>

ServerManager::ServerManager(Router &router) : router(router) {}

ServerManager::~ServerManager() { stop(); }

void ServerManager::start(bool wantHttps, const Config &config) {
  if (wantHttps) {
    httpsEnabled = true;
    serverPort = 443;
    configureHttpsServer(config);

    if (!httpsEnabled) {
      // HTTPS failed to start (no certs, or httpd_ssl_start failed) - fall
      // back to plain HTTP.
      serverPort = 80;
      if (server) {
        server->stop();
        delete server;
        server = nullptr;
      }

      server = new WebServerClass(serverPort);
      if (!server) {
        ERROR_PRINTLN("ServerManager: ERROR - Failed to create web server!");
        return;
      }

      server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

      server->begin();
      running = true;
      DEBUG_PRINTF(
          "ServerManager: HTTP server started on port %d (HTTPS fallback)\n",
          serverPort);
    } else {
      // HTTPS is working - also create an HTTP server on port 80 whose only
      // job is redirecting to HTTPS.
      serverPort = 443;

      if (server) {
        server->stop();
        delete server;
        server = nullptr;
      }

      server = new WebServerClass(80);
      if (!server) {
        ERROR_PRINTLN(
            "ServerManager: ERROR - Failed to create HTTP redirect server!");
        return;
      }

      server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

      server->onNotFound([this]() {
        String host = server->hostHeader();
        int colonPos = host.indexOf(":");
        if (colonPos >= 0) {
          host = host.substring(0, colonPos);
        }

        String httpsUrl =
            "https://" + host + ":" + String(serverPort) + server->uri();

        if (server->args() > 0) {
          httpsUrl += "?";
          for (int i = 0; i < server->args(); i++) {
            if (i > 0) {
              httpsUrl += "&";
            }
            httpsUrl += server->argName(i) + "=" + server->arg(i);
          }
        }

        DEBUG_PRINTF("ServerManager: Redirecting HTTP request to HTTPS: %s\n",
                     httpsUrl.c_str());
        server->sendHeader("Location", httpsUrl);
        server->sendHeader("Connection", "close");
        server->send(301, "text/plain", "Redirecting to secure connection...");
      });

      server->begin();
      running = true;
      DEBUG_PRINTF("ServerManager: HTTPS server running on port %d with "
                   "HTTP-to-HTTPS redirection on port 80\n",
                   serverPort);
    }
  } else {
    httpsEnabled = false;
    serverPort = 80;
    if (server) {
      server->stop();
      delete server;
      server = nullptr;
    }

    server = new WebServerClass(serverPort);
    if (!server) {
      ERROR_PRINTLN("ServerManager: ERROR - Failed to create web server!");
      return;
    }

    server->collectHeaders(COMMON_HTTP_HEADERS, COMMON_HTTP_HEADERS_COUNT);

    server->begin();
    running = true;
    DEBUG_PRINTF("ServerManager: HTTP server started on port %d\n",
                 serverPort);
  }
}

void ServerManager::configureHttpsServer(const Config &config) {
  if (httpsServerHandle) {
    DEBUG_PRINTLN("ServerManager: HTTPS server already running");
    return;
  }

  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!CertificateLoader::getEmbeddedCertificates(&cert_data, &cert_len,
                                                  &key_data, &key_len)) {
    DEBUG_PRINTLN("ServerManager: Failed to get certificates for HTTPS");
    httpsEnabled = false;
    return;
  }

  httpd_ssl_config_t httpdConfig = HTTPD_SSL_CONFIG_DEFAULT();
  httpdConfig.httpd.server_port = serverPort;
  httpdConfig.httpd.max_uri_handlers = config.maxUriHandlers;
  httpdConfig.httpd.task_priority = 5;
  httpdConfig.httpd.stack_size = config.stackSize;
  httpdConfig.httpd.lru_purge_enable = true;

  httpdConfig.cacert_pem = cert_data;
  httpdConfig.cacert_len = cert_len;
  httpdConfig.prvtkey_pem = key_data;
  httpdConfig.prvtkey_len = key_len;

  esp_err_t ret = httpd_ssl_start(&httpsServerHandle, &httpdConfig);
  if (ret != ESP_OK) {
    DEBUG_PRINTF("ServerManager: Failed to start HTTPS server: %d\n", ret);
    httpsServerHandle = nullptr;
    httpsEnabled = false;
    return;
  }

  DEBUG_PRINTLN("ServerManager: HTTPS server started successfully");

  // Route registry is still empty at this point (routes are registered
  // after startServer()/start() returns) - this call only wires up the
  // handle for later. bindRoutes() re-runs it once routes actually exist.
  router.bindHttpsRoutes(httpsServerHandle);
  router.registerHttpsNotFoundHandler(httpsServerHandle);

  DEBUG_PRINTLN("ServerManager: HTTPS routes registered successfully");
}

bool ServerManager::detectHttpsCapability(bool isConfigPortalMode) const {
  // Config portal always uses HTTP for captive portal compatibility
  if (isConfigPortalMode) {
    DEBUG_PRINTLN("ServerManager: Config portal mode - forcing HTTP for "
                  "captive portal compatibility");
    return false;
  }

  DEBUG_PRINTLN("ServerManager: Checking for SSL certificates...");
  return CertificateLoader::areCertificatesAvailable();
}

void ServerManager::handleClient() {
  if (server) {
    server->handleClient();
  }
}

void ServerManager::bindRoutes(std::function<void()> notFoundFallback) {
  bool isHttpRedirectServer =
      httpsEnabled && serverPort == 443 && server != nullptr;

  if (server) {
    router.bindHttp(server, isHttpRedirectServer, notFoundFallback);
  }

  if (httpsEnabled && httpsServerHandle) {
    router.bindHttpsRoutes(httpsServerHandle);
  }
}

void ServerManager::stop() {
  if (!running) {
    return;
  }

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

  running = false;
}
