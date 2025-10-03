#ifndef OPENAPI_TYPES_H
#define OPENAPI_TYPES_H

#include <Arduino.h>
#include <type_traits>
#include <vector>

#ifndef WEB_PLATFORM_OPENAPI
// NOSONAR: Build flags must use macros for conditional compilation
#define WEB_PLATFORM_OPENAPI 1 // Default to enabled
#endif

#ifndef WEB_PLATFORM_MAKERAPI
// NOSONAR: Build flags must use macros for conditional compilation
#define WEB_PLATFORM_MAKERAPI 0 // Default to enabled
#endif

#define OPENAPI_ENABLED WEB_PLATFORM_OPENAPI
#define MAKERAPI_ENABLED WEB_PLATFORM_MAKERAPI

// Template-based OpenAPI documentation - full implementation when enabled
template <bool Enabled = (OPENAPI_ENABLED || MAKERAPI_ENABLED)>
struct OpenAPIDoc {
private:
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

public:
  // Default constructor
  OpenAPIDoc() = default;

  // Copy/move constructors and assignment operators
  OpenAPIDoc(const OpenAPIDoc &) = default;
  OpenAPIDoc &operator=(const OpenAPIDoc &) = default;
  OpenAPIDoc(OpenAPIDoc &&) noexcept = default;
  OpenAPIDoc &operator=(OpenAPIDoc &&) noexcept = default;

  // Convenience constructor with common fields - tags are now optional
  OpenAPIDoc(const String &sum, const String &desc = "",
             const String &opId = "")
      : summary(sum), operationId(opId), description(desc), tags({}) {}

  // Constructor with explicit tags (for when you want to override defaults)
  OpenAPIDoc(const String &sum, const String &desc, const String &opId,
             const std::vector<String> &t)
      : summary(sum), operationId(opId), description(desc), tags(t) {}

  // Builder pattern methods - each returns a reference to the current object
  // for chaining
  OpenAPIDoc &withRequestExample(const String &example) {
    this->requestExample = example;
    return *this;
  }

  OpenAPIDoc &withResponseExample(const String &example) {
    this->responseExample = example;
    return *this;
  }

  // OpenAPI 3.0 compliant request bodies
  OpenAPIDoc &withRequestBody(const String &requestBody) {
    this->requestSchema = requestBody;
    return *this;
  }

  OpenAPIDoc &withResponseSchema(const String &schema) {
    this->responseSchema = schema;
    return *this;
  }

  OpenAPIDoc &withParameters(const String &params) {
    this->parameters = params;
    return *this;
  }

  OpenAPIDoc &withResponses(const String &responses) {
    this->responsesJson = responses;
    return *this;
  }

  // Getters
  const String &getSummary() const { return summary; }
  const String &getOperationId() const { return operationId; }
  const String &getDescription() const { return description; }
  const std::vector<String> &getTags() const { return tags; }
  const String &getRequestExample() const { return requestExample; }
  const String &getResponseExample() const { return responseExample; }
  const String &getRequestSchema() const { return requestSchema; }
  const String &getResponseSchema() const { return responseSchema; }
  const String &getParameters() const { return parameters; }
  const String &getResponsesJson() const { return responsesJson; }

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
template <> struct OpenAPIDoc<false> {
  // Accept any constructor arguments and do nothing
  OpenAPIDoc() = default;

  // Copy/move constructors and assignment operators
  OpenAPIDoc(const OpenAPIDoc &) = default;
  OpenAPIDoc &operator=(const OpenAPIDoc &) = default;
  OpenAPIDoc(OpenAPIDoc &&) noexcept = default;
  OpenAPIDoc &operator=(OpenAPIDoc &&) noexcept = default;

  template <typename... Args,
            typename std::enable_if<!(sizeof...(Args) == 1), int>::type = 0>
  OpenAPIDoc(Args &&...args) {}

  // Builder pattern methods that do nothing but return self for chaining
  OpenAPIDoc &withRequestExample(const String &) { return *this; }
  OpenAPIDoc &withResponseExample(const String &) { return *this; }
  OpenAPIDoc &withRequestBody(const String &) { return *this; }
  OpenAPIDoc &withResponseSchema(const String &) { return *this; }
  OpenAPIDoc &withParameters(const String &) { return *this; }
  OpenAPIDoc &withResponses(const String &) { return *this; }

  // Getters that return empty values
  String getSummary() const { return ""; }
  String getOperationId() const { return ""; }
  String getDescription() const { return ""; }
  std::vector<String> getTags() const { return {}; }
  String getRequestExample() const { return ""; }
  String getResponseExample() const { return ""; }
  String getRequestSchema() const { return ""; }
  String getResponseSchema() const { return ""; }
  String getParameters() const { return ""; }
  String getResponsesJson() const { return ""; }

  // All methods return empty/default values and inline to nothing
  bool hasDocumentation() const { return false; }
  String getTagsString() const { return ""; }
};

// Type alias for easy use - enabled if either OpenAPI or MakerAPI is enabled
using OpenAPIDocumentation = OpenAPIDoc<(OPENAPI_ENABLED || MAKERAPI_ENABLED)>;

// Convenience macros for cleaner API - enabled if either OpenAPI or MakerAPI is
// enabled
#if (OPENAPI_ENABLED || MAKERAPI_ENABLED)
#define API_DOC(...) OpenAPIDocumentation(__VA_ARGS__)
#define API_DOC_BLOCK(code) (code)
#define COMPLEX_API_DOC(summary, desc, reqSchema, respExample)                 \
  []() {                                                                       \
    auto doc = OpenAPIDocumentation(summary, desc);                            \
    doc.requestSchema = reqSchema;                                             \
    doc.responseExample = respExample;                                         \
    return doc;                                                                \
  }()
#else
#define API_DOC(...) OpenAPIDocumentation()
#define API_DOC_BLOCK(code) OpenAPIDocumentation()
#define COMPLEX_API_DOC(...) OpenAPIDocumentation()
#endif

#endif // OPENAPI_TYPES_H