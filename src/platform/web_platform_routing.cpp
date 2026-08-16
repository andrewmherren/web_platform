#include "web_platform.h"

// Route registration is a thin, public-API-preserving forward onto Router -
// see platform/router.h/.cpp for path normalization, matching, and dispatch.

void WebPlatform::registerWebRoute(const String &path,
                                   WebModule::UnifiedRouteHandler handler,
                                   const AuthRequirements &auth,
                                   WebModule::Method method) {
  router.registerWebRoute(path, handler, auth, method);
}

void WebPlatform::registerApiRoute(const String &path,
                                   WebModule::UnifiedRouteHandler handler,
                                   const AuthRequirements &auth,
                                   WebModule::Method method,
                                   const OpenAPIDocumentation &docs) {
  router.registerApiRoute(path, handler, auth, method, docs);
}

void WebPlatform::disableRoute(const String &path, WebModule::Method method) {
  router.disableRoute(path, method);
}
