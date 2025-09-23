#include "interface/openapi_types.h"
#include "interface/web_module_types.h"
#include "route_entry.h"
#include "storage/storage_manager.h"
#include "web_platform.h"
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
#if !OPENAPI_ENABLED
  DEBUG_PRINTLN("WebPlatform: OpenAPI generation disabled at compile time");
  openAPISpecReady = false;
  return;
#endif

  DEBUG_PRINTLN("WebPlatform: Generating OpenAPI specification to storage "
                "using temporary context...");

  IDatabaseDriver *driver = StorageManager::driver();

  size_t freeHeap = ESP.getFreeHeap();
  size_t targetSize;
  size_t maxBlock = ESP.getMaxAllocHeap();

  // CRITICAL FIX: Increase target size for 23 API routes
  // Based on validation results, 32KB is nearly full, increase to 40KB
  size_t maxAllowable = (size_t)(maxBlock * 0.7);
  targetSize = (maxAllowable < 40960) ? maxAllowable
                                      : 40960; // Increase from 32KB to 40KB

  if (targetSize < 16384) { // Increase minimum from 8KB to 16KB
    ERROR_PRINTLN("ERROR: Insufficient memory for OpenAPI generation!");
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

  // Process routes from temporary storage instead of routeRegistry
  const auto &apiRoutes = openAPIGenerationContext.getApiRoutes();

  DEBUG_PRINTF("WebPlatform: OpenAPI generation found %d routes in context\n",
               apiRoutes.size());

  // Process API routes from temporary storage
  int processedCount = 0;
  for (const auto &routeDoc : openAPIGenerationContext.getApiRoutes()) {
    String routePathStr = routeDoc.path;

    // Skip non-API routes (should already be filtered but double-check)
    if (routePathStr.indexOf("/api/") == -1) {
      DEBUG_PRINTF("  Skipping non-API route: %s\n", routePathStr.c_str());
      continue;
    }

    processedCount++;

    // Ensure proper path key and method string
    String pathKey = routeDoc.path;
    String methodStr = wmMethodToString(routeDoc.method);
    methodStr.toLowerCase(); // Ensure lowercase method names (get, post, put,
                             // delete)

    JsonObject pathItem;
    if (paths.containsKey(pathKey)) {
      pathItem = paths[pathKey];
    } else {
      pathItem = paths.createNestedObject(pathKey);
    }

    JsonObject operation = pathItem.createNestedObject(methodStr);

    // Use documentation from temporary storage or generate defaults
    const OpenAPIDocumentation &docs = routeDoc.docs;

#if OPENAPI_ENABLED
    if (!docs.summary.isEmpty()) {
      operation["summary"] = docs.summary;
    } else {
      operation["summary"] = generateDefaultSummary(routePathStr, methodStr);
    }

    if (!docs.operationId.isEmpty()) {
      operation["operationId"] = docs.operationId;
    } else {
      operation["operationId"] = generateOperationId(methodStr, routePathStr);
    }

    if (!docs.description.isEmpty()) {
      operation["description"] = docs.description;
    }

    // Handle tags - use provided tags or generate default (restored original
    // logic)
    JsonArray tags = operation.createNestedArray("tags");
    String defaultModuleTag = inferModuleFromPath(routePathStr);

    if (!docs.tags.empty()) {
      // Add default module tag first
      tags.add(defaultModuleTag);

      // Add custom tags, avoiding duplicates
      String lowerDefaultTag = defaultModuleTag;
      lowerDefaultTag.toLowerCase();

      for (const String &tag : docs.tags) {
        String lowerTag = tag;
        lowerTag.toLowerCase();
        if (lowerTag != lowerDefaultTag) {
          tags.add(tag);
        }
      }
    } else {
      // Just add the default module tag
      tags.add(defaultModuleTag);
    }
#else
    // When OpenAPI is disabled, just generate basic operation info
    operation["summary"] = generateDefaultSummary(routePathStr, methodStr);
    operation["operationId"] = generateOperationId(methodStr, routePathStr);

    JsonArray tags = operation.createNestedArray("tags");
    tags.add(inferModuleFromPath(routePathStr));
#endif

    // Add authentication requirements
    if (!routeDoc.authRequirements.empty()) {
      JsonArray security = operation.createNestedArray("security");
      for (const auto &authType : routeDoc.authRequirements) {
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

    // Add parameters using the helper method to avoid duplication
    addParametersToOperationFromDocs(operation, routeDoc);

    // Add request body for POST/PUT operations
#if OPENAPI_ENABLED
    if ((routeDoc.method == WebModule::WM_POST ||
         routeDoc.method == WebModule::WM_PUT) &&
        (!docs.requestSchema.isEmpty() || !docs.requestExample.isEmpty())) {
      addRequestBodyToOperationFromDocs(operation, routeDoc);
    }
#else
    if (routeDoc.method == WebModule::WM_POST ||
        routeDoc.method == WebModule::WM_PUT) {
      addRequestBodyToOperationFromDocs(operation, routeDoc);
    }
#endif

    // Add responses
#if OPENAPI_ENABLED
    if (!docs.responsesJson.isEmpty() || !docs.responseSchema.isEmpty() ||
        !docs.responseExample.isEmpty()) {
      addResponsesToOperationFromDocs(operation, routeDoc);
    } else {
#else
    {
#endif
      // Add basic responses
      JsonObject responses = operation.createNestedObject("responses");
      JsonObject response200 = responses.createNestedObject("200");
      response200["description"] = "Successful operation";

      // Add auth error responses if needed
      if (!routeDoc.authRequirements.empty()) {
        JsonObject response401 = responses.createNestedObject("401");
        response401["description"] = "Unauthorized - Authentication required";
        JsonObject response403 = responses.createNestedObject("403");
        response403["description"] = "Forbidden - Insufficient permissions";
      }

      JsonObject response500 = responses.createNestedObject("500");
      response500["description"] = "Internal server error";
    }

    // Check if we're running low on memory
    if (doc.memoryUsage() > doc.capacity() * 0.9) {
      WARN_PRINTF(
          "WARNING: JSON document nearly full at route #%d (%d/%d bytes)\n",
          processedCount, doc.memoryUsage(), doc.capacity());
    }
  }

  JsonObject pathsDebug = doc["paths"];
  int totalPaths = 0;
  int totalOperations = 0;

  for (JsonPair pathPair : pathsDebug) {
    totalPaths++;
    JsonObject pathObj = pathPair.value();
    for (JsonPair methodPair : pathObj) {
      totalOperations++;
    }
  }

  // Serialize to string
  String openAPIJson;
  size_t estimatedSize = measureJson(doc);
  openAPIJson.reserve(estimatedSize + 256);

  size_t bytesWritten = serializeJson(doc, openAPIJson);

  if (bytesWritten == 0 || openAPIJson.length() == 0) {
    ERROR_PRINTLN("ERROR: Failed to serialize OpenAPI spec");
    openAPISpecReady = false;
    return;
  }

  // Store in storage system
  if (driver &&
      driver->store(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY, openAPIJson)) {
    openAPISpecReady = true;
    DEBUG_PRINTF("WebPlatform: OpenAPI spec generated and stored (%d bytes)\n",
                 openAPIJson.length());
  } else {
    ERROR_PRINTLN("ERROR: Failed to store OpenAPI spec in storage system");
    openAPISpecReady = false;
  }

  // Critical cleanup - free temporary storage
  openAPIGenerationContext.endGeneration();

#if !OPENAPI_ENABLED
  // This code should not be reached when disabled, but safety check
  ERROR_PRINTLN("ERROR: OpenAPI generation code executed when disabled!");
#endif

  // Clear the temporary string to free memory
  openAPIJson = "";
}

void WebPlatform::streamPreGeneratedOpenAPISpec(WebResponse &res) const {
#if !OPENAPI_ENABLED
  res.setStatus(501);
  res.setContent("{\"error\":\"OpenAPI specification generation disabled\"}",
                 "application/json");
  return;
#else
  DEBUG_PRINTF("WebPlatform: OpenAPI spec request - ready flag: %s\n",
               openAPISpecReady ? "true" : "false");

  if (!openAPISpecReady) {
    res.setStatus(503);
    res.setContent("{\"error\":\"OpenAPI specification not ready\"}",
                   "application/json");
    return;
  }

  // Retrieve from storage system
  IDatabaseDriver *driver = StorageManager::driver();
  if (!driver) {
    ERROR_PRINTLN("WebPlatform: Storage driver unavailable for OpenAPI spec");
    res.setStatus(500);
    res.setContent("{\"error\":\"Storage system unavailable\"}",
                   "application/json");
    return;
  }

  DEBUG_PRINTF("WebPlatform: Attempting to retrieve OpenAPI spec from storage "
               "(collection: %s, key: %s)\n",
               OPENAPI_COLLECTION.c_str(), OPENAPI_SPEC_KEY.c_str());

  String openAPISpec = driver->retrieve(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY);

  if (openAPISpec.isEmpty()) {
    ERROR_PRINTF("WebPlatform: OpenAPI spec not found in storage! Collection "
                 "exists: %s\n",
                 driver->exists(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY)
                     ? "true"
                     : "false");

    // Try to regenerate the spec on-demand if it's missing
    DEBUG_PRINTLN("WebPlatform: Attempting to regenerate OpenAPI spec...");
    const_cast<WebPlatform *>(this)->generateOpenAPISpec();

    // Try again after regeneration
    openAPISpec = driver->retrieve(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY);
    if (openAPISpec.isEmpty()) {
      ERROR_PRINTLN("WebPlatform: Failed to regenerate OpenAPI spec");
      res.setStatus(404);
      res.setContent("{\"error\":\"OpenAPI specification not found in storage "
                     "and regeneration failed\"}",
                     "application/json");
      return;
    }

    DEBUG_PRINTF(
        "WebPlatform: Successfully regenerated OpenAPI spec (%d bytes)\n",
        openAPISpec.length());
  }

  DEBUG_PRINTLN("WebPlatform: Serving OpenAPI spec using storage streaming");

  res.setStatus(200);
  res.setHeader("Cache-Control", "public, max-age=300");

  // Use the new storage streaming feature instead of loading entire spec into
  // memory
  res.setStorageStreamContent(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY,
                              "application/json");
#endif // OPENAPI_ENABLED
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

void WebPlatform::addParametersToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const {
#if OPENAPI_ENABLED
  JsonArray parameters = operation.createNestedArray("parameters");
  const OpenAPIDocumentation &docs = routeDoc.docs;

  // Track parameter names to avoid duplicates
  std::map<String, bool> parameterNames;

  // First, add custom parameters from module documentation
  if (!docs.parameters.isEmpty()) {
    DynamicJsonDocument paramDoc(2048);
    if (deserializeJson(paramDoc, docs.parameters) ==
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

  // Add auto-generated path parameters
  String routePathStr = routeDoc.path;
  if (routePathStr.indexOf("{") != -1) {
    String pathCopy = routePathStr;
    int startPos = 0;
    while ((startPos = pathCopy.indexOf("{", startPos)) != -1) {
      int endPos = pathCopy.indexOf("}", startPos);
      if (endPos != -1) {
        String paramName = pathCopy.substring(startPos + 1, endPos);
        String paramKey = paramName + ":path";

        if (parameterNames.find(paramKey) == parameterNames.end()) {
          JsonObject param = parameters.createNestedObject();
          param["name"] = paramName;
          param["in"] = "path";
          param["required"] = true;

          // Enhanced parameter descriptions
          if (paramName == "id") {
            param["description"] = "Resource identifier";
          } else if (paramName == "userId") {
            param["description"] = "User identifier (UUID)";
          } else if (paramName == "tokenId") {
            param["description"] = "Token identifier";
          } else {
            param["description"] = "Path parameter: " + paramName;
          }

          JsonObject schema = param.createNestedObject("schema");
          if (paramName.endsWith("Id") && paramName != "id") {
            schema["type"] = "string";
            schema["format"] = "uuid";
          } else {
            schema["type"] = "string";
          }

          parameterNames[paramKey] = true;
        }

        startPos = endPos + 1;
      } else {
        break;
      }
    }
  }

  // Add access_token parameter for routes that support token authentication
  bool hasTokenAuth = false;
  for (const auto &authType : routeDoc.authRequirements) {
    if (authType == AuthType::TOKEN) {
      hasTokenAuth = true;
      break;
    }
  }

  if (hasTokenAuth &&
      parameterNames.find("access_token:query") == parameterNames.end()) {
    JsonObject tokenParam = parameters.createNestedObject();
    tokenParam["name"] = "access_token";
    tokenParam["in"] = "query";
    tokenParam["required"] = false;
    tokenParam["description"] =
        "API access token (alternative to Bearer header)";
    JsonObject tokenSchema = tokenParam.createNestedObject("schema");
    tokenSchema["type"] = "string";
    parameterNames["access_token:query"] = true;
  }
#endif
}

void WebPlatform::addResponsesToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const {
#if OPENAPI_ENABLED
  JsonObject responses = operation.createNestedObject("responses");
  const OpenAPIDocumentation &docs = routeDoc.docs;

  // Success response
  JsonObject response200 = responses.createNestedObject("200");
  response200["description"] = "Successful operation";

  // Add content type and examples
  JsonObject content = response200.createNestedObject("content");
  String contentType = "application/json"; // Default content type
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add response schema if provided
  if (!docs.responseSchema.isEmpty()) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, docs.responseSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add response example if provided
  if (!docs.responseExample.isEmpty()) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, docs.responseExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Add module-provided response info
  if (!docs.responsesJson.isEmpty()) {
    DynamicJsonDocument responseDoc(2048);
    if (deserializeJson(responseDoc, docs.responsesJson) ==
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
  if (!routeDoc.authRequirements.empty()) {
    JsonObject response401 = responses.createNestedObject("401");
    response401["description"] = "Unauthorized - Authentication required";

    JsonObject response403 = responses.createNestedObject("403");
    response403["description"] = "Forbidden - Insufficient permissions";
  }

  JsonObject response500 = responses.createNestedObject("500");
  response500["description"] = "Internal server error";

#endif
}

void WebPlatform::addRequestBodyToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const {
#if OPENAPI_ENABLED
  JsonObject requestBody = operation.createNestedObject("requestBody");
  requestBody["description"] = "Request payload";
  const OpenAPIDocumentation &docs = routeDoc.docs;

  JsonObject content = requestBody.createNestedObject("content");
  String contentType = "application/json"; // Default content type
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add request schema if provided
  if (!docs.requestSchema.isEmpty()) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, docs.requestSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add request example if provided
  if (!docs.requestExample.isEmpty()) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, docs.requestExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Make request body required for POST/PUT operations by default
  requestBody["required"] = true;
#endif
}