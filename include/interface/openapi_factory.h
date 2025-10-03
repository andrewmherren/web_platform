#ifndef OPENAPI_FACTORY_H
#define OPENAPI_FACTORY_H

#include "auth_types.h"
#include "openapi_types.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Base factory class for creating standardized OpenAPI documentation
 * Provides structural patterns and common schemas that can be extended by
 * modules
 */
class OpenAPIFactory {
public:
  // Core factory method - creates basic documentation structure
  static OpenAPIDocumentation create(const String &summary,
                                     const String &description = "",
                                     const String &operationId = "",
                                     const std::vector<String> &tags = {});

  // Shorthand for common response patterns with builder pattern
  static OpenAPIDocumentation createWithSuccessResponse(
      const String &summary, const String &description,
      const String &operationId, const std::vector<String> &tags,
      const String &responseDescription = "Operation successful");

  // Schema generators for common response patterns
  static String
  createSuccessResponse(const String &description = "Operation successful");
  static String
  createErrorResponse(const String &description = "Operation failed");
  static String createListResponse(const String &itemDescription = "items");

  // Common parameter schemas
  static String
  createIdParameter(const String &name = "id",
                    const String &description = "Resource identifier");

  // Utility methods for operation ID generation and tag formatting
  static String generateOperationId(const String &method,
                                    const String &resource);
  static String formatTag(const String &moduleName);
};
#endif // OPENAPI_FACTORY_H