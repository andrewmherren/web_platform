#ifndef ROUTE_ENTRY_H
#define ROUTE_ENTRY_H

#include <Arduino.h>
#include <web_module_interface.h>

// Complete RouteEntry definition
struct RouteEntry {
  String path;
  WebModule::Method method;
  WebModule::UnifiedRouteHandler handler;
  bool disabled;
  bool isOverride;

  RouteEntry()
      : method(WebModule::WM_GET), disabled(false), isOverride(false) {}

  RouteEntry(const String &p, WebModule::Method m,
             WebModule::UnifiedRouteHandler h, bool override = false)
      : path(p), method(m), handler(h), disabled(false), isOverride(override) {}
};

// Declare the global routeRegistry
extern std::vector<RouteEntry> routeRegistry;

#endif // ROUTE_ENTRY_H