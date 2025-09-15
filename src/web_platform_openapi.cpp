#include "../include/storage/storage_manager.h"
#include "../include/web_platform.h"
#include <ArduinoJson.h>
#include <map>

// Use the constant defined in web_platform_openapi_cache.cpp
extern const char *OPENAPI_CACHE_COLLECTION;

// OpenAPI specification generation
String WebPlatform::getOpenAPISpec() const {
  return getOpenAPISpec(AuthType::NONE, true); // Use cache by default
}

String WebPlatform::getOpenAPISpec(AuthType filterType) const {
  return getOpenAPISpec(filterType, true); // Use cache by default
}

String WebPlatform::getOpenAPISpec(AuthType filterType, bool useCache) const {
  // Check cache first if requested
  if (useCache && hasValidOpenAPICache(filterType)) {
    Serial.println("Serving OpenAPI spec from cache for filter: " +
                   String((int)filterType));
    return getCachedOpenAPISpec(filterType);
  }

  Serial.println("Generating fresh OpenAPI spec for filter: " +
                 String((int)filterType));
  // Create the OpenAPI document
  DynamicJsonDocument doc(65536); // Increased size for enhanced documentation

  // Basic OpenAPI info
  doc["openapi"] = "3.0.0";

  JsonObject info = doc.createNestedObject("info");
  info["title"] = String(deviceName) + " API";
  info["description"] = "RESTful API endpoints for " + String(deviceName) +
                        ". Only routes containing '/api/' are documented here.";
  info["version"] = "1.0.0";

  // Server information
  JsonArray servers = doc.createNestedArray("servers");
  JsonObject server = servers.createNestedObject();
  server["url"] = getBaseUrl();
  server["description"] = "Device API Server";

  // Security schemes
  JsonObject components = doc.createNestedObject("components");
  JsonObject securitySchemes = components.createNestedObject("securitySchemes");

  // Bearer token security
  JsonObject bearerAuth = securitySchemes.createNestedObject("bearerAuth");
  bearerAuth["type"] = "http";
  bearerAuth["scheme"] = "bearer";
  bearerAuth["bearerFormat"] = "JWT";

  // Cookie-based security for sessions
  JsonObject cookieAuth = securitySchemes.createNestedObject("cookieAuth");
  cookieAuth["type"] = "apiKey";
  cookieAuth["in"] = "cookie";
  cookieAuth["name"] = "session";

  // Paths object to hold all routes
  JsonObject paths = doc.createNestedObject("paths");

  // Convert route registry to OpenAPI paths
  for (const auto &route : routeRegistry) {
    // Skip disabled routes
    if (!route.handler) {
      continue;
    }

    // Only include API routes (paths containing /api/)
    if (route.path.indexOf("/api/") == -1) {
      continue;
    }

    // Apply filter if requested
    if (filterType != AuthType::NONE) {
      bool matchesFilter = false;
      // Check if any auth type in the requirements matches the filter
      for (size_t i = 0; i < route.authRequirements.size(); i++) {
        if (route.authRequirements[i] == filterType) {
          matchesFilter = true;
          break;
        }
      }
      if (!matchesFilter) {
        continue;
      }
    }

    // Get or create the path object
    JsonObject pathItem;
    if (paths.containsKey(route.path)) {
      pathItem = paths[route.path];
    } else {
      pathItem = paths.createNestedObject(route.path);
    }

    // Convert WebModule::Method to OpenAPI method string
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
    // Skip WM_OPTIONS and WM_HEAD as they're not defined
    default:
      methodStr = "get";
    }

    // Create operation object
    JsonObject operation = pathItem.createNestedObject(methodStr);

    // Add summary - use module-provided summary or generate default
    if (!route.summary.isEmpty()) {
      operation["summary"] = route.summary;
    } else {
      operation["summary"] = generateDefaultSummary(route.path, methodStr);
      // Log warning for missing documentation on token routes
      if (hasTokenAuth(route.authRequirements)) {
        Serial.println("WARNING: Token route " + route.path +
                       " missing documentation summary");
      }
    }

    // Use module-provided operationId or generate default
    String operationId;
    if (!route.operationId.isEmpty()) {
      operationId = route.operationId;
    } else {
      operationId = generateOperationId(methodStr, route.path);
    }
    operation["operationId"] = operationId;

    // Add tags - use module-provided tags or infer from module
    JsonArray tags = operation.createNestedArray("tags");

    // Get the default module tag from the path
    String defaultModuleTag = inferModuleFromPath(route.path);

    if (!route.tags.isEmpty()) {
      // Always add the module tag first
      tags.add(defaultModuleTag);

      // Parse comma-separated tags provided by the module
      String tagsCopy = route.tags;
      String lowerDefaultTag = defaultModuleTag;
      lowerDefaultTag.toLowerCase();

      while (tagsCopy.length() > 0) {
        int commaIndex = tagsCopy.indexOf(',');
        String tag =
            (commaIndex != -1) ? tagsCopy.substring(0, commaIndex) : tagsCopy;
        tag.trim();
        if (tag.length() > 0) {
          // Skip if this tag is the same as the module tag (case-insensitive)
          String lowerTag = tag;
          lowerTag.toLowerCase();
          if (lowerTag != lowerDefaultTag) {
            tags.add(tag);
          }
        }
        tagsCopy = (commaIndex != -1) ? tagsCopy.substring(commaIndex + 1) : "";
      }
    } else {
      // No explicit tags provided, use the inferred module tag
      tags.add(defaultModuleTag);
    }

    // Add security requirements
    if (route.authRequirements.size() > 0) {
      JsonArray security = operation.createNestedArray("security");

      for (size_t i = 0; i < route.authRequirements.size(); i++) {
        AuthType authType = route.authRequirements[i];
        if (authType == AuthType::TOKEN) {
          JsonObject secObj = security.createNestedObject();
          secObj.createNestedArray("bearerAuth");
        } else if (authType == AuthType::SESSION) {
          JsonObject secObj = security.createNestedObject();
          secObj.createNestedArray("cookieAuth");
        }
      }
    }

    // Add parameters - enhanced with module-provided info
    addParametersToOperation(operation, route);

    // Add responses with examples
    addResponsesToOperation(operation, route);

    // Add request body if POST/PUT/PATCH with examples
    if (methodStr == "post" || methodStr == "put" || methodStr == "patch") {
      addRequestBodyToOperation(operation, route);
    }
  }

  // Before serializing, check document size
  size_t estimatedSize = measureJson(doc);
  Serial.println("Estimated OpenAPI JSON size: " + String(estimatedSize) +
                 " bytes");

  if (estimatedSize > doc.capacity() * 0.9) {
    Serial.println("WARNING: OpenAPI document very close to capacity limit!");
    // Consider simplifying the output
    if (paths.size() > 20) {
      Serial.println("Too many paths, limiting to essential ones only");
      // Implement simplification logic here
    }
  }

  // Serialize with monitoring
  String openApiJson;
  serializeJson(doc, openApiJson);

  Serial.println("Generated OpenAPI JSON: " + String(openApiJson.length()) +
                 " bytes");

  // Verify serialization was complete (simple validation)
  if (!openApiJson.endsWith("}")) {
    Serial.println("ERROR: OpenAPI JSON appears to be truncated!");
  }

  // Cache only if not truncated and within reasonable size
  if (useCache && openApiJson.endsWith("}") && openApiJson.length() < 50000) {

    if (!openApiCacheInitialized) {
      const_cast<WebPlatform *>(this)->initializeOpenAPICache();
    }
    const_cast<WebPlatform *>(this)->openApiCache[(int)filterType] =
        openApiJson;

    // Also save to persistent storage
    String key = String((int)filterType);
    StorageManager::query(OPENAPI_CACHE_COLLECTION).store(key, openApiJson);

    Serial.println(
        "Cached OpenAPI spec for filter: " + String((int)filterType) + " (" +
        String(openApiJson.length()) + " bytes)");
  }

  return openApiJson;
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
  String bestMatch = "";
  int bestMatchLength = 0;

  for (const auto &regModule : registeredModules) {
    if (path.startsWith(regModule.basePath) &&
        static_cast<int>(regModule.basePath.length()) > bestMatchLength) {
      bestMatch = regModule.module->getModuleName();
      bestMatchLength = regModule.basePath.length();
    }
  }

  // If we found a registered module, format its name
  if (!bestMatch.isEmpty()) {
    return formatModuleName(bestMatch);
  }

  // Fallback: determine module based on common path prefixes
  // For external modules (not part of WebPlatform itself)
  if (path.startsWith("/maker_api")) {
    return "Maker API";
  } else if (path.startsWith("/sensors")) {
    return "Environmental Sensor";
  } else if (path.startsWith("/usb_pd")) {
    return "USB PD Control";
  }
  // All WebPlatform internal routes (including auth routes) should return "Web
  // Platform" The specific functional tags like "User Management" will be added
  // as explicit tags
  else {
    return "Web Platform";
  }
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
  for (size_t i = 0; i < requirements.size(); i++) {
    if (requirements[i] == AuthType::TOKEN) {
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
  if (!route.parameters.isEmpty()) {
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
  if (route.path.indexOf("{") != -1) {
    String pathCopy = route.path;
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
          if (!route.parameterConstraints.isEmpty()) {
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
  String contentType =
      route.contentType.isEmpty() ? "application/json" : route.contentType;
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add response schema if provided
  if (!route.responseSchema.isEmpty()) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, route.responseSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add response example if provided
  if (!route.responseExample.isEmpty()) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, route.responseExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Add module-provided response info
  if (!route.responseInfo.isEmpty()) {
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
  if (route.authRequirements.size() > 0) {
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
  String contentType =
      route.contentType.isEmpty() ? "application/json" : route.contentType;
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add request schema if provided
  if (!route.requestSchema.isEmpty()) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, route.requestSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add request example if provided
  if (!route.requestExample.isEmpty()) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, route.requestExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Make request body required for POST/PUT operations by default
  requestBody["required"] = true;
}
