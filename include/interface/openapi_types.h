#ifndef OPENAPI_TYPES_H
#define OPENAPI_TYPES_H

#include <Arduino.h>
#include <vector>

#ifndef WEB_PLATFORM_OPENAPI
#define WEB_PLATFORM_OPENAPI 1  // Default to enabled
#endif

#define OPENAPI_ENABLED WEB_PLATFORM_OPENAPI

// Template-based OpenAPI documentation - full implementation when enabled
template<bool Enabled = OPENAPI_ENABLED>
struct OpenAPIDoc {
  String summary;           // Short summary of the operation
  String operationId;       // Unique identifier for the operation
  String description;       // Detailed description
  std::vector<String> tags; // Tags for organizing operations
  String requestExample;    // JSON string containing example request body
  String responseExample;   // JSON string containing example response body
  String requestSchema;     // JSON string containing request schema definition
  String responseSchema;    // JSON string containing response schema definition
  String parameters;        // JSON string containing parameter definitions
  String responsesJson;     // JSON string containing response definitions

  // Default constructor
  OpenAPIDoc() = default;

  // Convenience constructor with common fields - tags are now optional
  OpenAPIDoc(const String &sum, const String &desc = "", const String &opId = "")
      : summary(sum), operationId(opId), description(desc), tags({}) {}

  // Constructor with explicit tags (for when you want to override defaults)
  OpenAPIDoc(const String &sum, const String &desc, const String &opId, 
             const std::vector<String> &t)
      : summary(sum), operationId(opId), description(desc), tags(t) {}

  // Check if any documentation is provided
  bool hasDocumentation() const {
    return !summary.isEmpty() || !description.isEmpty() ||
           !operationId.isEmpty() || !tags.empty() ||
           !requestExample.isEmpty() || !responseExample.isEmpty() ||
           !requestSchema.isEmpty() || !responseSchema.isEmpty() ||
           !parameters.isEmpty() || !responsesJson.isEmpty();
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

// Template specialization - empty implementation when disabled
template<>
struct OpenAPIDoc<false> {
  // Accept any constructor arguments and do nothing
  OpenAPIDoc() = default;
  
  template<typename... Args>
  OpenAPIDoc(Args&&... args) {}

  // All methods return empty/default values and inline to nothing
  bool hasDocumentation() const { return false; }
  String getTagsString() const { return ""; }
};

// Type alias for easy use
using OpenAPIDocumentation = OpenAPIDoc<OPENAPI_ENABLED>;

// Convenience macros for cleaner API
#if OPENAPI_ENABLED
    #define API_DOC(...) OpenAPIDocumentation(__VA_ARGS__)
    #define API_DOC_BLOCK(code) (code)
    #define COMPLEX_API_DOC(summary, desc, reqSchema, respExample) \
        []() { \
            auto doc = OpenAPIDocumentation(summary, desc); \
            doc.requestSchema = reqSchema; \
            doc.responseExample = respExample; \
            return doc; \
        }()
#else
    #define API_DOC(...) OpenAPIDocumentation()
    #define API_DOC_BLOCK(code) OpenAPIDocumentation()
    #define COMPLEX_API_DOC(...) OpenAPIDocumentation()
#endif

#endif // OPENAPI_TYPES_H