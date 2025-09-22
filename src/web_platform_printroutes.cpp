#include "../include/interface/auth_types.h"
#include "../include/route_entry.h"
#include "../include/web_platform.h"

void WebPlatform::printUnifiedRoutes() const {
  // Determine what we're printing based on parameters
  DEBUG_PRINTF("\n=== WebPlatform Route Registry ===\n");
  DEBUG_PRINTLN("PATH                        METHOD  AUTH");
  DEBUG_PRINTLN("--------------------------- ------- -------------");

  int routeCount = 0;
  for (const auto &route : routeRegistry) {
    // If filtering by module, only show routes that start with the module's
    // base path
    routeCount++;

    // Format path with padding (up to 24 chars)
    String pathStr = route.path;
    if (pathStr.length() > 27) {
      pathStr = pathStr.substring(0, 24) + "...";
    } else {
      while (pathStr.length() < 27) {
        pathStr += " ";
      }
    }

    // Format method (7 chars)
    String methodStr = wmMethodToString(route.method);
    while (methodStr.length() < 7) {
      methodStr += " ";
    }

    // Format auth requirements (12 chars)
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

size_t WebPlatform::getRouteCount() const {
  size_t enabledCount = 0;
  for (const auto &route : routeRegistry) {
    if (route.handler) {
      enabledCount++;
    }
  }
  return enabledCount;
}