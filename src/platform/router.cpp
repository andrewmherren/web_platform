#include "platform/router.h"
#include "platform/route_string_pool.h"
#include "utilities/debug_macros.h"
#include <interface/web_module_interface.h>

#ifdef ESP_PLATFORM
Router *Router::activeHttpsInstance = nullptr;

Router::~Router() {
  if (activeHttpsInstance == this) {
    activeHttpsInstance = nullptr;
  }
}
#endif

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void Router::registerWebRoute(const String &path,
                              WebModule::UnifiedRouteHandler handler,
                              const AuthRequirements &auth,
                              WebModule::Method method) {
  if (path.startsWith("/api/") || path.startsWith("api/")) {
    WARN_PRINTLN(
        "WARNING: registerWebRoute() path '" + path +
        "' starts with '/api/' or 'api/'. Consider using registerApiRoute() "
        "instead for better API documentation and path normalization.");
  }

  OpenAPIDocumentation emptyDocs;
  registerRoute(path, handler, auth, method, emptyDocs);
}

void Router::registerApiRoute(const String &path,
                              WebModule::UnifiedRouteHandler handler,
                              const AuthRequirements &auth,
                              WebModule::Method method,
                              const OpenAPIDocumentation &docs) {
  String apiPath = path;

  if (apiPath.startsWith("/")) {
    apiPath = apiPath.substring(1);
  }

  if (apiPath.startsWith("api/")) {
    apiPath = apiPath.substring(4);
  }

  String finalPath;
  if (apiPath.indexOf("/api/") == -1) {
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

void Router::registerRoute(const String &path,
                           WebModule::UnifiedRouteHandler handler,
                           const AuthRequirements &auth,
                           WebModule::Method method,
                           const OpenAPIDocumentation &docs) {
  const char *storedPath = RouteStringPool::store(path);

  for (auto &route : routeRegistry) {
    if (strcmp(route.path ? route.path : "", storedPath ? storedPath : "") ==
            0 &&
        route.method == method) {
      DEBUG_PRINTF("Router: Route %s %s already exists, replacing\n",
                   wmMethodToString(method).c_str(),
                   storedPath ? storedPath : "null");
      route.handler = handler;
      route.authRequirements = auth;

      if (callbacks.isGeneratingDocs && callbacks.isGeneratingDocs()) {
        DEBUG_PRINTF("Router: Collecting docs during generation for "
                     "replacement route: %s %s\n",
                     wmMethodToString(method).c_str(), path.c_str());
        if (callbacks.onRouteDocumented) {
          callbacks.onRouteDocumented(path, method, docs, auth);
        }
      }
      return;
    }
  }

  RouteEntry newRoute(storedPath, method, handler, auth);
  routeRegistry.push_back(newRoute);

  if (callbacks.isGeneratingDocs && callbacks.isGeneratingDocs()) {
    if (callbacks.onRouteDocumented) {
      callbacks.onRouteDocumented(path, method, docs, auth);
    }
  }
}

void Router::disableRoute(const String &path, WebModule::Method method) {
  for (auto &route : routeRegistry) {
    if (route.path && strcmp(route.path, path.c_str()) == 0 &&
        route.method == method) {
      DEBUG_PRINTF("Router: Disabling route %s %s\n",
                   wmMethodToString(method).c_str(), path.c_str());
      route.handler = nullptr; // shouldSkipRoute() will treat this as absent
      return;
    }
  }
  DEBUG_PRINTF("Router: Route %s %s not found for disabling\n",
               wmMethodToString(method).c_str(), path.c_str());
}

// ---------------------------------------------------------------------------
// Matching / dispatch
// ---------------------------------------------------------------------------

bool Router::pathMatchesRoute(const char *routePath,
                              const String &requestPath) const {
  if (routePath && requestPath == routePath) {
    return true;
  }

  String routePathStr = routePath ? String(routePath) : String("");

  // Simple pattern matching instead of regex (ESP32 doesn't fully support
  // std::regex)
  if (routePathStr.endsWith("/*")) {
    String prefix = routePathStr.substring(0, routePathStr.length() - 1);
    return requestPath.startsWith(prefix);
  }

  if (routePathStr.indexOf('{') < 0) {
    return false;
  }

  std::vector<String> routeSegments;
  std::vector<String> requestSegments;

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

  if (routeSegments.size() != requestSegments.size()) {
    return false;
  }

  for (size_t i = 0; i < routeSegments.size(); i++) {
    String routeSegment = routeSegments[i];
    String requestSegment = requestSegments[i];

    if (routeSegment.startsWith("{") && routeSegment.endsWith("}")) {
      bool validParam = false;

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
    } else if (routeSegment != requestSegment) {
      return false;
    }
  }

  return true;
}

bool Router::shouldSkipRoute(const RouteEntry &route,
                             const String &serverType) const {
  if (!route.handler) {
    DEBUG_PRINTF("Router: Skipping %s route with null handler %s %s\n",
                 serverType.c_str(), wmMethodToString(route.method).c_str(),
                 route.path ? route.path : "<null>");
    return true;
  }
  return false;
}

void Router::executeRouteWithAuth(const RouteEntry &route, WebRequest &request,
                                  WebResponse &response,
                                  const String &serverType) {
  DEBUG_PRINTF("%s handling request: %s with route pattern: %s\n",
               serverType.c_str(), request.getPath().c_str(), route.path);

  request.setMatchedRoute(route.path);

  String moduleBasePath;
  if (callbacks.resolveModuleBasePath) {
    moduleBasePath = callbacks.resolveModuleBasePath(request.getPath());
  }
  request.setModuleBasePath(moduleBasePath);

  if (callbacks.authenticate(request, response, route.authRequirements)) {
    route.handler(request, response);

    if (!response.isResponseSent() &&
        (!callbacks.shouldProcessResponse ||
         callbacks.shouldProcessResponse(response))) {
      DEBUG_PRINTF(
          "Processing templates for %s %s response, content length: %d\n",
          serverType.c_str(), request.getPath().c_str(),
          response.getContent().length());

      if (callbacks.processResponseTemplates) {
        callbacks.processResponseTemplates(request, response);
      }
    }
  }
}

bool Router::dispatchRoute(const String &path, WebModule::Method wmMethod,
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
      return true;
    }
  }

  return false;
}

bool Router::dispatchWildcardOnly(const String &path, WebModule::Method wmMethod,
                                  WebRequest &request, WebResponse &response,
                                  const char *protocol) {
  for (const auto &route : routeRegistry) {
    if (!route.handler || route.method != wmMethod) {
      continue;
    }

    String routePathStr = route.path ? String(route.path) : String("");
    bool hasWildcard =
        routePathStr.indexOf('*') >= 0 || routePathStr.indexOf('{') >= 0;
    if (!hasWildcard) {
      continue;
    }

    if (pathMatchesRoute(route.path, path)) {
      executeRouteWithAuth(route, request, response, protocol);
      return true;
    }
  }

  return false;
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

size_t Router::getEnabledRouteCount() const {
  size_t enabledCount = 0;
  for (const auto &route : routeRegistry) {
    if (route.handler) {
      enabledCount++;
    }
  }
  return enabledCount;
}

void Router::printUnifiedRoutes() const {
  DEBUG_PRINTF("\n=== WebPlatform Route Registry ===\n");
  DEBUG_PRINTLN("PATH                        METHOD  AUTH");
  DEBUG_PRINTLN("--------------------------- ------- -------------");

  for (const auto &route : routeRegistry) {
    String pathStr = route.path;
    if (pathStr.length() > 27) {
      pathStr = pathStr.substring(0, 24) + "...";
    } else {
      while (pathStr.length() < 27) {
        pathStr += " ";
      }
    }

    String methodStr = wmMethodToString(route.method);
    while (methodStr.length() < 7) {
      methodStr += " ";
    }

    String authStr = "";
    if (route.authRequirements.empty() ||
        (route.authRequirements.size() == 1 &&
         route.authRequirements[0] == AuthType::NONE)) {
      authStr = "NONE";
    } else {
      bool first = true;
      for (const auto &auth : route.authRequirements) {
        if (!first)
          authStr += "|";
        first = false;
        authStr += AuthUtils::authTypeToString(auth);
      }
    }
    while (authStr.length() < 12) {
      authStr += " ";
    }

    DEBUG_PRINTF("%s %s %s\n", pathStr.c_str(), methodStr.c_str(),
                 authStr.c_str());
  }

  DEBUG_PRINTLN("========================================================");
  DEBUG_PRINTF("Total routes: %d\n\n", routeRegistry.size());
}

// ---------------------------------------------------------------------------
// Server binding (ESP32 only)
// ---------------------------------------------------------------------------

#ifdef ESP_PLATFORM

void Router::bindHttp(WebServerClass *server, bool isHttpRedirectServer,
                      std::function<void()> notFoundFallback) {
  if (!server) {
    DEBUG_PRINTLN("Router: No HTTP server to register unified routes on");
    return;
  }

  if (isHttpRedirectServer) {
    DEBUG_PRINTLN("Router: Skipping unified route registration on HTTP "
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
      // handled by the wildcard-aware 404 handler below
      continue;
    }

    if (route.path && strcmp(route.path, "/") == 0) {
      server->on(route.path, httpMethod, [this, server, route]() {
        WebRequest request(server);
        WebResponse response;

        executeRouteWithAuth(route, request, response, "HTTP");
        response.sendTo(server);
      });
      continue;
    }

    bool looksLikeFile = String(route.path ? route.path : "").lastIndexOf('.') >
                         String(route.path ? route.path : "").lastIndexOf('/');

    bool isApiRoute =
        String(route.path ? route.path : "").indexOf("/api/") != -1;

    String routeWithSlash = route.path ? String(route.path) : String("");
    if (!looksLikeFile && !isApiRoute && !routeWithSlash.endsWith("/")) {
      routeWithSlash += "/";
    }

    String routeNoSlash =
        looksLikeFile || isApiRoute
            ? route.path // don't alter file routes
            : routeWithSlash.substring(0, routeWithSlash.length() - 1);

    auto wrapperHandler = [this, server, route]() {
      WebRequest request(server);
      WebResponse response;

      executeRouteWithAuth(route, request, response, "HTTP");
      response.sendTo(server);
    };
    server->on(routeWithSlash.c_str(), httpMethod, wrapperHandler);

    if (!looksLikeFile && !isApiRoute &&
        (route.path == nullptr || strcmp(route.path, "/") != 0)) {
      server->on(routeNoSlash.c_str(), httpMethod, [routeWithSlash, server]() {
        server->sendHeader("Location", routeWithSlash, true);
        server->send(301);
      });
    }
  }

  server->onNotFound([this, server, notFoundFallback]() {
    WebRequest request(server);
    String requestPath = request.getPath();
    WebModule::Method wmMethod = httpMethodToWMMethod(server->method());

    WebResponse response;
    if (dispatchWildcardOnly(requestPath, wmMethod, request, response,
                             "HTTP")) {
      response.sendTo(server);
      return;
    }

    if (notFoundFallback) {
      notFoundFallback();
    }
  });
}

void Router::bindHttpsRoutes(httpd_handle_t handle) {
  if (!handle) {
    DEBUG_PRINTLN("Router: No HTTPS server to register unified routes on");
    return;
  }

  activeHttpsInstance = this;
  httpsRoutePaths.clear();

  for (const auto &route : routeRegistry) {
    if (shouldSkipRoute(route, "HTTPS")) {
      continue;
    }

    httpd_method_t httpdMethod = wmMethodToHttpMethod(route.method);

    bool hasWildcard = String(route.path ? route.path : "").indexOf('*') >= 0 ||
                       String(route.path ? route.path : "").indexOf('{') >= 0;

    if (hasWildcard) {
      continue; // handled by the wildcard-aware 404 handler
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

        if (!Router::activeHttpsInstance->dispatchRoute(
                requestPath, wmMethod, request, response, "HTTPS")) {
          httpd_resp_send_404(req);
          return ESP_FAIL;
        }
        return response.sendTo(req);
      };
      httpd_register_uri_handler(handle, &uri_config);
      continue;
    }

    String routeWithSlash = registrationPath;
    bool looksLikeFile =
        registrationPath.lastIndexOf('.') > registrationPath.lastIndexOf('/');

    bool isApiRoute =
        String(route.path ? route.path : "").indexOf("/api/") != -1;

    if (!looksLikeFile && !isApiRoute && !routeWithSlash.endsWith("/")) {
      routeWithSlash += "/";
    }

    String routeNoSlash =
        looksLikeFile || isApiRoute
            ? registrationPath // don't alter file routes
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

      if (!Router::activeHttpsInstance->dispatchRoute(
              requestPath, wmMethod, request, response, "HTTPS")) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }

      return response.sendTo(req);
    };
    httpd_register_uri_handler(handle, &uri_config);

    // === No-slash redirect ===
    if (!looksLikeFile && !isApiRoute && registrationPath != "/") {
      // note: if we ever allow dynamic route addition/removal or teardown
      // and recreation of the server this String could create a memory
      // leak. It would need to be freed in these conditions
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
      httpd_register_uri_handler(handle, &redirect_config);
    }
  }
}

void Router::registerHttpsNotFoundHandler(httpd_handle_t handle) {
  activeHttpsInstance = this;

  httpd_register_err_handler(
      handle, HTTPD_404_NOT_FOUND,
      [](httpd_req_t *req, httpd_err_code_t err) -> esp_err_t {
        WebRequest request(req);
        String requestPath = request.getPath();

        WebModule::Method wmMethod =
            httpMethodToWMMethod((HTTPMethod)req->method);

        WebResponse response;
        if (Router::activeHttpsInstance->dispatchWildcardOnly(
                requestPath, wmMethod, request, response, "HTTPS")) {
          return response.sendTo(req);
        }

        String errorPage =
            Router::activeHttpsInstance->callbacks.getErrorPage
                ? Router::activeHttpsInstance->callbacks.getErrorPage(404)
                : String();
        if (errorPage.length() > 0) {
          String processedErrorPage =
              Router::activeHttpsInstance->callbacks.prepareHtml
                  ? Router::activeHttpsInstance->callbacks.prepareHtml(
                        errorPage, request)
                  : errorPage;

          WebResponse errorResponse;
          errorResponse.setStatus(404);
          errorResponse.setContent(processedErrorPage, "text/html");
          return errorResponse.sendTo(req);
        }

        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_send(req, "Page Not Found", 14);
        return ESP_OK;
      });
  DEBUG_PRINTLN("Registered 404 error handler");
}

#endif // ESP_PLATFORM
