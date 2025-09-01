#include "../include/route_entry.h"
#include "../include/web_platform.h"
#include "../include/interface/auth_types.h"

// Enhanced route debugging tools for Phase 2

void WebPlatform::printUnifiedRoutes(const String* moduleBasePath, IWebModule* module) const {
  // Determine what we're printing based on parameters
  String headerTitle;
  if (module && moduleBasePath) {
    headerTitle = "Module '" + module->getModuleName() + "' Routes (" + *moduleBasePath + ")";
  } else {
    headerTitle = "WebPlatform Route Registry";
  }
  
  Serial.printf("\n=== %s ===\n", headerTitle.c_str());
  Serial.println(
      "PATH                     METHOD  AUTH         STATUS     SOURCE");
  Serial.println(
      "------------------------ ------- ------------ ---------- -----------");

  int routeCount = 0;
  for (const auto &route : routeRegistry) {
    // If filtering by module, only show routes that start with the module's base path
    bool shouldShow = true;
    if (moduleBasePath && module) {
      // Only show routes that start with the module's base path
      shouldShow = route.path.startsWith(*moduleBasePath);
      
      // Special case for module root path
      if (*moduleBasePath != "/" && route.path == *moduleBasePath) {
        shouldShow = true;
      }
    }
    
    if (!shouldShow) {
      continue;
    }
    
    routeCount++;
    
    // Format path with padding (up to 24 chars)
    String pathStr = route.path;
    if (pathStr.length() > 24) {
      pathStr = pathStr.substring(0, 21) + "...";
    } else {
      while (pathStr.length() < 24) {
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

    // Format status (10 chars)
    String statusStr = route.disabled ? "DISABLED" : "ENABLED";
    while (statusStr.length() < 10) {
      statusStr += " ";
    }

    // Format source
    String sourceStr = route.isOverride ? "OVERRIDE" : "DEFAULT";

    Serial.printf("%s %s %s %s %s\n", pathStr.c_str(), methodStr.c_str(),
                  authStr.c_str(), statusStr.c_str(), sourceStr.c_str());
  }

  Serial.println("=============================================");
  if (module && moduleBasePath) {
    Serial.printf("Total module routes: %d\n\n", routeCount);
  } else {
    Serial.printf("Total routes: %d\n\n", routeRegistry.size());
  }
}

size_t WebPlatform::getRouteCount() const {
  size_t enabledCount = 0;
  for (const auto &route : routeRegistry) {
    if (!route.disabled) {
      enabledCount++;
    }
  }
  return enabledCount;
}

// Validation warnings for common route issues
void WebPlatform::validateRoutes() const {
  Serial.println("\n=== Route Validation ===");

  // Check for duplicate paths
  std::map<String, std::vector<size_t>> pathMethodMap;

  for (size_t i = 0; i < routeRegistry.size(); i++) {
    const auto &route = routeRegistry[i];
    String key = route.path + ":" + wmMethodToString(route.method);
    pathMethodMap[key].push_back(i);
  }

  // Report duplicates
  bool hasDuplicates = false;
  for (const auto &entry : pathMethodMap) {
    if (entry.second.size() > 1) {
      hasDuplicates = true;
      Serial.printf("WARNING: Duplicate route found: %s\n",
                    entry.first.c_str());

      for (const auto &idx : entry.second) {
        const auto &route = routeRegistry[idx];
        Serial.printf("  - %s (Override: %s, Disabled: %s)\n",
                      route.disabled ? "DISABLED" : "ENABLED",
                      route.isOverride ? "YES" : "no",
                      route.disabled ? "YES" : "no");
      }
    }
  }

  if (!hasDuplicates) {
    Serial.println("No duplicate routes found.");
  }

  // Check for routes without authentication
  bool hasUnauthenticatedRoutes = false;
  for (const auto &route : routeRegistry) {
    if (!route.disabled && !AuthUtils::requiresAuth(route.authRequirements)) {
      if (!hasUnauthenticatedRoutes) {
        Serial.println("Routes without authentication requirements:");
        hasUnauthenticatedRoutes = true;
      }
      Serial.printf("  - %s %s\n", wmMethodToString(route.method).c_str(),
                    route.path.c_str());
    }
  }

  if (!hasUnauthenticatedRoutes) {
    Serial.println("All routes have authentication requirements.");
  }

  Serial.println("======================\n");
}