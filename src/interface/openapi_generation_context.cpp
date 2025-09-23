#include "interface/openapi_generation_context.h"
#include "utilities/debug_macros.h"

void OpenAPIGenerationContext::beginGeneration() {
  if (generationInProgress) {
    WARN_PRINTLN("OpenAPIGenerationContext: Generation already in progress");
    return;
  }

  generationInProgress = true;
  generationStartTime = millis();
  initialHeapSize = ESP.getFreeHeap();

  // Clear any existing data and pre-allocate reasonable capacity
  apiRouteDocs.clear();

  // CRITICAL FIX: Increase capacity to handle all API routes (23 found +
  // growth) The system has 23 API routes, so reserve 40 to be safe
  apiRouteDocs.reserve(40);
}

void OpenAPIGenerationContext::addRouteDocumentation(
    const String &path, WebModule::Method method,
    const OpenAPIDocumentation &docs, const AuthRequirements &auth) {
  if (!generationInProgress) {
    return;
  }

  // Only store routes that have actual documentation or are API routes
  // This prevents storing empty documentation for non-API web routes
  bool isApiRoute = path.indexOf("/api/") != -1;
  bool hasDocumentation = docs.hasDocumentation();

  if (isApiRoute || hasDocumentation) {
    // CRITICAL FIX: Check for duplicates before adding
    for (const auto &existing : apiRouteDocs) {
      if (existing.path == path && existing.method == method) {
        return;
      }
    }

    // Safety check to prevent memory issues
    if (apiRouteDocs.size() >= apiRouteDocs.capacity()) {
      ERROR_PRINTF("OpenAPIGenerationContext: Capacity exceeded (%d/%d), "
                   "ignoring route %s\n",
                   apiRouteDocs.size(), apiRouteDocs.capacity(), path.c_str());
      return;
    }

    apiRouteDocs.emplace_back(path, method, docs, auth);
  }
}

const std::vector<OpenAPIGenerationContext::RouteDocumentation> &
OpenAPIGenerationContext::getApiRoutes() const {
  return apiRouteDocs;
}

void OpenAPIGenerationContext::endGeneration() {
  if (!generationInProgress) {
    return;
  }

  // Critical: Free all temporary storage
  apiRouteDocs.clear();
  apiRouteDocs.shrink_to_fit(); // Force deallocation

  generationInProgress = false;
}