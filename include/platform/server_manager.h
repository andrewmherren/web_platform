#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

// ServerManager owns the HTTP/HTTPS server lifecycle: creating/destroying the
// Arduino WebServerClass and/or ESP-IDF HTTPS httpd, certificate wiring (via
// CertificateLoader), and the dual-server (real HTTPS + HTTP-redirect-only)
// setup used when HTTPS is active. It knows nothing about individual routes
// - it takes a Router reference at construction and hands it the live
// server/handle to bind routes onto, once at startup and again once routes
// have actually been registered (see bindRoutes()).

#include "platform/router.h"
#include <Arduino.h>
#include <functional>

#ifdef ESP_PLATFORM
#include <DNSServer.h>
#include <WebServer.h>
#include <esp_https_server.h>
#endif

class ServerManager {
public:
  struct Config {
    uint16_t maxUriHandlers;
    uint16_t stackSize;
  };

  explicit ServerManager(Router &router);
  ~ServerManager();

  // Starts the server(s) for the current mode. wantHttps requests HTTPS;
  // actual isHttpsEnabled() afterward may still be false (no certs, start
  // failure) - in that case this falls back to HTTP automatically, same as
  // before. When HTTPS does start successfully, a second, HTTP-only server
  // is also started on port 80 solely to redirect to HTTPS.
  void start(bool wantHttps, const Config &config);
  void stop();

  // Config portal mode always forces HTTP (captive portal compatibility) -
  // pass isConfigPortalMode so that's reflected here rather than relying on
  // the caller to skip calling this correctly.
  bool detectHttpsCapability(bool isConfigPortalMode) const;

  bool isHttpsEnabled() const { return httpsEnabled; }
  int getPort() const { return serverPort; }
  bool isRunning() const { return running; }

  void handleClient();

  // Binds every currently-registered route onto whatever server(s) are
  // live. Called once right after start() (when the route registry is
  // still empty - a no-op for HTTP, and for HTTPS registers only the 404
  // handler) and again after all routes have actually been registered.
  void bindRoutes(std::function<void()> notFoundFallback);

#ifdef ESP_PLATFORM
  WebServerClass *getHttpServer() const { return server; }
  DNSServer &getDnsServer() { return dnsServer; }
#endif

private:
  void configureHttpsServer(const Config &config);

  Router &router;

#ifdef ESP_PLATFORM
  WebServerClass *server = nullptr;
  DNSServer dnsServer;
  httpd_handle_t httpsServerHandle = nullptr;
#else
  void *server = nullptr;
  void *httpsServerHandle = nullptr;
#endif

  bool httpsEnabled = false;
  bool running = false;
  int serverPort = 80;
};

#endif // SERVER_MANAGER_H
