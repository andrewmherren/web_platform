#ifndef ROUTE_ENTRY_H
#define ROUTE_ENTRY_H
#include "interface/auth_types.h"
#include "interface/web_module_interface.h"
#include "platform/route_string_pool.h"
#include <Arduino.h>

// Complete RouteEntry definition - optimized for memory efficiency
struct RouteEntry {
  const char *path; // Points to PROGMEM or static string
  WebModule::Method method;
  WebModule::UnifiedRouteHandler handler;
  AuthRequirements authRequirements;

#if OPENAPI_ENABLED
  // OpenAPI-compatible documentation fields - using const char* to avoid heap
  // allocations
  const char *summary;
  const char *operationId;
  const char *parameters;
  const char *responseInfo;
  const char *tags;
  const char *requestExample;
  const char *responseExample;
  const char *requestSchema;
  const char *responseSchema;
  const char *contentType;
  const char *parameterConstraints;
  const char *description;
#endif

  RouteEntry()
      : path(nullptr), method(WebModule::WM_GET), handler(nullptr), authRequirements()
#if OPENAPI_ENABLED
        , summary(nullptr), operationId(nullptr), parameters(nullptr), responseInfo(nullptr),
        tags(nullptr), requestExample(nullptr), responseExample(nullptr),
        requestSchema(nullptr), responseSchema(nullptr), contentType(nullptr),
        parameterConstraints(nullptr), description(nullptr)
#endif
      {}

  RouteEntry(const char *p, WebModule::Method m,
             WebModule::UnifiedRouteHandler h,
             const AuthRequirements &auth = {AuthType::NONE})
      : path(p), method(m), handler(h), authRequirements(auth)
#if OPENAPI_ENABLED
        , summary(nullptr), operationId(nullptr), parameters(nullptr),
        responseInfo(nullptr), tags(nullptr), requestExample(nullptr),
        responseExample(nullptr), requestSchema(nullptr),
        responseSchema(nullptr), contentType(nullptr),
        parameterConstraints(nullptr), description(nullptr)
#endif
      {}
};

// Declare the global routeRegistry
extern std::vector<RouteEntry> routeRegistry;

#endif // ROUTE_ENTRY_H