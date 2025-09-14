#ifndef OPENAPI_TYPES_H
#define OPENAPI_TYPES_H

#include <Arduino.h>
#include <vector>

// OpenAPI documentation structure for route registration
struct OpenAPIDocumentation {
  String summary;           // Short summary of the operation
  String operationId;       // Unique identifier for the operation
  String description;       // Detailed description
  std::vector<String> tags; // Tags for organizing operations
  String requestExample;    // JSON string containing example request body
  String responseExample;   // JSON string containing example response body
  String requestSchema;     // JSON string containing request schema definition
  String responseSchema;    // JSON string containing response schema definition
  String parametersJson;    // JSON string containing parameter definitions
  String responsesJson;     // JSON string containing response definitions

  // Default constructor
  OpenAPIDocumentation() = default;

  // Convenience constructor with common fields - tags are now optional
  OpenAPIDocumentation(const String &sum, const String &desc = "",
                       const String &opId = "")
      : summary(sum), operationId(opId), description(desc), tags({}) {}

  // Constructor with explicit tags (for when you want to override defaults)
  OpenAPIDocumentation(const String &sum, const String &desc,
                       const String &opId, const std::vector<String> &t)
      : summary(sum), operationId(opId), description(desc), tags(t) {}

  // Check if any documentation is provided
  bool hasDocumentation() const {
    return !summary.isEmpty() || !description.isEmpty() ||
           !operationId.isEmpty() || !tags.empty() ||
           !requestExample.isEmpty() || !responseExample.isEmpty() ||
           !requestSchema.isEmpty() || !responseSchema.isEmpty() ||
           !parametersJson.isEmpty() || !responsesJson.isEmpty();
  }

  // Helper to get tags as comma-separated string
  String getTagsString() const {
    if (tags.empty())
      return "";
    String result = "";
    for (size_t i = 0; i < tags.size(); i++) {
      if (i > 0)
        result += ",";
      result += tags[i];
    }
    return result;
  }
};

#endif // OPENAPI_TYPES_H