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
const String WebPlatform::MAKER_OPENAPI_SPEC_KEY = "maker";

// Helper function to check if a route should be included in Maker API
bool WebPlatform::isMakerAPIRoute(
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) const {
  // Check if route has any of the maker tags
  for (const String &routeTag : routeDoc.docs.getTags()) {
    for (const String &makerTag : makerApiTags) {
      if (routeTag.equalsIgnoreCase(makerTag)) {
        return true;
      }
    }
  }

  return false;
}

// Helper function to create basic OpenAPI document structure
void WebPlatform::createOpenAPIDocumentStructure(
    DynamicJsonDocument &doc, const String &title,
    const String &description) const {
  doc["openapi"] = "3.0.3";

  JsonObject info = doc.createNestedObject("info");
  info["title"] = title;
  info["description"] = description;
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
}

// Helper function to generate and store a spec
bool WebPlatform::generateAndStoreSpec(
    size_t targetSize, const String &title, const String &description,
    std::function<bool(const OpenAPIGenerationContext::RouteDocumentation &)>
        routeFilter,
    std::function<void(JsonArray &,
                       const OpenAPIGenerationContext::RouteDocumentation &)>
        tagModifier,
    const String &storageKey, const String &specType) {

  DynamicJsonDocument doc(targetSize);
  createOpenAPIDocumentStructure(doc, title, description);

  JsonObject paths = doc.createNestedObject("paths");

  const auto &apiRoutes = openAPIGenerationContext.getApiRoutes();
  DEBUG_PRINTF("WebPlatform: %s generation found %d routes in context\n",
               specType.c_str(), apiRoutes.size());

  int processedCount = 0;
  for (const auto &routeDoc : apiRoutes) {
    // Apply route filter
    if (!routeFilter(routeDoc)) {
      continue;
    }

    String routePathStr = routeDoc.path;
    processedCount++;

    // Ensure proper path key and method string
    String pathKey = routeDoc.path;
    String methodStr = wmMethodToString(routeDoc.method);
    methodStr.toLowerCase();

    JsonObject pathItem;
    if (paths.containsKey(pathKey)) {
      pathItem = paths[pathKey];
    } else {
      pathItem = paths.createNestedObject(pathKey);
    }

    JsonObject operation = pathItem.createNestedObject(methodStr);
    const OpenAPIDocumentation &docs = routeDoc.docs;

    // Add basic operation properties
#if OPENAPI_ENABLED
    if (!docs.getSummary().isEmpty()) {
      operation["summary"] = docs.getSummary();
    } else {
      operation["summary"] = generateDefaultSummary(routePathStr, methodStr);
    }

    if (!docs.getOperationId().isEmpty()) {
      operation["operationId"] = docs.getOperationId();
    } else {
      operation["operationId"] = generateOperationId(methodStr, routePathStr);
    }

    if (!docs.getDescription().isEmpty()) {
      operation["description"] = docs.getDescription();
    }

    // Handle tags using the provided modifier
    JsonArray tags = operation.createNestedArray("tags");
    String defaultModuleTag = inferModuleFromPath(routePathStr);

    if (!docs.getTags().empty()) {
      // Add default module tag first
      tags.add(defaultModuleTag);

      // Add custom tags, avoiding duplicates
      String lowerDefaultTag = defaultModuleTag;
      lowerDefaultTag.toLowerCase();

      for (const String &tag : docs.getTags()) {
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

    // Apply custom tag modification (e.g., for Maker API)
    tagModifier(tags, routeDoc);
#else
    // When OpenAPI is disabled, just generate basic operation info
    operation["summary"] = generateDefaultSummary(routePathStr, methodStr);
    operation["operationId"] = generateOperationId(methodStr, routePathStr);

    JsonArray tags = operation.createNestedArray("tags");
    tags.add(inferModuleFromPath(routePathStr));
    tagModifier(tags, routeDoc);
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

    // Add parameters, request body, and responses
    addParametersToOperationFromDocs(operation, routeDoc);

    // Add request body for POST/PUT operations
#if OPENAPI_ENABLED
    if ((routeDoc.method == WebModule::WM_POST ||
         routeDoc.method == WebModule::WM_PUT) &&
        (!docs.getRequestSchema().isEmpty() ||
         !docs.getRequestExample().isEmpty())) {
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
    if (!docs.getResponsesJson().isEmpty() ||
        !docs.getResponseSchema().isEmpty() ||
        !docs.getResponseExample().isEmpty()) {
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

    // Check if we're running low on memory - only warn after processing some
    // routes and if we have less than 10% capacity remaining
    if (processedCount > 2 && doc.memoryUsage() > doc.capacity() * 0.9) {
      size_t remainingBytes = doc.capacity() - doc.memoryUsage();
      WARN_PRINTF("WARNING: %s JSON document nearly full at route #%d (%d/%d "
                  "bytes, %d remaining)\n",
                  specType.c_str(), processedCount, doc.memoryUsage(),
                  doc.capacity(), remainingBytes);

      // Consider breaking early if we're truly out of space
      if (remainingBytes < 1024) {
        WARN_PRINTF("WARNING: Breaking early due to insufficient memory for "
                    "more routes\n");
        break;
      }
    }
  }

  // Serialize to string
  String openAPIJson;
  size_t estimatedSize = measureJson(doc);
  openAPIJson.reserve(estimatedSize + 256);

  size_t bytesWritten = serializeJson(doc, openAPIJson);

  // Critical cleanup - clear the JSON document immediately after serialization
  doc.clear();

  if (bytesWritten == 0 || openAPIJson.length() == 0) {
    ERROR_PRINTF("ERROR: Failed to serialize %s spec\n", openAPIJson.c_str());
    openAPIJson = "";
    return false;
  }

  // Store in storage system with enhanced diagnostics
  IDatabaseDriver *driver = StorageManager::driver("littlefs");
  if (!driver) {
    ERROR_PRINTF("ERROR: LittleFS storage driver unavailable for %s spec\n",
                 specType.c_str());
    openAPIJson = "";
    return false;
  }

  DEBUG_PRINTF("WebPlatform: Attempting to store %s spec: collection='%s', "
               "key='%s', size=%d bytes\n",
               specType.c_str(), OPENAPI_COLLECTION.c_str(), storageKey.c_str(),
               openAPIJson.length());

  // Check available heap before storage
  size_t heapBefore = ESP.getFreeHeap();
  DEBUG_PRINTF("WebPlatform: Free heap before storage: %d bytes\n", heapBefore);

  bool storeResult = driver->store(OPENAPI_COLLECTION, storageKey, openAPIJson);
  size_t heapAfter = ESP.getFreeHeap();

  DEBUG_PRINTF("WebPlatform: Storage operation result: %s, heap after: %d "
               "bytes (change: %d)\n",
               storeResult ? "SUCCESS" : "FAILED", heapAfter,
               (int)heapAfter - (int)heapBefore);

  if (storeResult) {
    // Immediately verify the storage worked by trying to retrieve
    // But only do verification if we have sufficient heap
    if (ESP.getFreeHeap() > openAPIJson.length() + 8192) {
      String verifyRetrieve = driver->retrieve(OPENAPI_COLLECTION, storageKey);
      bool verifyExists = driver->exists(OPENAPI_COLLECTION, storageKey);

      DEBUG_PRINTF("WebPlatform: Storage verification - exists: %s, retrieved "
                   "size: %d bytes\n",
                   verifyExists ? "true" : "false", verifyRetrieve.length());

      if (verifyRetrieve.isEmpty() ||
          verifyRetrieve.length() != openAPIJson.length()) {
        ERROR_PRINTF("ERROR: %s spec storage verification failed! Stored %d "
                     "bytes but retrieved %d bytes\n",
                     specType.c_str(), openAPIJson.length(),
                     verifyRetrieve.length());

        // Clean up and return failure
        openAPIJson = "";
        verifyRetrieve = "";
        return false;
      }

      // Clean up verification string immediately
      verifyRetrieve = "";
    } else {
      WARN_PRINTF(
          "WARNING: Skipping storage verification due to low heap (%d bytes)\n",
          ESP.getFreeHeap());

      // Just check existence without retrieving content
      if (!driver->exists(OPENAPI_COLLECTION, storageKey)) {
        ERROR_PRINTF("ERROR: %s spec storage failed - document doesn't exist "
                     "after store\n",
                     specType.c_str());
        openAPIJson = "";
        return false;
      }
    }

    DEBUG_PRINTF("WebPlatform: %s spec successfully stored and verified (%d "
                 "bytes, %d routes)\n",
                 specType.c_str(), openAPIJson.length(), processedCount);

    // Critical cleanup - free temporary string immediately
    openAPIJson = "";
    return true;
  } else {
    ERROR_PRINTF("ERROR: Failed to store %s spec in storage system (heap "
                 "before: %d, after: %d)\n",
                 specType.c_str(), heapBefore, heapAfter);

    // Critical cleanup - free temporary string immediately
    openAPIJson = "";
    return false;
  }
}

void WebPlatform::generateOpenAPISpec() {
  // Skip full OpenAPI if it's disabled but still continue for Maker API if
  // enabled
#if !OPENAPI_ENABLED
  DEBUG_PRINTLN("WebPlatform: Full OpenAPI generation disabled, checking for "
                "Maker API...");
  openAPISpecReady = false;

// If neither is enabled, there's nothing to do
#if !MAKERAPI_ENABLED
  DEBUG_PRINTLN("WebPlatform: Maker API also disabled, skipping generation");
  makerAPISpecReady = false;
  return;
#endif
#endif

  DEBUG_PRINTLN("WebPlatform: Generating OpenAPI specification to storage "
                "using temporary context...");

  IDatabaseDriver *driver = StorageManager::driver("littlefs");
  if (!driver) {
    ERROR_PRINTLN(
        "ERROR: LittleFS storage driver unavailable for OpenAPI generation!");
    openAPISpecReady = false;
    makerAPISpecReady = false;
    return;
  }

  size_t freeHeap = ESP.getFreeHeap();
  size_t maxBlock = ESP.getMaxAllocHeap();
  size_t maxAllowable = (size_t)(maxBlock * 0.7);
  size_t targetSize = (maxAllowable < 40960) ? maxAllowable : 40960;

  if (targetSize < 16384) {
    ERROR_PRINTLN("ERROR: Insufficient memory for OpenAPI generation!");
    openAPISpecReady = false;
    makerAPISpecReady = false;
    return;
  }

#if OPENAPI_ENABLED
  // Generate full OpenAPI spec
  auto allRoutesFilter =
      [](const OpenAPIGenerationContext::RouteDocumentation &routeDoc) -> bool {
    // Skip non-API routes
    return routeDoc.path.indexOf("/api/") != -1;
  };

  auto defaultTagModifier =
      [](JsonArray &tags,
         const OpenAPIGenerationContext::RouteDocumentation &routeDoc) {
        // No additional tag modification for full API
      };

  openAPISpecReady = generateAndStoreSpec(
      targetSize, String(deviceName) + " API",
      "RESTful API endpoints for " + String(deviceName) + ".", allRoutesFilter,
      defaultTagModifier, OPENAPI_SPEC_KEY, "OpenAPI");
#else
  openAPISpecReady = false;
#endif

#if MAKERAPI_ENABLED
  // Generate Maker API spec - check if we have any maker routes first
  bool hasMakerRoutes = false;
  for (const auto &routeDoc : openAPIGenerationContext.getApiRoutes()) {
    if (isMakerAPIRoute(routeDoc)) {
      hasMakerRoutes = true;
      break;
    }
  }

  if (!hasMakerRoutes) {
    DEBUG_PRINTLN("WebPlatform: No routes with 'maker' tag found, skipping "
                  "Maker API spec");
    makerAPISpecReady = false;
  } else {
    DEBUG_PRINTLN("WebPlatform: Generating Maker API OpenAPI specification...");

    auto makerRoutesFilter =
        [this](const OpenAPIGenerationContext::RouteDocumentation &routeDoc)
        -> bool { return isMakerAPIRoute(routeDoc); };

    auto makerTagModifier =
        [](JsonArray &tags,
           const OpenAPIGenerationContext::RouteDocumentation &routeDoc) {
          // Just clear and rebuild with Maker API tag first - matches original
          // simple logic
          String moduleTag = "";
          if (tags.size() > 0) {
            moduleTag = tags[0].as<String>();
          }
          tags.clear();
          tags.add("Maker API");
          if (!moduleTag.isEmpty() && moduleTag != "Maker API") {
            tags.add(moduleTag);
          }
        };

    // Use appropriate size for Maker API - it should be smaller than main API
    // but not tiny
    size_t makerTargetSize = std::min(maxAllowable, targetSize / 2);
    if (makerTargetSize < 24576)
      makerTargetSize = 24576; // Minimum 24KB

    makerAPISpecReady = generateAndStoreSpec(
        makerTargetSize, String(deviceName) + " Maker API",
        "Public Maker API endpoints for " + String(deviceName) + ".",
        makerRoutesFilter, makerTagModifier, MAKER_OPENAPI_SPEC_KEY,
        "Maker API");
  }
#else
  makerAPISpecReady = false;
#endif

  // Critical cleanup - free temporary storage
  openAPIGenerationContext.endGeneration();
}

void WebPlatform::streamPreGeneratedOpenAPISpec(WebResponse &res) const {
#if !OPENAPI_ENABLED
  res.setStatus(501);
  res.setContent("{\"error\":\"OpenAPI specification generation disabled\"}",
                 "application/json");
  return;
#else
  // Even with OpenAPI enabled, we need to make sure we're not in the process of
  // generation
  DEBUG_PRINTF("WebPlatform: OpenAPI spec request - ready flag: %s\n",
               openAPISpecReady ? "true" : "false");

  if (!openAPISpecReady) {
    res.setStatus(503);
    res.setContent("{\"error\":\"OpenAPI specification not ready\"}",
                   "application/json");
    return;
  }

  // Retrieve from storage system
  IDatabaseDriver *driver = StorageManager::driver("littlefs");
  if (!driver) {
    ERROR_PRINTLN(
        "WebPlatform: LittleFS storage driver unavailable for OpenAPI spec");
    res.setStatus(500);
    res.setContent("{\"error\":\"LittleFS storage system unavailable\"}",
                   "application/json");
    return;
  }

  DEBUG_PRINTF("WebPlatform: Attempting to retrieve OpenAPI spec from storage "
               "(collection: %s, key: %s)\n",
               OPENAPI_COLLECTION.c_str(), OPENAPI_SPEC_KEY.c_str());

  DEBUG_PRINTF(
      "WebPlatform: Retrieving OpenAPI spec - collection='%s', key='%s'\n",
      OPENAPI_COLLECTION.c_str(), OPENAPI_SPEC_KEY.c_str());

  size_t heapBefore = ESP.getFreeHeap();
  // Check if the spec exists in storage without loading it
  if (!driver->exists(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY)) {
    ERROR_PRINTF("WebPlatform: OpenAPI spec not found in storage!\n");
    res.setStatus(404);
    res.setContent(
        "{\"error\":\"OpenAPI specification not found in storage.\"}",
        "application/json");
    return;
  }

  DEBUG_PRINTLN(
      "WebPlatform: Serving OpenAPI spec using direct storage streaming");

  res.setStatus(200);
  res.setHeader("Cache-Control", "public, max-age=300");

  // Stream directly from storage without loading into memory
  res.setStorageStreamContent(OPENAPI_COLLECTION, OPENAPI_SPEC_KEY,
                              "application/json");
#endif // OPENAPI_ENABLED
}

void WebPlatform::streamPreGeneratedMakerAPISpec(WebResponse &res) const {
#if !MAKERAPI_ENABLED
  res.setStatus(501);
  res.setContent("{\"error\":\"Maker API specification generation disabled\"}",
                 "application/json");
  return;
#else
  // Check if we have any Maker API routes
  if (!makerAPISpecReady) {
    res.setStatus(503);
    res.setContent("{\"error\":\"Maker API specification not ready\"}",
                   "application/json");
    return;
  }

  // Retrieve from storage system
  IDatabaseDriver *driver = StorageManager::driver("littlefs");
  if (!driver) {
    ERROR_PRINTLN(
        "WebPlatform: LittleFS storage driver unavailable for Maker API spec");
    res.setStatus(500);
    res.setContent("{\"error\":\"Storage system unavailable\"}",
                   "application/json");
    return;
  }

  DEBUG_PRINTF(
      "WebPlatform: Attempting to retrieve Maker API spec from storage "
      "(collection: %s, key: %s)\n",
      OPENAPI_COLLECTION.c_str(), MAKER_OPENAPI_SPEC_KEY.c_str());

  // Check if the spec exists in storage without loading it
  if (!driver->exists(OPENAPI_COLLECTION, MAKER_OPENAPI_SPEC_KEY)) {
    ERROR_PRINTF("WebPlatform: Maker API spec not found in storage!\n");
    res.setStatus(404);
    res.setContent(
        "{\"error\":\"Maker API specification not found in storage.\"}",
        "application/json");
    return;
  }

  DEBUG_PRINTLN(
      "WebPlatform: Serving Maker API spec using direct storage streaming");

  res.setStatus(200);
  res.setHeader("Cache-Control", "public, max-age=300");

  // Stream directly from storage without loading into memory
  res.setStorageStreamContent(OPENAPI_COLLECTION, MAKER_OPENAPI_SPEC_KEY,
                              "application/json");
#endif // MAKERAPI_ENABLED
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
  if (!docs.getParameters().isEmpty()) {
    DynamicJsonDocument paramDoc(2048);
    if (deserializeJson(paramDoc, docs.getParameters()) ==
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
  if (!docs.getResponseSchema().isEmpty()) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, docs.getResponseSchema()) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add response example if provided
  if (!docs.getResponseExample().isEmpty()) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, docs.getResponseExample()) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Add module-provided response info
  if (!docs.getResponsesJson().isEmpty()) {
    DynamicJsonDocument responseDoc(2048);
    if (deserializeJson(responseDoc, docs.getResponsesJson()) ==
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
  const OpenAPIDocumentation &docs = routeDoc.docs;

  // Early exit if no request documentation
  if (docs.getRequestSchema().isEmpty() && docs.getRequestExample().isEmpty()) {
    if (routeDoc.method == WebModule::WM_POST ||
        routeDoc.method == WebModule::WM_PUT) {
      JsonObject requestBody = operation.createNestedObject("requestBody");
      requestBody["description"] = "Request payload";
      requestBody["required"] = true;
      JsonObject content = requestBody.createNestedObject("content");
      JsonObject mediaType = content.createNestedObject("application/json");
      JsonObject schema = mediaType.createNestedObject("schema");
      schema["type"] = "object";
    }
    return;
  }

  // Handle request schema - use minimal buffer and immediate cleanup
  if (!docs.getRequestSchema().isEmpty()) {
    DynamicJsonDocument schemaDoc(2048); // Keep original smaller size
    DeserializationError error =
        deserializeJson(schemaDoc, docs.getRequestSchema());

    if (error == DeserializationError::Ok) {
      JsonObject schemaObj = schemaDoc.as<JsonObject>();

      // Check if this is a complete request body (has both content AND
      // required)
      bool isCompleteRequestBody =
          schemaObj.containsKey("content") && schemaObj.containsKey("required");

      JsonObject requestBody = operation.createNestedObject("requestBody");

      if (isCompleteRequestBody) {
        // Direct assignment of key properties - avoid copying entire object
        if (schemaObj.containsKey("description")) {
          requestBody["description"] = schemaObj["description"];
        } else {
          requestBody["description"] = "Request payload";
        }

        if (schemaObj.containsKey("required")) {
          requestBody["required"] = schemaObj["required"];
        }

        if (schemaObj.containsKey("content")) {
          requestBody["content"] = schemaObj["content"];
        }
      } else {
        // Simple schema object - wrap it
        requestBody["description"] = "Request payload";
        requestBody["required"] = true;
        JsonObject content = requestBody.createNestedObject("content");
        JsonObject mediaType = content.createNestedObject("application/json");
        mediaType["schema"] = schemaObj;
      }
    } else {
      // Parse failed - create minimal structure
      JsonObject requestBody = operation.createNestedObject("requestBody");
      requestBody["description"] = "Request payload";
      requestBody["required"] = true;
      JsonObject content = requestBody.createNestedObject("content");
      JsonObject mediaType = content.createNestedObject("application/json");
      JsonObject schema = mediaType.createNestedObject("schema");
      schema["type"] = "object";
    }

    // Critical: immediately clear the document to free heap
    schemaDoc.clear();

    // Handle example separately if schema was processed
    if (!docs.getRequestExample().isEmpty()) {
      // Find the media type object to add example
      JsonObject requestBody = operation["requestBody"];
      if (requestBody.containsKey("content")) {
        JsonObject content = requestBody["content"];
        for (JsonPair contentType : content) {
          JsonObject mediaType = contentType.value().as<JsonObject>();
          if (mediaType) {
            DynamicJsonDocument exampleDoc(1024); // Smaller buffer for examples
            if (deserializeJson(exampleDoc, docs.getRequestExample()) ==
                DeserializationError::Ok) {
              mediaType["example"] = exampleDoc.as<JsonVariant>();
            }
            exampleDoc.clear(); // Immediate cleanup
            break;              // Only add to first media type
          }
        }
      }
    }

  } else if (!docs.getRequestExample().isEmpty()) {
    // Only example provided - minimal structure
    JsonObject requestBody = operation.createNestedObject("requestBody");
    requestBody["description"] = "Request payload";
    requestBody["required"] = true;
    JsonObject content = requestBody.createNestedObject("content");
    JsonObject mediaType = content.createNestedObject("application/json");
    JsonObject schema = mediaType.createNestedObject("schema");
    schema["type"] = "object";

    // Add example with immediate cleanup
    DynamicJsonDocument exampleDoc(1024);
    if (deserializeJson(exampleDoc, docs.getRequestExample()) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
    exampleDoc.clear(); // Immediate cleanup
  }
#endif
}