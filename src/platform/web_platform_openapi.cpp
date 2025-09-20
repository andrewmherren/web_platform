#include "../../include/interface/openapi_types.h"
#include "../../include/route_entry.h"
#include "../../include/storage/storage_manager.h"
#include "../../include/web_platform.h"
#include <ArduinoJson.h>
#include <map>

// OpenAPI specification generation and serving functions
// This file contains the comprehensive OpenAPI generation logic that was moved
// from the old web_platform_openapi.cpp to work with the new pre-generation
// architecture

// Static constants for OpenAPI storage
const String WebPlatform::OPENAPI_COLLECTION = "openapi";
const String WebPlatform::OPENAPI_SPEC_KEY = "spec";

void WebPlatform::generateOpenAPISpec() {
  Serial.println("WebPlatform: Generating OpenAPI specification to storage...");

  // Check if we already have a valid spec in storage
  IDatabaseDriver *driver = StorageManager::driver();
  if (driver) {
    String existingSpec =
        driver->retrieve(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY);
    if (existingSpec.length() > 100) { // Basic validation
      Serial.printf("WebPlatform: Found existing OpenAPI spec (%d bytes)\n",
                    existingSpec.length());
      openAPISpecReady = true;
      return;
    }
  }

  size_t freeHeap = ESP.getFreeHeap();
  size_t targetSize;
  size_t maxBlock = ESP.getMaxAllocHeap();
  size_t maxAllowable = (size_t)(maxBlock * 0.8);
  targetSize = (maxAllowable < 24576) ? maxAllowable : 24576;

  if (targetSize < 8192) {
    Serial.println("ERROR: Insufficient memory for OpenAPI generation!");
    openAPISpecReady = false;
    return;
  }

  DynamicJsonDocument doc(targetSize);

  // Build the complete OpenAPI spec
  doc["openapi"] = "3.0.3";

  JsonObject info = doc.createNestedObject("info");
  info["title"] = String(deviceName) + " API";
  info["description"] = "RESTful API endpoints for " + String(deviceName) +
                        ". Only routes containing '/api/' are documented here.";
  info["version"] = "1.0.0";

  JsonArray servers = doc.createNestedArray("servers");
  JsonObject server = servers.createNestedObject();
  server["url"] = getBaseUrl();
  server["description"] = "Device API Server";

  // Security schemes
  JsonObject components = doc.createNestedObject("components");
  JsonObject securitySchemes = components.createNestedObject("securitySchemes");

  JsonObject bearerAuth = securitySchemes.createNestedObject("bearerAuth");
  bearerAuth["type"] = "http";
  bearerAuth["scheme"] = "bearer";
  bearerAuth["bearerFormat"] = "JWT";

  JsonObject cookieAuth = securitySchemes.createNestedObject("cookieAuth");
  cookieAuth["type"] = "apiKey";
  cookieAuth["in"] = "cookie";
  cookieAuth["name"] = "session";

  JsonObject tokenParam = securitySchemes.createNestedObject("tokenParam");
  tokenParam["type"] = "apiKey";
  tokenParam["in"] = "query";
  tokenParam["name"] = "access_token";

  JsonObject paths = doc.createNestedObject("paths");

  // Process all routes
  for (const auto &route : routeRegistry) {
    if (!route.handler)
      continue;

    String routePathStr = route.path ? String(route.path) : String("");
    if (routePathStr.indexOf("/api/") == -1)
      continue;

    JsonObject pathItem;
    const char *pathKey = route.path ? route.path : "";
    if (paths.containsKey(pathKey)) {
      pathItem = paths[pathKey];
    } else {
      pathItem = paths.createNestedObject(pathKey);
    }

    String methodStr;
    switch (route.method) {
    case WebModule::WM_GET:
      methodStr = "get";
      break;
    case WebModule::WM_POST:
      methodStr = "post";
      break;
    case WebModule::WM_PUT:
      methodStr = "put";
      break;
    case WebModule::WM_DELETE:
      methodStr = "delete";
      break;
    case WebModule::WM_PATCH:
      methodStr = "patch";
      break;
    default:
      methodStr = "get";
    }

    JsonObject operation = pathItem.createNestedObject(methodStr);

    if (route.summary && strlen(route.summary) > 0) {
      operation["summary"] = route.summary;
    } else {
      operation["summary"] = generateDefaultSummary(routePathStr, methodStr);
    }

    String operationId;
    if (route.operationId && strlen(route.operationId) > 0) {
      operationId = String(route.operationId);
    } else {
      operationId = generateOperationId(methodStr, routePathStr);
    }
    operation["operationId"] = operationId;

    JsonArray tags = operation.createNestedArray("tags");
    String defaultModuleTag = inferModuleFromPath(routePathStr);

    if (route.tags && strlen(route.tags) > 0) {
      tags.add(defaultModuleTag);
      String tagsCopy = String(route.tags);
      String lowerDefaultTag = defaultModuleTag;
      lowerDefaultTag.toLowerCase();

      while (tagsCopy.length() > 0) {
        int commaIndex = tagsCopy.indexOf(',');
        String tag =
            (commaIndex != -1) ? tagsCopy.substring(0, commaIndex) : tagsCopy;
        tag.trim();
        if (tag.length() > 0) {
          String lowerTag = tag;
          lowerTag.toLowerCase();
          if (lowerTag != lowerDefaultTag) {
            tags.add(tag);
          }
        }
        tagsCopy = (commaIndex != -1) ? tagsCopy.substring(commaIndex + 1) : "";
      }
    } else {
      tags.add(defaultModuleTag);
    }

    if (!route.authRequirements.empty()) {
      JsonArray security = operation.createNestedArray("security");
      for (const auto &authType : route.authRequirements) {
        if (authType == AuthType::TOKEN) {
          JsonObject secObj = security.createNestedObject();
          secObj.createNestedArray("bearerAuth");
          JsonObject tokenSecObj = security.createNestedObject();
          tokenSecObj.createNestedArray("tokenParam");
        } else if (authType == AuthType::SESSION) {
          JsonObject secObj = security.createNestedObject();
          secObj.createNestedArray("cookieAuth");
        }
      }
    }

    addParametersToOperation(operation, route);
    addResponsesToOperation(operation, route);

    if (methodStr == "post" || methodStr == "put" || methodStr == "patch") {
      addRequestBodyToOperation(operation, route);
    }
  }

  // Serialize to string
  String openAPIJson;
  size_t estimatedSize = measureJson(doc);
  openAPIJson.reserve(estimatedSize + 256);

  size_t bytesWritten = serializeJson(doc, openAPIJson);

  if (bytesWritten == 0 || openAPIJson.length() == 0) {
    Serial.println("ERROR: Failed to serialize OpenAPI spec");
    openAPISpecReady = false;
    return;
  }

  // Store in storage system
  if (driver &&
      driver->store(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY, openAPIJson)) {
    openAPISpecReady = true;
    Serial.printf("WebPlatform: OpenAPI spec generated and stored (%d bytes)\n",
                  openAPIJson.length());
  } else {
    Serial.println("ERROR: Failed to store OpenAPI spec in storage system");
    openAPISpecReady = false;
  }

  // Clear the temporary string to free memory
  openAPIJson = "";
}

void WebPlatform::streamPreGeneratedOpenAPISpec(WebResponse &res) const {
  if (!openAPISpecReady) {
    res.setStatus(503);
    res.setContent("{\"error\":\"OpenAPI specification not ready\"}",
                   "application/json");
    return;
  }

  // Retrieve from storage system
  IDatabaseDriver *driver = StorageManager::driver();
  if (!driver) {
    res.setStatus(500);
    res.setContent("{\"error\":\"Storage system unavailable\"}",
                   "application/json");
    return;
  }

  String openAPISpec = driver->retrieve(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY);

  if (openAPISpec.isEmpty()) {
    res.setStatus(404);
    res.setContent("{\"error\":\"OpenAPI specification not found in storage\"}",
                   "application/json");
    return;
  }

  Serial.printf("Serving OpenAPI spec from storage (%d bytes)\n",
                openAPISpec.length());

  res.setStatus(200);
  res.setContent(openAPISpec, "application/json");
  res.setHeader("Cache-Control", "public, max-age=300");
  res.setHeader("Content-Length", String(openAPISpec.length()));
}

// Helper functions for enhanced OpenAPI generation
String WebPlatform::generateDefaultSummary(const String &path,
                                           const String &method) const {
  // Generate meaningful summary from path and method
  String summary = method.substring(0, 1);
  summary.toUpperCase();
  summary += method.substring(1) + " ";

  // Clean up path for summary
  String cleanPath = path;
  cleanPath.replace("/api/", "");
  cleanPath.replace("/", " ");
  cleanPath.replace("_", " ");
  cleanPath.replace("-", " ");

  if (cleanPath.isEmpty()) {
    summary += "endpoint";
  } else {
    summary += cleanPath;
  }

  return summary;
}

String WebPlatform::generateOperationId(const String &method,
                                        const String &path) const {
  String operationId = method + path;
  // Sanitize operationId for OpenAPI compliance
  operationId.replace("/", "_");
  operationId.replace("-", "_");
  operationId.replace(".", "_");
  operationId.replace("{", "");
  operationId.replace("}", "");
  return operationId;
}

String WebPlatform::inferModuleFromPath(const String &path) const {
  // First, try to find a registered module that matches this path
  String moduleName = "";

  for (const auto &regModule : registeredModules) {
    if (path.startsWith(regModule.basePath)) {
      moduleName = regModule.module->getModuleName();
      break;
    }
  }

  // If we found a registered module, format its name
  if (!moduleName.isEmpty()) {
    return formatModuleName(moduleName);
  }

  // All WebPlatform internal routes (including auth routes) should return "Web
  // Platform" The specific functional tags like "User Management" will be added
  // as explicit tags
  return "Web Platform";
}

String WebPlatform::formatModuleName(const String &moduleName) const {
  String formatted = moduleName;

  // Replace underscores and dashes with spaces
  formatted.replace("_", " ");
  formatted.replace("-", " ");

  // Capitalize first letter and letters after spaces
  bool capitalizeNext = true;
  for (size_t i = 0; i < formatted.length(); i++) {
    if (capitalizeNext && formatted[i] >= 'a' && formatted[i] <= 'z') {
      formatted[i] = formatted[i] - 'a' + 'A';
      capitalizeNext = false;
    } else if (formatted[i] == ' ') {
      capitalizeNext = true;
    } else {
      capitalizeNext = false;
    }
  }

  return formatted;
}

bool WebPlatform::hasTokenAuth(const AuthRequirements &requirements) const {
  for (const auto &authType : requirements) {
    if (authType == AuthType::TOKEN) {
      return true;
    }
  }
  return false;
}

void WebPlatform::addParametersToOperation(JsonObject &operation,
                                           const RouteEntry &route) const {
  JsonArray parameters = operation.createNestedArray("parameters");

  // Track parameter names to avoid duplicates
  std::map<String, bool> parameterNames;

  // First, add custom parameters from module documentation
  // These take precedence and may override auto-generated ones
  if (route.parameters && strlen(route.parameters) > 0) {
    DynamicJsonDocument paramDoc(2048);
    if (deserializeJson(paramDoc, route.parameters) ==
        DeserializationError::Ok) {
      if (paramDoc.is<JsonArray>()) {
        JsonArray customParams = paramDoc.as<JsonArray>();
        for (JsonVariant param : customParams) {
          if (param.is<JsonObject>()) {
            JsonObject paramObj = param.as<JsonObject>();
            if (paramObj.containsKey("name") && paramObj.containsKey("in")) {
              String paramName = paramObj["name"].as<String>();
              String paramIn = paramObj["in"].as<String>();
              String paramKey = paramName + ":" + paramIn;

              if (parameterNames.find(paramKey) == parameterNames.end()) {
                parameters.add(param);
                parameterNames[paramKey] = true;
              }
            }
          }
        }
      }
    }
  }

  // Then add auto-generated path parameters only if not already defined
  String routePathStr = route.path ? String(route.path) : String("");
  if (routePathStr.indexOf("{") != -1) {
    String pathCopy = routePathStr;
    int startPos = 0;
    while ((startPos = pathCopy.indexOf("{", startPos)) != -1) {
      int endPos = pathCopy.indexOf("}", startPos);
      if (endPos != -1) {
        String paramName = pathCopy.substring(startPos + 1, endPos);
        String paramKey = paramName + ":path";

        // Only add if not already defined by custom parameters
        if (parameterNames.find(paramKey) == parameterNames.end()) {
          JsonObject param = parameters.createNestedObject();
          param["name"] = paramName;
          param["in"] = "path";
          param["required"] = true;
          param["description"] = "Path parameter: " + paramName;

          JsonObject schema = param.createNestedObject("schema");
          schema["type"] = "string";

          // Add parameter constraints if available
          if (route.parameterConstraints &&
              strlen(route.parameterConstraints) > 0) {
            // Parse parameter constraints JSON and apply to matching parameters
            DynamicJsonDocument constraintsDoc(1024);
            if (deserializeJson(constraintsDoc, route.parameterConstraints) ==
                DeserializationError::Ok) {
              if (constraintsDoc.containsKey(paramName)) {
                JsonObject constraints = constraintsDoc[paramName];
                if (constraints.containsKey("pattern")) {
                  schema["pattern"] = constraints["pattern"];
                }
                if (constraints.containsKey("minLength")) {
                  schema["minLength"] = constraints["minLength"];
                }
                if (constraints.containsKey("maxLength")) {
                  schema["maxLength"] = constraints["maxLength"];
                }
              }
            }
          }

          parameterNames[paramKey] = true;
        }

        startPos = endPos + 1;
      } else {
        break;
      }
    }
  }
}

void WebPlatform::addResponsesToOperation(JsonObject &operation,
                                          const RouteEntry &route) const {
  JsonObject responses = operation.createNestedObject("responses");

  // Success response
  JsonObject response200 = responses.createNestedObject("200");
  response200["description"] = "Successful operation";

  // Add content type and examples
  JsonObject content = response200.createNestedObject("content");
  String contentType = (route.contentType && strlen(route.contentType) > 0)
                           ? String(route.contentType)
                           : "application/json";
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add response schema if provided
  if (route.responseSchema && strlen(route.responseSchema) > 0) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, route.responseSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add response example if provided
  if (route.responseExample && strlen(route.responseExample) > 0) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, route.responseExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Add module-provided response info
  if (route.responseInfo && strlen(route.responseInfo) > 0) {
    DynamicJsonDocument responseDoc(2048);
    if (deserializeJson(responseDoc, route.responseInfo) ==
        DeserializationError::Ok) {
      // Merge additional response information
      for (JsonPair kv : responseDoc.as<JsonObject>()) {
        if (!responses.containsKey(kv.key().c_str())) {
          responses[kv.key().c_str()] = kv.value();
        }
      }
    }
  }

  // Add standard error responses for authenticated routes
  if (!route.authRequirements.empty()) {
    JsonObject response401 = responses.createNestedObject("401");
    response401["description"] = "Unauthorized - Authentication required";

    JsonObject response403 = responses.createNestedObject("403");
    response403["description"] = "Forbidden - Insufficient permissions";
  }

  JsonObject response500 = responses.createNestedObject("500");
  response500["description"] = "Internal server error";
}

void WebPlatform::addRequestBodyToOperation(JsonObject &operation,
                                            const RouteEntry &route) const {
  JsonObject requestBody = operation.createNestedObject("requestBody");
  requestBody["description"] = "Request payload";

  JsonObject content = requestBody.createNestedObject("content");
  String contentType = (route.contentType && strlen(route.contentType) > 0)
                           ? String(route.contentType)
                           : "application/json";
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add request schema if provided
  if (route.requestSchema && strlen(route.requestSchema) > 0) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, route.requestSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add request example if provided
  if (route.requestExample && strlen(route.requestExample) > 0) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, route.requestExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Make request body required for POST/PUT operations by default
  requestBody["required"] = true;
}