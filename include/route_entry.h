#ifndef ROUTE_ENTRY_H
#define ROUTE_ENTRY_H
#include "interface/auth_types.h"
#include "interface/web_module_interface.h"
#include <Arduino.h>

// Complete RouteEntry definition
struct RouteEntry {
  String path;
  WebModule::Method method;
  WebModule::UnifiedRouteHandler handler;
  AuthRequirements authRequirements;

  // OpenAPI-compatible documentation fields
  String summary;
  String operationId;
  String parameters;
  String responseInfo;
  String tags;
  String requestExample;
  String responseExample;
  String requestSchema;
  String responseSchema;
  String contentType;
  String parameterConstraints;
  String description;

  RouteEntry() : method(WebModule::WM_GET) {}

  RouteEntry(const String &p, WebModule::Method m,
             WebModule::UnifiedRouteHandler h,
             const AuthRequirements &auth = {AuthType::NONE})
      : path(p), method(m), handler(h), authRequirements(auth) {}
};

// Declare the global routeRegistry
extern std::vector<RouteEntry> routeRegistry;

#endif // ROUTE_ENTRY_H