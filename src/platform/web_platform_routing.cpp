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

void WebPlatform::clearRouteRegistry() {
  Serial.printf("WebPlatform: Clearing route registry (%d routes)\n", routeRegistry.size());
  routeRegistry.clear();
}

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
                      wmMethodToString(method).c_str(), path.c_str());
        return;
      }

      Serial.printf("WebPlatform: Route %s %s already exists, replacing\n",
                    wmMethodToString(method).c_str(), path.c_str());
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
                    wmMethodToString(method).c_str(), path.c_str());
      return;
    }
  }

  // Create new override route that will take precedence over future
  // registrations
  routeRegistry.push_back(RouteEntry(path, method, handler, auth, true));

  Serial.printf("WebPlatform: Added preemptive override route %s %s\n",
                wmMethodToString(method).c_str(), path.c_str());
}

void WebPlatform::disableRoute(const String &path, WebModule::Method method) {
  // Find route and disable it
  for (auto &route : routeRegistry) {
    if (route.path == path && route.method == method) {
      route.disabled = true;

      Serial.printf("WebPlatform: Disabled route %s %s\n",
                    wmMethodToString(method).c_str(), path.c_str());
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
                wmMethodToString(method).c_str(), path.c_str());
}

// Helper function to check if a path matches a route pattern with wildcards
bool WebPlatform::pathMatchesRoute(const String& routePath, const String& requestPath) {
  // Check for exact match first
  if (routePath == requestPath) {
    return true;
  }
  
  // Check for wildcard pattern
  int wildcardPos = routePath.indexOf('*');
  if (wildcardPos >= 0) {
    // Extract prefix before wildcard
    String prefix = routePath.substring(0, wildcardPos);
    
    // Check if request path starts with the prefix
    if (requestPath.startsWith(prefix)) {
      // For now, accept any suffix after the prefix
      // Could be enhanced to support more complex patterns
      return true;
    }
  }
  
  // TODO: couldn't we do simple regex capture match for {}?
  // Check for named parameter patterns like {param}
  int braceStart = routePath.indexOf('{');
  if (braceStart >= 0) {
    int braceEnd = routePath.indexOf('}', braceStart);
    if (braceEnd > braceStart) {
      // This is a simplified implementation
      // For full Laravel-style routing, this would need more complex parsing
      String beforeParam = routePath.substring(0, braceStart);
      String afterParam = routePath.substring(braceEnd + 1);
      
      if (requestPath.startsWith(beforeParam)) {
        if (afterParam.length() == 0) {
          // Parameter is at the end
          return requestPath.length() > beforeParam.length();
        } else {
          // Check if the part after the parameter matches
          int afterPos = requestPath.indexOf(afterParam, beforeParam.length());
          return afterPos > (int)beforeParam.length();
        }
      }
    }
  }
  
  return false;
}

// Helper function to check if route should be skipped (shared logic)
bool WebPlatform::shouldSkipRoute(const RouteEntry& route, const String& serverType) {
  if (route.disabled) {
    Serial.printf("WebPlatform: Skipping disabled %s route %s %s\n",
                  serverType.c_str(),
                  wmMethodToString(route.method).c_str(),
                  route.path.c_str());
    return true;
  }

  if (!route.handler) {
    Serial.printf("WebPlatform: Skipping %s route with null handler %s %s\n",
                  serverType.c_str(),
                  wmMethodToString(route.method).c_str(),
                  route.path.c_str());
    return true;
  }

  return false;
}

// Helper function to execute route with authentication and CSRF processing (shared logic)
void WebPlatform::executeRouteWithAuth(const RouteEntry& route, WebRequest& request, WebResponse& response, const String& serverType) {
  Serial.printf("%s handling request: %s\n",
                serverType.c_str(), request.getPath().c_str());

  // Set the matched route pattern on the request for parameter extraction
  request.setMatchedRoute(route.path);

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
      Serial.printf("Client IP from request.getClientIp(): %s\n", request.getClientIp().c_str());
      this->processCsrfForResponse(request, response);
    }
  }
}

// Internal method to register unified routes with actual server
void WebPlatform::bindRegisteredRoutes() {
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

    HTTPMethod httpMethod = wmMethodToHttpMethod(route.method);

    // Check if this route has wildcards or parameters - if so, use a generic handler
    bool hasWildcard = route.path.indexOf('*') >= 0 || route.path.indexOf('{') >= 0;
    
    if (hasWildcard) {
      // Create a generic handler that checks all requests for pattern matching
      auto wrapperHandler = [route, this]() {
        WebRequest request(server);
        String requestPath = request.getPath();
        
        // Check if this request matches our route pattern
        if (pathMatchesRoute(route.path, requestPath)) {
          WebResponse response;
          executeRouteWithAuth(route, request, response, "HTTP");
          response.sendTo(server);
        } else {
          // This shouldn't happen if routing is working correctly
          server->send(404, "text/plain", "Not Found");
        }
      };
      
      // For wildcard routes, we need to register a catch-all pattern
      // This is a limitation of Arduino WebServer - we'll register with the server's notFound handler
      // For now, we'll store these routes and handle them in a custom way
      
    } else {
      // Regular exact-match route
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
  
  // Set up a custom notFound handler that checks wildcard routes
  server->onNotFound([this]() {
    WebRequest request(server);
    String requestPath = request.getPath();
    HTTPMethod requestMethod = server->method();
    
    // Convert HTTP method back to our WebModule method
    WebModule::Method wmMethod = WebModule::WM_GET; // default
    if (requestMethod == HTTP_POST) wmMethod = WebModule::WM_POST;
    else if (requestMethod == HTTP_PUT) wmMethod = WebModule::WM_PUT;
    else if (requestMethod == HTTP_DELETE) wmMethod = WebModule::WM_DELETE;
    else if (requestMethod == HTTP_PATCH) wmMethod = WebModule::WM_PATCH;
    
    // Check all routes for wildcard matches
    for (const auto &route : routeRegistry) {
      if (route.disabled || !route.handler || route.method != wmMethod) {
        continue;
      }
      
      bool hasWildcard = route.path.indexOf('*') >= 0 || route.path.indexOf('{') >= 0;
      if (hasWildcard && this->pathMatchesRoute(route.path, requestPath)) {
        WebResponse response;
        this->executeRouteWithAuth(route, request, response, "HTTP");
        response.sendTo(server);
        return;
      }
    }
    
    // No match found
    server->send(404, "text/plain", "Not Found");
  });
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
    httpd_method_t httpdMethod = wmMethodToHttpMethod(route.method);

    // Check if this route has wildcards or parameters
    bool hasWildcard = route.path.indexOf('*') >= 0 || route.path.indexOf('{') >= 0;

    // For wildcard/parameterized routes, we need to handle them differently
    // ESP-IDF doesn't support wildcard matching, so we skip registration here
    // and handle them in a catch-all handler
    if (hasWildcard) {
      continue; // Skip registration, handle in catch-all
    }
    
    String registrationPath = route.path;

    // Store the path permanently for ESP-IDF
    httpsRoutePaths.push_back(registrationPath);
    String pathWithSlash = registrationPath + "/";
    if (!registrationPath.endsWith("/") && registrationPath != "/") {
      httpsRoutePaths.push_back(pathWithSlash);
    }

    // Register the route
    httpd_uri_t uri_config = {};
    uri_config.uri =
        httpsRoutePaths[httpsRoutePaths.size() -
                        (registrationPath.endsWith("/") || registrationPath == "/" ? 1 : 2)]
            .c_str();
    uri_config.method = httpdMethod;
    uri_config.handler = [](httpd_req_t *req) -> esp_err_t {
      // Create WebRequest first to get properly parsed path
      WebRequest request(req);
      WebResponse response;
      String requestPath = request.getPath();

      Serial.printf("HTTPS handling request: %s\n", req->uri);
      for (const auto &route : routeRegistry) {
        // Convert ESP-IDF method back to our WebModule method for comparison
        WebModule::Method wmMethod = WebModule::WM_GET; // default
        if (req->method == HTTP_POST) wmMethod = WebModule::WM_POST;
        else if (req->method == HTTP_PUT) wmMethod = WebModule::WM_PUT;
        else if (req->method == HTTP_DELETE) wmMethod = WebModule::WM_DELETE;
        else if (req->method == HTTP_PATCH) wmMethod = WebModule::WM_PATCH;
        
        if (route.method != wmMethod || route.disabled || !route.handler) {
          continue;
        }
        
        bool pathMatches = WebPlatform::httpsInstance->pathMatchesRoute(route.path, requestPath) ||
                          (route.path + "/" == requestPath &&
                           !route.path.endsWith("/") && route.path != "/");

        if (pathMatches && !route.disabled) {
          Serial.printf("HTTPS found matching route: %s matches %s (override: %s)\n",
                        route.path.c_str(), requestPath.c_str(), route.isOverride ? "yes" : "no");

          // Use shared execution logic with authentication and CSRF
          WebPlatform::httpsInstance->executeRouteWithAuth(route, request, response, "HTTPS");

          return response.sendTo(req);
        }
      }

      Serial.printf("HTTPS no matching route found for: %s (path: %s)\n",
                    req->uri, requestPath.c_str());
      return ESP_FAIL;
    };
    uri_config.user_ctx = nullptr;

    httpd_register_uri_handler(httpsServerHandle, &uri_config);

    // Register trailing slash version if different
    if (!registrationPath.endsWith("/") && registrationPath != "/") {
      uri_config.uri = httpsRoutePaths.back().c_str();
      httpd_register_uri_handler(httpsServerHandle, &uri_config);
    }
  }
  
  // No need for catch-all handlers - wildcard routes will be handled in the existing 404 error handler
}
#endif