#include "../../include/web_platform.h"
#include "../../include/route_entry.h"
#include "../../include/interface/web_module_interface.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif
// WebPlatform unified route handler implementation
// This file implements the new route registration system for Phase 1

// Define the global vector
std::vector<RouteEntry> routeRegistry;

void WebPlatform::registerRoute(const String &path,
                                WebModule::UnifiedRouteHandler handler,
                                const AuthRequirements &auth,
                                WebModule::Method method) {
  // Check if route already exists
  for (auto &route : routeRegistry) {
    if (route.path == path && route.method == method) {
      if (route.isOverride) {
        Serial.printf("WebPlatform: Route %s %s already overridden, ignoring "
                      "normal registration\n",
                      httpMethodToString(method).c_str(), path.c_str());
        return;
      }

      Serial.printf("WebPlatform: Route %s %s already exists, replacing\n",
                    httpMethodToString(method).c_str(), path.c_str());
      route.handler = handler;
      route.disabled = false;
      return;
    }
  }

  // Add new route
  routeRegistry.push_back(RouteEntry(path, method, handler, auth, false));
}

void WebPlatform::overrideRoute(const String &path,
                                WebModule::UnifiedRouteHandler handler,
                                const AuthRequirements &auth,
                                WebModule::Method method) {
  // Find existing route and mark as overridden
  for (auto &route : routeRegistry) {
    if (route.path == path && route.method == method) {
      route.handler = handler;
      route.disabled = false;
      route.isOverride = true;

      Serial.printf("WebPlatform: Overrode existing route %s %s\n",
                    httpMethodToString(method).c_str(), path.c_str());
      return;
    }
  }

  // Create new override route that will take precedence over future
  // registrations
  routeRegistry.push_back(RouteEntry(path, method, handler, auth, true));

  Serial.printf("WebPlatform: Added preemptive override route %s %s\n",
                httpMethodToString(method).c_str(), path.c_str());
}

void WebPlatform::disableRoute(const String &path, WebModule::Method method) {
  // Find route and disable it
  for (auto &route : routeRegistry) {
    if (route.path == path && route.method == method) {
      route.disabled = true;

      Serial.printf("WebPlatform: Disabled route %s %s\n",
                    httpMethodToString(method).c_str(), path.c_str());
      return;
    }
  }

  // Create a disabled route entry to prevent future registration
  RouteEntry disabledRoute;
  disabledRoute.path = path;
  disabledRoute.method = method;
  disabledRoute.disabled = true;
  routeRegistry.push_back(disabledRoute);

  Serial.printf("WebPlatform: Pre-disabled route %s %s\n",
                httpMethodToString(method).c_str(), path.c_str());
}

// Helper function to check if route should be skipped (shared logic)
bool WebPlatform::shouldSkipRoute(const RouteEntry& route, const String& serverType) {
  if (route.disabled) {
    Serial.printf("WebPlatform: Skipping disabled %s route %s %s\n",
                  serverType.c_str(),
                  httpMethodToString(route.method).c_str(),
                  route.path.c_str());
    return true;
  }

  if (!route.handler) {
    Serial.printf("WebPlatform: Skipping %s route with null handler %s %s\n",
                  serverType.c_str(),
                  httpMethodToString(route.method).c_str(),
                  route.path.c_str());
    return true;
  }

  return false;
}

// Helper function to execute route with authentication and CSRF processing (shared logic)
void WebPlatform::executeRouteWithAuth(const RouteEntry& route, WebRequest& request, WebResponse& response, const String& serverType) {
  Serial.printf("--> %s handling request: %s\n",
                serverType.c_str(), request.getPath().c_str());

  // Check authentication requirements
  if (this->authenticateRequest(request, response, route.authRequirements)) {
    // Call the unified handler
    route.handler(request, response);

    // Process CSRF token injection for HTML responses
    if (!response.isResponseSent() &&
        response.getMimeType() == "text/html") {
      Serial.printf("Processing CSRF for %s %s response, content length: %d\n",
                    serverType.c_str(),
                    request.getPath().c_str(),
                    response.getContent().length());
      this->processCsrfForResponse(request, response);
    }
  }
}

// Internal method to register unified routes with actual server
void WebPlatform::registerUnifiedRoutes() {
  if (!server) {
    Serial.println("WebPlatform: No HTTP server to register unified routes on");
    return;
  }

  // Check if this is the HTTP redirect server (HTTPS mode)
  bool isRedirectServer =
      (httpsEnabled && serverPort == 443 && server != nullptr);
  bool isHttpRedirectServer = false;

#if defined(ESP8266)
  isHttpRedirectServer = isRedirectServer && serverPort == 80;
#elif defined(ESP32)
  isHttpRedirectServer = isRedirectServer;
#endif

  if (isHttpRedirectServer) {
    Serial.println("WebPlatform: Skipping unified route registration on HTTP "
                   "redirect server");
    return;
  }

  for (const auto &route : routeRegistry) {
    // Use shared route validation
    if (shouldSkipRoute(route, "HTTP")) {
      continue;
    }

    // Convert method to HTTP method
    HTTPMethod httpMethod = (route.method == WebModule::WM_GET)    ? HTTP_GET
                            : (route.method == WebModule::WM_POST) ? HTTP_POST
                            : (route.method == WebModule::WM_PUT)  ? HTTP_PUT
                            : (route.method == WebModule::WM_DELETE)
                                ? HTTP_DELETE
                                : HTTP_GET;

    // Create wrapper function that converts Arduino WebServer calls to unified handlers
    auto wrapperHandler = [route, this]() {
      WebRequest request(server);
      WebResponse response;

      // Use shared execution logic with authentication and CSRF
      executeRouteWithAuth(route, request, response, "HTTP");

      // Send the response
      response.sendTo(server);
    };

    // Register both with and without trailing slash
    server->on(route.path.c_str(), httpMethod, wrapperHandler);

    if (!route.path.endsWith("/") && route.path != "/") {
      String pathWithSlash = route.path + "/";
      server->on(pathWithSlash.c_str(), httpMethod, wrapperHandler);
    }
  }
}

// Internal method to register unified routes with HTTPS server
#if defined(ESP32)
void WebPlatform::registerUnifiedHttpsRoutes() {
  if (!httpsServerHandle || !httpsEnabled) {
    Serial.println(
        "WebPlatform: No HTTPS server to register unified routes on");
    return;
  }

  for (const auto &route : routeRegistry) {
    // Use shared route validation
    if (shouldSkipRoute(route, "HTTPS")) {
      continue;
    }

    // Convert method to ESP-IDF HTTP method
    httpd_method_t httpdMethod =
        (route.method == WebModule::WM_GET)      ? HTTP_GET
        : (route.method == WebModule::WM_POST)   ? HTTP_POST
        : (route.method == WebModule::WM_PUT)    ? HTTP_PUT
        : (route.method == WebModule::WM_DELETE) ? HTTP_DELETE
                                                 : HTTP_GET;

    // Store the path permanently for ESP-IDF
    httpsRoutePaths.push_back(route.path);
    String pathWithSlash = route.path + "/";
    if (!route.path.endsWith("/") && route.path != "/") {
      httpsRoutePaths.push_back(pathWithSlash);
    }

    // Register the route
    httpd_uri_t uri_config = {};
    uri_config.uri =
        httpsRoutePaths[httpsRoutePaths.size() -
                        (route.path.endsWith("/") || route.path == "/" ? 1 : 2)]
            .c_str();
    uri_config.method = httpdMethod;
    uri_config.handler = [](httpd_req_t *req) -> esp_err_t {
      // Find the route in our registry by comparing URI
      String requestUri(req->uri);

      Serial.printf("HTTPS handling request: %s\n", requestUri.c_str());
      for (const auto &route : routeRegistry) {
        bool pathMatches = (route.path == requestUri ||
                            (route.path + "/" == requestUri &&
                             !route.path.endsWith("/") && route.path != "/"));

        if (pathMatches && !route.disabled && route.handler) {
          Serial.printf("HTTPS found matching route: %s (override: %s)\n",
                        route.path.c_str(), route.isOverride ? "yes" : "no");

          WebRequest request(req);
          WebResponse response;

          // Use shared execution logic with authentication and CSRF
          WebPlatform::httpsInstance->executeRouteWithAuth(route, request, response, "HTTPS");

          return response.sendTo(req);
        }
      }

      Serial.printf("HTTPS no matching route found for: %s\n",
                    requestUri.c_str());
      return ESP_FAIL;
    };
    uri_config.user_ctx = nullptr;

    httpd_register_uri_handler(httpsServerHandle, &uri_config);

    // Register trailing slash version if different
    if (!route.path.endsWith("/") && route.path != "/") {
      uri_config.uri = httpsRoutePaths.back().c_str();
      httpd_register_uri_handler(httpsServerHandle, &uri_config);
    }
  }
}
#endif