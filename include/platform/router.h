#ifndef ROUTER_H
#define ROUTER_H

// Router owns route registration, path matching, and dispatch. It knows
// nothing about server lifecycle (sockets, certificates, HTTP vs HTTPS) -
// ServerManager owns that and hands Router a live WebServerClass*/
// httpd_handle_t to bind routes onto.
//
// Executing a matched route needs a few things Router doesn't own itself
// (auth policy, response template/CSRF processing, error pages, which
// registered module owns a given path) - those are injected as callbacks
// from WebPlatform at construction so Router takes explicit dependencies
// instead of reaching back into WebPlatform's private state.

#include "route_entry.h"
#include <functional>
#include <interface/auth_types.h>
#include <interface/openapi_types.h>
#include <interface/web_module_interface.h>
#include <interface/web_request.h>
#include <interface/web_response.h>
#include <vector>

#ifdef ESP_PLATFORM
#include <WebServer.h>
#include <esp_https_server.h>
#endif

class Router {
public:
  using AuthCallback = std::function<bool(
      WebRequest &, WebResponse &, const AuthRequirements &)>;
  using ShouldProcessResponseCallback =
      std::function<bool(const WebResponse &)>;
  using ProcessResponseTemplatesCallback =
      std::function<void(WebRequest &, WebResponse &)>;
  using ModuleBasePathResolver = std::function<String(const String &)>;
  using IsGeneratingDocsCallback = std::function<bool()>;
  using RouteDocumentedCallback =
      std::function<void(const String &, WebModule::Method,
                         const OpenAPIDocumentation &, const AuthRequirements &)>;
  using ErrorPageResolver = std::function<String(int)>;
  using HtmlPreparer = std::function<String(String, WebRequest)>;
  using RedirectResolver = std::function<String(const String &)>;

  struct Callbacks {
    AuthCallback authenticate;
    ShouldProcessResponseCallback shouldProcessResponse;
    ProcessResponseTemplatesCallback processResponseTemplates;
    ModuleBasePathResolver resolveModuleBasePath;
    IsGeneratingDocsCallback isGeneratingDocs;
    RouteDocumentedCallback onRouteDocumented;
    ErrorPageResolver getErrorPage;
    HtmlPreparer prepareHtml;
    RedirectResolver getRedirectTarget;
  };

  Router() = default;
#ifdef ESP_PLATFORM
  ~Router();
#endif

  // Must be called once, before begin(), with every callback populated.
  void setCallbacks(const Callbacks &cb) { callbacks = cb; }

  // Registration (mirrors the previous WebPlatform public/private surface)
  void registerWebRoute(const String &path,
                        WebModule::UnifiedRouteHandler handler,
                        const AuthRequirements &auth,
                        WebModule::Method method);
  void registerApiRoute(const String &path,
                        WebModule::UnifiedRouteHandler handler,
                        const AuthRequirements &auth,
                        WebModule::Method method,
                        const OpenAPIDocumentation &docs);
  void registerRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                     const AuthRequirements &auth, WebModule::Method method,
                     const OpenAPIDocumentation &docs);
  void disableRoute(const String &path, WebModule::Method method);

  size_t getEnabledRouteCount() const;
  size_t getTotalRouteCount() const { return routeRegistry.size(); }
  void printUnifiedRoutes() const;

  bool pathMatchesRoute(const char *routePath, const String &requestPath) const;

  // Matches and executes a route (any route, wildcard or not) against path.
  // Returns false if nothing matched.
  bool dispatchRoute(const String &path, WebModule::Method method,
                     WebRequest &request, WebResponse &response,
                     const char *protocol);

  // Like dispatchRoute, but only considers wildcard/parameterized routes -
  // used by 404 fallbacks that already tried an exact-path match elsewhere.
  bool dispatchWildcardOnly(const String &path, WebModule::Method method,
                            WebRequest &request, WebResponse &response,
                            const char *protocol);

#ifdef ESP_PLATFORM
  // Binds every currently-registered route onto a live Arduino WebServerClass
  // and installs the wildcard-aware 404 handler (falling back to
  // notFoundFallback when nothing - including wildcards - matches).
  void bindHttp(WebServerClass *server, bool isHttpRedirectServer,
               std::function<void()> notFoundFallback);

  // Binds every currently-registered route onto a live ESP-IDF HTTPS httpd
  // handle. Safe to call before any routes exist (used once at server
  // startup, before routes are registered, and again after routes are
  // finalized) - matches prior WebPlatform behavior.
  void bindHttpsRoutes(httpd_handle_t handle);

  // Registers the HTTPS 404 handler (wildcard match, then errorPage via
  // callbacks). Call once, right after the HTTPS server starts.
  void registerHttpsNotFoundHandler(httpd_handle_t handle);
#endif

private:
  bool shouldSkipRoute(const RouteEntry &route, const String &serverType) const;
  void executeRouteWithAuth(const RouteEntry &route, WebRequest &request,
                            WebResponse &response, const String &serverType);

  std::vector<RouteEntry> routeRegistry;
#ifdef ESP_PLATFORM
  std::vector<String> httpsRoutePaths; // stable c_str() storage for httpd_uri_t.uri
#endif
  Callbacks callbacks;

#ifdef ESP_PLATFORM
  // ESP-IDF httpd handlers are plain C function pointers (no capturing
  // lambdas), so the static trampolines below need a way back to a live
  // Router. Only one Router is ever constructed (a WebPlatform member), same
  // assumption the previous WebPlatform::httpsInstance pattern made.
  static Router *activeHttpsInstance;
#endif
};

#endif // ROUTER_H
