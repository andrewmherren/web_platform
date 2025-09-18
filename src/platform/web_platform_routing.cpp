#include "interface/web_module_interface.h"
#include "route_entry.h"
#include "route_string_pool.h"
#include "web_platform.h"

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
  Serial.printf("WebPlatform: Clearing route registry (%d routes)\n",
                routeRegistry.size());
  routeRegistry.clear();
}

void WebPlatform::registerWebRoute(const String &path,
                                   WebModule::UnifiedRouteHandler handler,
                                   const AuthRequirements &auth,
                                   WebModule::Method method) {
  // Check for API path usage warning
  if (path.startsWith("/api/") || path.startsWith("api/")) {
    Serial.println(
        "WARNING: registerWebRoute() path '" + path +
        "' starts with '/api/' or 'api/'. Consider using registerApiRoute() "
        "instead for better API documentation and path normalization.");
  }

  // Create default empty OpenAPIDocumentation and delegate to the documented
  // version
  OpenAPIDocumentation emptyDocs;
  registerRoute(path, handler, auth, method, emptyDocs);
}

void WebPlatform::registerApiRoute(const String &path,
                                   WebModule::UnifiedRouteHandler handler,
                                   const AuthRequirements &auth,
                                   WebModule::Method method,
                                   const OpenAPIDocumentation &docs) {
  // Ensure API routes always start with /api/
  String apiPath = path;

  // Remove leading slash if present for normalization
  if (apiPath.startsWith("/")) {
    apiPath = apiPath.substring(1);
  }

  // Remove api/ prefix if present (with or without leading slash)
  if (apiPath.startsWith("api/")) {
    apiPath = apiPath.substring(4);
  }

  String finalPath;
  if (apiPath.indexOf("/api/") == -1) {
    // Build final API path with /api/ prefix
    finalPath = "/api/";
    finalPath += apiPath;
  } else {
    // this is a module path already containing api which now looks like
    // module_prefix/api/some_path so we need to simply add the leading /
    finalPath = "/";
    finalPath += apiPath;
  }

  registerRoute(finalPath, handler, auth, method, docs);
}

void WebPlatform::registerRoute(const String &path,
                                WebModule::UnifiedRouteHandler handler,
                                const AuthRequirements &auth,
                                WebModule::Method method,
                                const OpenAPIDocumentation &docs) {
  // Convert path to const char* and store it
  const char *storedPath = RouteStringPool::store(path);

  // Check if route already exists
  for (auto &route : routeRegistry) {
    if (strcmp(route.path ? route.path : "", storedPath ? storedPath : "") ==
            0 &&
        route.method == method) {
      Serial.printf("WebPlatform: Route %s %s already exists, replacing\n",
                    wmMethodToString(method).c_str(),
                    storedPath ? storedPath : "null");
      route.handler = handler;
      route.authRequirements = auth;

      // Add OpenAPI documentation - store strings in pool
      route.summary = RouteStringPool::store(docs.summary);
      route.operationId = RouteStringPool::store(docs.operationId);
      route.description = RouteStringPool::store(docs.description);
      route.tags = RouteStringPool::store(docs.getTagsString());
      route.requestExample = RouteStringPool::store(docs.requestExample);
      route.responseExample = RouteStringPool::store(docs.responseExample);
      route.requestSchema = RouteStringPool::store(docs.requestSchema);
      route.responseSchema = RouteStringPool::store(docs.responseSchema);
      route.parameters = RouteStringPool::store(docs.parameters);
      route.responseInfo = RouteStringPool::store(docs.responsesJson);

      return;
    }
  }

  // Add new route
  RouteEntry newRoute(storedPath, method, handler, auth);

  // Add OpenAPI documentation - store strings in pool
  newRoute.summary = RouteStringPool::store(docs.summary);
  newRoute.operationId = RouteStringPool::store(docs.operationId);
  newRoute.description = RouteStringPool::store(docs.description);
  newRoute.tags = RouteStringPool::store(docs.getTagsString());
  newRoute.requestExample = RouteStringPool::store(docs.requestExample);
  newRoute.responseExample = RouteStringPool::store(docs.responseExample);
  newRoute.requestSchema = RouteStringPool::store(docs.requestSchema);
  newRoute.responseSchema = RouteStringPool::store(docs.responseSchema);
  newRoute.parameters = RouteStringPool::store(docs.parameters);
  newRoute.responseInfo = RouteStringPool::store(docs.responsesJson);

  routeRegistry.push_back(newRoute);
}

// Helper function to check if a path matches a route pattern with wildcards
// Helper function to check if a path matches a route pattern with wildcards
bool WebPlatform::pathMatchesRoute(const char *routePath,
                                   const String &requestPath) {
  // Check for exact match first
  if (routePath && requestPath == routePath) {
    return true;
  }

  // Convert to String for complex pattern matching
  String routePathStr = routePath ? String(routePath) : String("");

  // Simple pattern matching instead of regex (ESP32 doesn't fully support
  // std::regex)

  // First, handle simple wildcards
  if (routePathStr.endsWith("/*")) {
    // Check if path starts with the part before the wildcard
    String prefix = routePathStr.substring(0, routePathStr.length() - 1);
    return requestPath.startsWith(prefix);
  }

  // Handle parameter pattern matching {param}
  if (routePathStr.indexOf('{') < 0) {
    // No parameters, return false (already checked for exact match above)
    return false;
  }

  // Split both paths by '/'
  std::vector<String> routeSegments;
  std::vector<String> requestSegments;

  // Split route path
  int start = 0;
  int end = 0;
  while ((end = routePathStr.indexOf('/', start)) >= 0) {
    if (end > start) {
      routeSegments.push_back(routePathStr.substring(start, end));
    }
    start = end + 1;
  }
  if (start < (int)routePathStr.length()) {
    routeSegments.push_back(routePathStr.substring(start));
  }

  // Split request path
  start = 0;
  end = 0;
  while ((end = requestPath.indexOf('/', start)) >= 0) {
    if (end > start) {
      requestSegments.push_back(requestPath.substring(start, end));
    }
    start = end + 1;
  }
  if (start < (int)requestPath.length()) {
    requestSegments.push_back(requestPath.substring(start));
  }

  // If segment counts don't match, paths don't match
  if (routeSegments.size() != requestSegments.size()) {
    return false;
  }

  // Check each segment
  for (size_t i = 0; i < routeSegments.size(); i++) {
    String routeSegment = routeSegments[i];
    String requestSegment = requestSegments[i];

    // If it's a parameter segment {param}, consider it a match
    if (routeSegment.startsWith("{") && routeSegment.endsWith("}")) {
      // Parameter segment - validate it's a number or UUID
      bool validParam = false;

      // Check if it's a number
      bool isNumber = true;
      for (unsigned int j = 0; j < requestSegment.length(); j++) {
        if (!isdigit(requestSegment[j])) {
          isNumber = false;
          break;
        }
      }

      if (isNumber) {
        validParam = true;
      } else {
        // Check for UUID format (simplified check)
        if (requestSegment.length() == 36 && requestSegment.indexOf('-') == 8 &&
            requestSegment.indexOf('-', 9) == 13 &&
            requestSegment.indexOf('-', 14) == 18 &&
            requestSegment.indexOf('-', 19) == 23) {
          validParam = true;
        }
      }

      if (!validParam) {
        return false;
      }
    }
    // If not a parameter, segments must match exactly
    else if (routeSegment != requestSegment) {
      return false;
    }
  }

  // All segments matched
  return true;
}

// Helper function to execute route with authentication and CSRF processing
// (shared logic)
void WebPlatform::executeRouteWithAuth(const RouteEntry &route,
                                       WebRequest &request,
                                       WebResponse &response,
                                       const String &serverType) {
  Serial.printf("%s handling request: %s with route pattern: %s\n",
                serverType.c_str(), request.getPath().c_str(), route.path);

  // Set the matched route pattern on the request for parameter extraction
  request.setMatchedRoute(route.path);

  // Determine module base path from route path
  String requestPath = request.getPath();
  String moduleBasePath = "";

  // Find the registered module that handles this route
  for (const auto &regModule : registeredModules) {
    if (requestPath.startsWith(regModule.basePath)) {
      moduleBasePath = regModule.basePath;
      break;
    }
  }

  request.setModuleBasePath(moduleBasePath);

  // Check authentication requirements
  if (this->authenticateRequest(request, response, route.authRequirements)) {
    // Call the unified handler
    route.handler(request, response);

    // Process templates and CSRF token injection for responses that should be
    // processed
    if (!response.isResponseSent() && this->shouldProcessResponse(response)) {
      Serial.printf(
          "Processing templates for %s %s response, content length: %d\n",
          serverType.c_str(), request.getPath().c_str(),
          response.getContent().length());

      this->processResponseTemplates(request, response);
    }
  }
}

bool WebPlatform::dispatchRoute(const String &path, WebModule::Method wmMethod,
                                WebRequest &request, WebResponse &response,
                                const char *protocol) {
  for (const auto &route : routeRegistry) {
    if (!route.handler || route.method != wmMethod) {
      continue;
    }

    String routePathStr = route.path ? String(route.path) : String("");
    bool matches = pathMatchesRoute(route.path, path) ||
                   (!routePathStr.endsWith("/") && routePathStr + "/" == path);

    if (matches) {
      executeRouteWithAuth(route, request, response, protocol);
      return true; // handled
    }
  }

  return false; // not handled
}

bool WebPlatform::shouldSkipRoute(const RouteEntry &route,
                                  const String &serverType) {
  if (!route.handler) {
    Serial.printf("WebPlatform: Skipping %s route with null handler %s %s\n",
                  serverType.c_str(), wmMethodToString(route.method).c_str(),
                  route.path ? route.path : "<null>");
    return true;
  }

  return false;
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
    if (shouldSkipRoute(route, "HTTP")) {
      continue;
    }

    HTTPMethod httpMethod = wmMethodToHttpMethod(route.method);

    bool hasWildcard = String(route.path ? route.path : "").indexOf('*') >= 0 ||
                       String(route.path ? route.path : "").indexOf('{') >= 0;

    if (hasWildcard) {
      // will be handled in 404 handler.
      continue;
    }

    // Special case: root
    if (route.path && strcmp(route.path, "/") == 0) {
      server->on(route.path, httpMethod, [this, route]() {
        WebRequest request(server);
        WebResponse response;

        executeRouteWithAuth(route, request, response, "HTTP");
        response.sendTo(server);
      });
      continue;
    }

    // Determine if the route looks like a "file" (has extension)
    bool looksLikeFile = String(route.path ? route.path : "").lastIndexOf('.') >
                         String(route.path ? route.path : "").lastIndexOf('/');

    // Detect REST API routes
    bool isApiRoute =
        String(route.path ? route.path : "").indexOf("/api/") != -1;

    String routeWithSlash = route.path ? String(route.path) : String("");
    // Only apply trailing-slash redirect for page-like routes that are not
    // API
    if (!looksLikeFile && !isApiRoute && !routeWithSlash.endsWith("/")) {
      routeWithSlash += "/";
    }

    String routeNoSlash =
        looksLikeFile || isApiRoute
            ? route.path // don’t alter file routes
            : routeWithSlash.substring(0, routeWithSlash.length() - 1);

    // Real handler (slash form)
    auto wrapperHandler = [this, route]() {
      WebRequest request(server);
      WebResponse response;

      executeRouteWithAuth(route, request, response, "HTTP");
      response.sendTo(server);
    };
    server->on(routeWithSlash.c_str(), httpMethod, wrapperHandler);

    // Redirect from no-slash to slash, only for directory-like routes
    if (!looksLikeFile && !isApiRoute &&
        (route.path == nullptr || strcmp(route.path, "/") != 0)) {
      server->on(routeNoSlash.c_str(), httpMethod, [routeWithSlash, this]() {
        server->sendHeader("Location", routeWithSlash, true);
        server->send(301);
      });
    }
  }

  // Set up a custom notFound handler that checks wildcard routes
  server->onNotFound([this]() {
    WebRequest request(server);
    String requestPath = request.getPath();
    HTTPMethod requestMethod = server->method();

    // Convert HTTP method back to our WebModule method
    WebModule::Method wmMethod = WebModule::WM_GET; // default
    if (requestMethod == HTTP_POST)
      wmMethod = WebModule::WM_POST;
    else if (requestMethod == HTTP_PUT)
      wmMethod = WebModule::WM_PUT;
    else if (requestMethod == HTTP_DELETE)
      wmMethod = WebModule::WM_DELETE;
    else if (requestMethod == HTTP_PATCH)
      wmMethod = WebModule::WM_PATCH;

    // Check all routes for wildcard matches
    for (const auto &route : routeRegistry) {
      if (!route.handler || route.method != wmMethod) {
        continue;
      }

      bool hasWildcard =
          String(route.path ? route.path : "").indexOf('*') >= 0 ||
          String(route.path ? route.path : "").indexOf('{') >= 0;
      if (hasWildcard && this->pathMatchesRoute(route.path, requestPath)) {
        WebResponse response;
        this->executeRouteWithAuth(route, request, response, "HTTP");
        response.sendTo(server);
        return;
      }
    }

    // Fall back to original handleNotFound logic
    this->handleNotFound();
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
    if (shouldSkipRoute(route, "HTTPS")) {
      continue;
    }

    httpd_method_t httpdMethod = wmMethodToHttpMethod(route.method);

    bool hasWildcard = String(route.path ? route.path : "").indexOf('*') >= 0 ||
                       String(route.path ? route.path : "").indexOf('{') >= 0;

    if (hasWildcard) {
      continue; // handled by catch-all
    }

    String registrationPath = route.path;

    // === Special case: root ("/") ===
    if (registrationPath == "/") {
      httpsRoutePaths.push_back(registrationPath);

      httpd_uri_t uri_config = {};
      uri_config.uri = httpsRoutePaths.back().c_str();
      uri_config.method = httpdMethod;
      uri_config.user_ctx = nullptr;
      uri_config.handler = [](httpd_req_t *req) -> esp_err_t {
        WebRequest request(req);
        WebResponse response;

        String requestPath = request.getPath();
        WebModule::Method wmMethod =
            httpMethodToWMMethod((HTTPMethod)req->method);

        if (!WebPlatform::httpsInstance->dispatchRoute(
                requestPath, wmMethod, request, response, "HTTPS")) {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
        return response.sendTo(req);
      };
      httpd_register_uri_handler(httpsServerHandle, &uri_config);
      continue;
    }

    // Ensure trailing slash form only for "directory-like" routes
    String routeWithSlash = registrationPath;
    bool looksLikeFile =
        registrationPath.lastIndexOf('.') > registrationPath.lastIndexOf('/');

    // Detect REST API routes
    bool isApiRoute =
        String(route.path ? route.path : "").indexOf("/api/") != -1;

    if (!looksLikeFile && !isApiRoute && !routeWithSlash.endsWith("/")) {
      routeWithSlash += "/";
    }

    // No-slash form
    String routeNoSlash =
        looksLikeFile || isApiRoute
            ? registrationPath // don’t alter file routes
            : routeWithSlash.substring(0, routeWithSlash.length() - 1);

    // === Slash form: real handler ===
    httpsRoutePaths.push_back(routeWithSlash);
    httpd_uri_t uri_config = {};
    uri_config.uri = httpsRoutePaths.back().c_str();
    uri_config.method = httpdMethod;
    uri_config.handler = [](httpd_req_t *req) -> esp_err_t {
      WebRequest request(req);
      WebResponse response;
      String requestPath = request.getPath();

      WebModule::Method wmMethod =
          httpMethodToWMMethod((HTTPMethod)req->method);

      if (!WebPlatform::httpsInstance->dispatchRoute(
              requestPath, wmMethod, request, response, "HTTPS")) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }

      return response.sendTo(req);
    };
    httpd_register_uri_handler(httpsServerHandle, &uri_config);

    // === No-slash redirect ===
    if (!looksLikeFile && !isApiRoute && registrationPath != "/") {
      // note: if we ever allow dynamic route addition/removal or
      // teardown and recreation of the server this String could create
      // a memory leak. It would need to be freed in these conditions
      String *targetCopy = new String(routeWithSlash);
      httpd_uri_t redirect_config = {};
      redirect_config.uri = routeNoSlash.c_str();
      redirect_config.method = httpdMethod;
      redirect_config.user_ctx = targetCopy;
      redirect_config.handler = [](httpd_req_t *req) -> esp_err_t {
        const String *target = static_cast<const String *>(req->user_ctx);
        httpd_resp_set_status(req, "301 Moved Permanently");
        httpd_resp_set_hdr(req, "Location", target->c_str());
        httpd_resp_sendstr(req, "Redirecting...");
        return ESP_OK;
      };
      httpd_register_uri_handler(httpsServerHandle, &redirect_config);
    }
  }
}
#endif