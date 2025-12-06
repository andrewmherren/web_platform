#ifndef ROUTE_ENTRY_H
#define ROUTE_ENTRY_H
#include "platform/route_string_pool.h"
#include <Arduino.h>
#include <interface/auth_types.h>
#include <interface/web_module_interface.h>


// Complete RouteEntry definition - optimized for memory efficiency
// OpenAPI fields removed - documentation now handled by temporary storage
// during generation
struct RouteEntry {
  const char *path; // Points to PROGMEM or static string
  WebModule::Method method;
  WebModule::UnifiedRouteHandler handler;
  AuthRequirements authRequirements;

  RouteEntry()
      : path(nullptr), method(WebModule::WM_GET), handler(nullptr),
        authRequirements() {}

  RouteEntry(const char *p, WebModule::Method m,
             WebModule::UnifiedRouteHandler h,
             const AuthRequirements &auth = {AuthType::NONE})
      : path(p), method(m), handler(h), authRequirements(auth) {}
};

// Declare the global routeRegistry
extern std::vector<RouteEntry> routeRegistry;

#endif // ROUTE_ENTRY_H