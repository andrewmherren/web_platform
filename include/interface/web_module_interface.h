#ifndef WEB_MODULE_INTERFACE_H
#define WEB_MODULE_INTERFACE_H

#include "../../assets/web_ui_styles.h"
#include "utilities/debug_macros.h"
#include "auth_types.h"
#include "openapi_factory.h"
#include "openapi_types.h"
#include "platform_service.h"
#include "utils/route_variant.h"
#include "web_module_types.h"
#include "web_request.h"
#include "web_response.h"
#include "webserver_typedefs.h"
#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>

// Use a completely different namespace to avoid conflicts with ESP32 built-in
// HTTP enums
namespace WebModule {

// Route handler function signature (legacy)
typedef std::function<String(const String &requestBody,
                             const std::map<String, String> &params)>
    RouteHandler;

// New unified route handler function signature
typedef std::function<void(WebRequest &, WebResponse &)> UnifiedRouteHandler;

} // namespace WebModule

// Forward declaration for variant
struct ApiRoute;

// Web route structure - supports both legacy and unified handlers
struct WebRoute {
  String path;                     // Route path (e.g., "/status", "/config")
  WebModule::Method method;        // HTTP method
  WebModule::RouteHandler handler; // Legacy function pointer (deprecated)
  WebModule::UnifiedRouteHandler unifiedHandler; // New unified handler
  String contentType; // Optional: "text/html", "application/json"
  String description; // Optional: Human-readable description
  AuthRequirements authRequirements; // Authentication requirements for route

private:
  // Helper function to check for API path usage warning
  static void checkApiPathWarning(const String &p) {
    if (p.startsWith("/api/") || p.startsWith("api/")) {
      WARN_PRINTLN(
          "WARNING: WebRoute path '" + p +
          "' starts with '/api/' or 'api/'. Consider using ApiRoute instead "
          "for better API documentation and path normalization.");
    }
  }

public:
  // Constructors for unified handlers
  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h)
      : path(p), method(m), unifiedHandler(h), contentType("text/html"),
        authRequirements({AuthType::NONE}) {
    checkApiPathWarning(p);
  }

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const String &ct)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        authRequirements({AuthType::NONE}) {
    checkApiPathWarning(p);
  }

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const String &ct,
           const String &desc)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        description(desc), authRequirements({AuthType::NONE}) {
    checkApiPathWarning(p);
  }

  // Constructors with auth requirements
  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth)
      : path(p), method(m), unifiedHandler(h), contentType("text/html"),
        authRequirements(auth) {
    checkApiPathWarning(p);
  }

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        authRequirements(auth) {
    checkApiPathWarning(p);
  }

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct, const String &desc)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        description(desc), authRequirements(auth) {
    checkApiPathWarning(p);
  }
};

struct ApiRoute {
  WebRoute webRoute; // Route details

  OpenAPIDocumentation docs; // OpenAPI documentation

private:
  // Helper function to normalize API paths by removing /api or api prefix
  static String normalizeApiPath(const String &path) {
    // If starts with /api/, remove the /api part
    if (path.startsWith("/api/")) {
      return path.substring(4); // Remove "/api" keeping the "/"
    }
    // If just "api", return "/"
    if (path.equals("api")) {
      return "/";
    }
    // Otherwise return as-is, ensuring it has a leading slash
    if (path.startsWith("/")) {
      return path;
    }
    return "/" + path;
  }

public:
  // Constructors for unified handlers
  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h)
      : webRoute(normalizeApiPath(p), m, h) {}

  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const String &ct)
      : webRoute(normalizeApiPath(p), m, h, ct) {}

  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const String &ct,
           const String &desc)
      : webRoute(normalizeApiPath(p), m, h, ct, desc) {}

  // Constructors with auth requirements
  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth)
      : webRoute(normalizeApiPath(p), m, h, auth) {}

  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct)
      : webRoute(normalizeApiPath(p), m, h, auth, ct) {}

  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct, const String &desc)
      : webRoute(normalizeApiPath(p), m, h, auth, ct, desc) {}

  // Constructors with OpenAPI documentation
  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h,
           const OpenAPIDocumentation &documentation)
      : webRoute(normalizeApiPath(p), m, h), docs(documentation) {}

  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const OpenAPIDocumentation &documentation)
      : webRoute(normalizeApiPath(p), m, h, auth), docs(documentation) {}

  ApiRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct, const OpenAPIDocumentation &documentation)
      : webRoute(normalizeApiPath(p), m, h, auth, ct), docs(documentation) {}
};

// Authentication visibility for navigation items
enum class NavAuthVisibility {
  ALWAYS,         // Always visible regardless of auth state
  AUTHENTICATED,  // Only visible when user has valid session
  UNAUTHENTICATED // Only visible when user is not authenticated
};

// Navigation menu item structure (optimized for PROGMEM)
struct NavigationItem {
  const char *name; // Display name for the menu item (PROGMEM pointer)
  const char *url;  // URL the menu item links to (PROGMEM pointer)
  const char
      *target; // Optional: target attribute for the link (PROGMEM pointer)
  NavAuthVisibility visibility; // When this item should be visible

  // Constructors for const char* (PROGMEM-friendly)
  NavigationItem(const char *n, const char *u)
      : name(n), url(u), target(""), visibility(NavAuthVisibility::ALWAYS) {}

  NavigationItem(const char *n, const char *u, const char *t)
      : name(n), url(u), target(t), visibility(NavAuthVisibility::ALWAYS) {}

  NavigationItem(const char *n, const char *u, NavAuthVisibility vis)
      : name(n), url(u), target(""), visibility(vis) {}

  NavigationItem(const char *n, const char *u, const char *t,
                 NavAuthVisibility vis)
      : name(n), url(u), target(t), visibility(vis) {}

  // Legacy String constructors (for backward compatibility - creates heap
  // allocations)
  NavigationItem(const String &n, const String &u);
  NavigationItem(const String &n, const String &u, const String &t);
  NavigationItem(const String &n, const String &u, NavAuthVisibility vis);
  NavigationItem(const String &n, const String &u, const String &t,
                 NavAuthVisibility vis);
};

// Convenience wrapper functions for cleaner syntax
inline NavigationItem Authenticated(const NavigationItem &item) {
  NavigationItem authItem = item;
  authItem.visibility = NavAuthVisibility::AUTHENTICATED;
  return authItem;
}

inline NavigationItem Unauthenticated(const NavigationItem &item) {
  NavigationItem unauthItem = item;
  unauthItem.visibility = NavAuthVisibility::UNAUTHENTICATED;
  return unauthItem;
}

// Redirect structure for managing URL redirects (simplified for embedded use)
struct RedirectRule {
  const char *fromPath; // Source path to redirect from (PROGMEM pointer)
  const char *toPath;   // Destination path to redirect to (PROGMEM pointer)

  // Constructor for const char* (PROGMEM-friendly)
  RedirectRule(const char *from, const char *to) : fromPath(from), toPath(to) {}

  // Legacy String constructor (for backward compatibility)
  RedirectRule(const String &from, const String &to);
};

// Abstract interface that all web modules must implement
class IWebModule {
private:
  static std::vector<NavigationItem> navigationMenu;
  static std::map<int, String> errorPages; // Custom error pages by status code
  static std::vector<RedirectRule> redirectRules; // URL redirect rules

public:
  virtual ~IWebModule() = default;

  // Required methods - pure virtual to enforce implementation
  virtual std::vector<RouteVariant> getHttpRoutes() = 0;
  virtual std::vector<RouteVariant> getHttpsRoutes() = 0;
  virtual String getModuleName() const = 0;

  // Optional implementations with defaults
  virtual String getModuleVersion() const { return "1.0.0"; }
  virtual String getModuleDescription() const { return "Web-enabled module"; }

  // Module lifecycle methods
  virtual void begin() {}
  virtual void begin(const JsonVariant &config) {
    begin();
  } // Default to parameterless begin()
  virtual void handle() {} // Called each loop iteration when in CONNECTED mode

  // Convenience method for modules with identical HTTP/HTTPS routes
  virtual std::vector<RouteVariant> getWebRoutes() { return getHttpRoutes(); }

  // Navigation menu management
  static void setNavigationMenu(const std::vector<NavigationItem> &items);
  static std::vector<NavigationItem> getNavigationMenu();

  // Helper methods for navigation menu
  static String generateNavigationHtml(bool isAuthenticated = false);

  // Custom error page management
  static void setErrorPage(int statusCode, const String &html);
  static String getErrorPage(int statusCode);

  // Helper method to generate default error pages with global CSS
  static String generateDefaultErrorPage(int statusCode,
                                         const String &message = "");

  // Add URL redirect rule (302 temporary redirect)
  static void addRedirect(const String &fromPath, const String &toPath);

  // Check if a path matches any redirect rule and return the redirect target
  // Returns empty string if no redirect matches
  static String getRedirectTarget(const String &requestPath);
};

#endif // WEB_MODULE_INTERFACE_H