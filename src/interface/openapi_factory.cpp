#if OPENAPI_ENABLED
#include "interface/openapi_factory.h"

OpenAPIDocumentation OpenAPIFactory::create(const String &summary,
                                            const String &description,
                                            const String &operationId,
                                            const std::vector<String> &tags) {
  return OpenAPIDocumentation(summary, description, operationId, tags);
}

String OpenAPIFactory::createSuccessResponse(const String &description) {
  return R"({
    "type": "object",
    "properties": {
      "success": {"type": "boolean", "description": "Operation result"},
      "message": {"type": "string", "description": ")" +
         description + R"("},
      "data": {"type": "object", "description": "Response data"}
    },
    "required": ["success", "message"]
  })";
}

String OpenAPIFactory::createErrorResponse(const String &description) {
  return R"({
    "type": "object",
    "properties": {
      "error": {"type": "boolean", "description": "Error indicator"},
      "message": {"type": "string", "description": ")" +
         description + R"("},
      "code": {"type": "integer", "description": "Error code"}
    },
    "required": ["error", "message"]
  })";
}

String OpenAPIFactory::createListResponse(const String &itemDescription) {
  return R"({
    "type": "object",
    "properties": {
      "items": {"type": "array", "items": {}, "description": "List of )" +
         itemDescription + R"("},
      "total": {"type": "integer", "description": "Total number of items"}
    },
    "required": ["items", "total"]
  })";
}

String OpenAPIFactory::createJsonRequest(const String &description,
                                         const String &properties) {
  return R"({
    "type": "object",
    "description": ")" +
         description + R"(",
    "properties": )" +
         properties + R"(
  })";
}

String OpenAPIFactory::createStringRequest(const String &description,
                                           int minLength) {
  return R"({
    "type": "string",
    "description": ")" +
         description + R"(",
    "minLength": )" +
         String(minLength) + R"(
  })";
}

String OpenAPIFactory::createIdParameter(const String &name,
                                         const String &description) {
  return R"({
    "name": ")" +
         name + R"(",
    "in": "path",
    "required": true,
    "schema": {"type": "string"},
    "description": ")" +
         description + R"("
  })";
}

String OpenAPIFactory::generateOperationId(const String &method,
                                           const String &resource) {
  String firstChar = String(resource.charAt(0));
  String restOfString = resource.substring(1);

  // Convert to lowercase (platform-compatible way)
  for (unsigned int i = 0; i < restOfString.length(); i++) {
    if (restOfString[i] >= 'A' && restOfString[i] <= 'Z') {
      restOfString[i] = restOfString[i] + 32; // Convert to lowercase
    }
  }

  return method + firstChar + restOfString;
}

String OpenAPIFactory::formatTag(const String &moduleName) {
  return moduleName;
}
#endif