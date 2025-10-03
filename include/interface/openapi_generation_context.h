#ifndef OPENAPI_GENERATION_CONTEXT_H
#define OPENAPI_GENERATION_CONTEXT_H

#include "interface/auth_types.h"
#include "interface/openapi_types.h"
#include "interface/web_module_interface.h"
#include <Arduino.h>
#include <vector>

/**
 * OpenAPIGenerationContext - Temporary storage for OpenAPI documentation during
 * generation
 *
 * This class provides temporary storage for route documentation that only
 * exists during the OpenAPI spec generation phase. This eliminates the
 * permanent memory overhead of storing documentation strings in RouteEntry
 * structures.
 *
 * Memory Benefits:
 * - Eliminates permanent 9.6% heap usage from RouteStringPool
 * - Only allocates memory during generation, then frees it immediately
 * - Stores only API routes with actual documentation
 */
class OpenAPIGenerationContext {
public:
  /**
   * RouteDocumentation - Temporary storage for a single route's documentation
   * This struct only exists during generation and is freed afterwards
   */
  struct RouteDocumentation {
    String path;                       // Route path (e.g., "/api/users/{id}")
    WebModule::Method method;          // HTTP method
    OpenAPIDocumentation docs;         // Complete documentation
    AuthRequirements authRequirements; // Authentication requirements

    RouteDocumentation() = default;
    RouteDocumentation(const RouteDocumentation &) = default;
    RouteDocumentation &operator=(const RouteDocumentation &) = default;
    RouteDocumentation(RouteDocumentation &&) noexcept = default;
    RouteDocumentation &operator=(RouteDocumentation &&) noexcept = default;

    RouteDocumentation(const String &p, WebModule::Method m,
                       const OpenAPIDocumentation &d,
                       const AuthRequirements &auth)
        : path(p), method(m), docs(d), authRequirements(auth) {}
  };

private:
  std::vector<RouteDocumentation> apiRouteDocs;
  bool generationInProgress = false;
  size_t initialHeapSize = 0;
  unsigned long generationStartTime = 0;

public:
  /**
   * Begin OpenAPI generation phase
   * Initializes temporary storage and memory tracking
   */
  void beginGeneration();

  /**
   * Add route documentation for temporary storage
   * Only called during generation phase for API routes with documentation
   *
   * @param path Route path
   * @param method HTTP method
   * @param docs OpenAPI documentation
   * @param auth Authentication requirements
   */
  void addRouteDocumentation(const String &path, WebModule::Method method,
                             const OpenAPIDocumentation &docs,
                             const AuthRequirements &auth);

  /**
   * Get all collected API route documentation
   * Used by OpenAPI generation system
   */
  const std::vector<RouteDocumentation> &getApiRoutes() const;

  /**
   * End generation phase and free all temporary storage
   * This is the critical cleanup step that frees ~9.6% heap
   */
  void endGeneration();

  /**
   * Check if generation is currently in progress
   */
  bool isGenerating() const { return generationInProgress; }

  /**
   * Get count of stored API routes
   */
  size_t getApiRouteCount() const { return apiRouteDocs.size(); }
};

#endif // OPENAPI_GENERATION_CONTEXT_H