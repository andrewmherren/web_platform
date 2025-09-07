#ifndef WEB_MODULE_INTERFACE_H
#define WEB_MODULE_INTERFACE_H

#include "../../assets/web_ui_styles.h"
#include "auth_types.h"
#include "platform_service.h"
#include "web_module_types.h"
#include "web_request.h"
#include "web_response.h"
#include "webserver_typedefs.h"
#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>

// Use a completely different namespace to avoid conflicts with ESP32/ESP8266
// built-in HTTP enums
namespace WebModule {

// Route handler function signature (legacy)
typedef std::function<String(const String &requestBody,
                             const std::map<String, String> &params)>
    RouteHandler;

// New unified route handler function signature
typedef std::function<void(WebRequest &, WebResponse &)> UnifiedRouteHandler;

} // namespace WebModule

// Web route structure - supports both legacy and unified handlers
struct WebRoute {
  String path;                     // Route path (e.g., "/status", "/config")
  WebModule::Method method;        // HTTP method
  WebModule::RouteHandler handler; // Legacy function pointer (deprecated)
  WebModule::UnifiedRouteHandler unifiedHandler; // New unified handler
  String contentType; // Optional: "text/html", "application/json"
  String description; // Optional: Human-readable description
  AuthRequirements authRequirements; // Authentication requirements for route

  // Constructors for unified handlers
  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h)
      : path(p), method(m), unifiedHandler(h), contentType("text/html"),
        authRequirements({AuthType::NONE}) {}

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const String &ct)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        authRequirements({AuthType::NONE}) {}

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const String &ct,
           const String &desc)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        description(desc), authRequirements({AuthType::NONE}) {}

  // Constructors with auth requirements
  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth)
      : path(p), method(m), unifiedHandler(h), contentType("text/html"),
        authRequirements(auth) {}

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        authRequirements(auth) {}

  WebRoute(const String &p, WebModule::Method m,
           WebModule::UnifiedRouteHandler h, const AuthRequirements &auth,
           const String &ct, const String &desc)
      : path(p), method(m), unifiedHandler(h), contentType(ct),
        description(desc), authRequirements(auth) {}
};

// Authentication visibility for navigation items
enum class NavAuthVisibility {
  ALWAYS,         // Always visible regardless of auth state
  AUTHENTICATED,  // Only visible when user has valid session
  UNAUTHENTICATED // Only visible when user is not authenticated
};

// Navigation menu item structure
struct NavigationItem {
  String name;   // Display name for the menu item
  String url;    // URL the menu item links to
  String target; // Optional: target attribute for the link (e.g., "_blank")
  NavAuthVisibility visibility; // When this item should be visible

  // Constructors for convenience
  NavigationItem(const String &n, const String &u)
      : name(n), url(u), target(""), visibility(NavAuthVisibility::ALWAYS) {}

  NavigationItem(const String &n, const String &u, const String &t)
      : name(n), url(u), target(t), visibility(NavAuthVisibility::ALWAYS) {}

  NavigationItem(const String &n, const String &u, NavAuthVisibility vis)
      : name(n), url(u), target(""), visibility(vis) {}

  NavigationItem(const String &n, const String &u, const String &t,
                 NavAuthVisibility vis)
      : name(n), url(u), target(t), visibility(vis) {}
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
  String fromPath; // Source path to redirect from
  String toPath;   // Destination path to redirect to

  // Constructor for convenience
  RedirectRule(const String &from, const String &to)
      : fromPath(from), toPath(to) {}
};

// Abstract interface that all web modules must implement
class IWebModule {
private:
  static std::vector<NavigationItem> navigationMenu;
  static String
      currentPath; // Store the current request path for auto-active detection
  static std::map<int, String> errorPages; // Custom error pages by status code
  static std::vector<RedirectRule> redirectRules; // URL redirect rules

public:
  virtual ~IWebModule() = default;

  // Required methods - pure virtual to enforce implementation
  virtual std::vector<WebRoute> getHttpRoutes() = 0;
  virtual std::vector<WebRoute> getHttpsRoutes() = 0;
  virtual String getModuleName() const = 0;

  // Optional methods with default implementations
  virtual String getModuleVersion() const { return "1.0.0"; }
  virtual String getModuleDescription() const { return "Web-enabled module"; }

  // Convenience method for modules with identical HTTP/HTTPS routes
  virtual std::vector<WebRoute> getWebRoutes() { return getHttpRoutes(); }

  // Phase 2: Navigation Menu System
  // Navigation menu management
  static void setNavigationMenu(const std::vector<NavigationItem> &items);
  static std::vector<NavigationItem> getNavigationMenu();

  // Set current path for auto-active detection in navigation
  static void setCurrentPath(const String &path);
  static String getCurrentPath();

  // Helper methods for navigation menu
  static String generateNavigationHtml(bool isAuthenticated = false);

  // Custom error page management
  static void setErrorPage(int statusCode, const String &html);
  static String getErrorPage(int statusCode);

  // Helper method to generate default error pages with global CSS
  static String generateDefaultErrorPage(int statusCode,
                                         const String &message = "");

  // Phase 3: Route Redirection System (simplified for embedded use)
  // Add URL redirect rule (302 temporary redirect)
  static void addRedirect(const String &fromPath, const String &toPath);

  // Check if a path matches any redirect rule and return the redirect target
  // Returns empty string if no redirect matches
  static String getRedirectTarget(const String &requestPath);
};

#endif // WEB_MODULE_INTERFACE_H