#ifndef NAVIGATION_TYPES_H
#define NAVIGATION_TYPES_H

#include <Arduino.h>

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

#endif // NAVIGATION_TYPES_H