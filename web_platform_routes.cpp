#include "route_entry.h"
#include "web_platform.h"
#include <web_module_interface.h>

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
  routeRegistry.push_back(RouteEntry(path, method, handler, false));

  Serial.printf("WebPlatform: Added new route %s %s\n",
                httpMethodToString(method).c_str(), path.c_str());
}

void WebPlatform::overrideRoute(const String &path,
                                WebModule::UnifiedRouteHandler handler,
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
  routeRegistry.push_back(RouteEntry(path, method, handler, true));

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

  // Dump route registry for debugging
  Serial.println("WebPlatform: Route registry state before registration:");
  for (const auto &route : routeRegistry) {
    Serial.printf("  %s %s (Override: %s, Disabled: %s)\n",
                  httpMethodToString(route.method).c_str(), route.path.c_str(),
                  route.isOverride ? "YES" : "no",
                  route.disabled ? "YES" : "no");
  }

  Serial.printf("WebPlatform: Registering %d unified routes with HTTP server\n",
                routeRegistry.size());

  for (const auto &route : routeRegistry) {
    if (route.disabled) {
      Serial.printf("WebPlatform: Skipping disabled route %s %s\n",
                    httpMethodToString(route.method).c_str(),
                    route.path.c_str());
      continue;
    }

    if (!route.handler) {
      Serial.printf("WebPlatform: Skipping route with null handler %s %s\n",
                    httpMethodToString(route.method).c_str(),
                    route.path.c_str());
      continue;
    }

    // Convert method to HTTP method
    HTTPMethod httpMethod = (route.method == WebModule::WM_GET)    ? HTTP_GET
                            : (route.method == WebModule::WM_POST) ? HTTP_POST
                            : (route.method == WebModule::WM_PUT)  ? HTTP_PUT
                            : (route.method == WebModule::WM_DELETE)
                                ? HTTP_DELETE
                                : HTTP_GET;

    // Create wrapper function that converts Arduino WebServer calls to unified
    // handlers
    auto wrapperHandler = [route, this]() {
      WebRequest request(server);
      WebResponse response;

      // Call the unified handler
      route.handler(request, response);

      // Send the response
      response.sendTo(server);
    };

    // Register both with and without trailing slash
    server->on(route.path.c_str(), httpMethod, wrapperHandler);

    if (!route.path.endsWith("/") && route.path != "/") {
      String pathWithSlash = route.path + "/";
      server->on(pathWithSlash.c_str(), httpMethod, wrapperHandler);
    }

    Serial.printf("WebPlatform: Registered unified route %s %s\n",
                  httpMethodToString(route.method).c_str(), route.path.c_str());
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

  // Add debugging for HTTPS route registration
  Serial.println(
      "WebPlatform: Route registry state before HTTPS registration:");
  for (const auto &route : routeRegistry) {
    Serial.printf("  %s %s (Override: %s, Disabled: %s)\n",
                  httpMethodToString(route.method).c_str(), route.path.c_str(),
                  route.isOverride ? "YES" : "no",
                  route.disabled ? "YES" : "no");
  }

  Serial.printf(
      "WebPlatform: Registering %d unified routes with HTTPS server\n",
      routeRegistry.size());

  for (const auto &route : routeRegistry) {
    if (route.disabled) {
      Serial.printf("WebPlatform: Skipping disabled HTTPS route %s %s\n",
                    httpMethodToString(route.method).c_str(),
                    route.path.c_str());
      continue;
    }

    if (!route.handler) {
      Serial.printf(
          "WebPlatform: Skipping HTTPS route with null handler %s %s\n",
          httpMethodToString(route.method).c_str(), route.path.c_str());
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

    // Create HTTPS route handler wrapper
    auto httpsWrapper = [route](httpd_req_t *req) -> esp_err_t {
      WebRequest request(req);
      WebResponse response;

      // Call the unified handler
      route.handler(request, response);

      // Send the response
      return response.sendTo(req);
    };

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

          route.handler(request, response);
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

    Serial.printf("WebPlatform: Registered unified HTTPS route %s %s\n",
                  httpMethodToString(route.method).c_str(), route.path.c_str());
  }
}
#endif

// Direct registration of module routes using the unified system
void WebPlatform::convertModuleRoutesToUnified() {
  Serial.println("\nWEBPLATFORM: Registering module routes directly to unified system");
  for (const auto &regModule : registeredModules) {
    Serial.printf("  Processing module: %s at path: %s\n",
                  regModule.module->getModuleName().c_str(),
                  regModule.basePath.c_str());

    // Register each module's root path to ensure it exists
    auto rootHandler = [regModule](WebRequest &req, WebResponse &res) {
      // This is just a placeholder - module should have registered a proper root handler
      // but we provide this as a fallback
      res.setContent("<h1>" + regModule.module->getModuleName() + "</h1><p>No index page provided by module.</p>",
                     "text/html");
    };

    String rootPath = regModule.basePath;
    if (!rootPath.endsWith("/")) {
      rootPath += "/";
    }

    // Only register this fallback if the module didn't register its own root handler
    bool hasRootHandler = false;
    for (const auto &route : routeRegistry) {
      if (route.path == rootPath && !route.disabled) {
        hasRootHandler = true;
        break;
      }
    }

    if (!hasRootHandler) {
      Serial.printf("  Adding fallback root handler for module at %s\n", rootPath.c_str());
      registerRoute(rootPath, rootHandler, WebModule::WM_GET);
    }
    
    // Process HTTP routes
    auto httpRoutes = regModule.module->getHttpRoutes();
    Serial.printf("  Module has %d HTTP routes\n", httpRoutes.size());

    for (const auto &route : httpRoutes) {
      Serial.printf("    Processing route: '%s' (method: %s)\n",
                    route.path.c_str(),
                    httpMethodToString(route.method).c_str());

      // Create full path
      String fullPath = regModule.basePath;

      // Special case for root path
      if (route.path == "/" || route.path.isEmpty()) {
        // For root path, ensure the base path ends with a slash
        if (!fullPath.endsWith("/")) {
          fullPath += "/";
        }
        Serial.printf("    Module root path at %s\n", fullPath.c_str());
      } else if (!fullPath.endsWith("/") && !route.path.startsWith("/")) {
        // Neither has slash, add one between
        fullPath += "/" + route.path;
        Serial.printf("    Added slash between paths: %s\n", fullPath.c_str());
      } else if (fullPath.endsWith("/") && route.path.startsWith("/")) {
        // Both have slash, remove duplicate
        fullPath += route.path.substring(1);
        Serial.printf("    Removed duplicate slash: %s\n", fullPath.c_str());
      } else {
        // One has slash, just concatenate
        fullPath += route.path;
        Serial.printf("    Standard path concatenation: %s\n", fullPath.c_str());
      }
      
      // Register route directly with unified system
      if (route.isUnified && route.unifiedHandler) {
        // Direct unified handler registration
        Serial.printf("    Registering unified route: %s %s\n",
                      httpMethodToString(route.method).c_str(),
                      fullPath.c_str());
        registerRoute(fullPath, route.unifiedHandler, route.method);
      } else if (route.handler) {
        // Convert to unified handler
        Serial.printf("    Converting to unified route: %s %s\n",
                      httpMethodToString(route.method).c_str(),
                      fullPath.c_str());
        auto unifiedHandler = [route, regModule](WebRequest &req, WebResponse &res) {
          std::map<String, String> params = req.getAllParams();
          String result = route.handler(req.getBody(), params);
          res.setContent(result, route.contentType);
        };

        registerRoute(fullPath, unifiedHandler, route.method);
      }
    }

    // Process HTTPS routes (similar logic)
    auto httpsRoutes = regModule.module->getHttpsRoutes();
    for (const auto &route : httpsRoutes) {
      String fullPath = regModule.basePath;

      // Special case for root path
      if (route.path == "/" || route.path.isEmpty()) {
        // For root path, ensure the base path ends with a slash
        if (!fullPath.endsWith("/")) {
          fullPath += "/";
        }
        Serial.printf("    Module HTTPS root path at %s\n", fullPath.c_str());
      } else if (!fullPath.endsWith("/") && !route.path.startsWith("/")) {
        // Neither has slash, add one between
        fullPath += "/" + route.path;
      } else if (fullPath.endsWith("/") && route.path.startsWith("/")) {
        // Both have slash, remove duplicate
        fullPath += route.path.substring(1);
      } else {
        // One has slash, just concatenate
        fullPath += route.path;
      }

      if (route.isUnified && route.unifiedHandler) {
        registerRoute(fullPath, route.unifiedHandler, route.method);
      } else if (route.handler) {
        auto unifiedHandler = [route, regModule](WebRequest &req, WebResponse &res) {
          std::map<String, String> params = req.getAllParams();
          String result = route.handler(req.getBody(), params);
          res.setContent(result, route.contentType);
        };

        registerRoute(fullPath, unifiedHandler, route.method);
      }
    }
  }
}