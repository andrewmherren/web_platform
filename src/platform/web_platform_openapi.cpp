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
  for (const String &routeTag : routeDoc.docs.tags) {
    for (const String &makerTag : makerApiTags) {
      if (routeTag.equalsIgnoreCase(makerTag)) {
        return true;
      }
    }
  }

  return false;
}

void WebPlatform::generateOpenAPISpec() {
#if !OPENAPI_ENABLED && !MAKERAPI_ENABLED
  DEBUG_PRINTLN("WebPlatform: All OpenAPI generation disabled at compile time");
  openAPISpecReady = false;
  makerAPISpecReady = false;
  return;
#endif

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

  IDatabaseDriver *driver = StorageManager::driver();

  size_t freeHeap = ESP.getFreeHeap();
  size_t targetSize;
  size_t maxBlock = ESP.getMaxAllocHeap();

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

#if MAKERAPI_ENABLED
  // Now generate the Maker API spec with a smaller size JSON document
  // This runs whether or not OPENAPI_ENABLED is defined

  // Skip if no routes have the maker tag
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

    // Use a smaller document size for Maker API (it will have fewer routes)
    size_t makerTargetSize = (maxAllowable < 20480) ? maxAllowable : 20480;

    DynamicJsonDocument makerDoc(makerTargetSize);

    // Build the Maker API spec (similar structure but different content)
    makerDoc["openapi"] = "3.0.3";

    JsonObject makerInfo = makerDoc.createNestedObject("info");
    makerInfo["title"] = String(deviceName) + " Maker API";
    makerInfo["description"] =
        "Public Maker API endpoints for " + String(deviceName) +
        ". Only routes with 'maker' tag are included here.";
    makerInfo["version"] = "1.0.0";

    JsonArray makerServers = makerDoc.createNestedArray("servers");
    JsonObject makerServer = makerServers.createNestedObject();
    makerServer["url"] = getBaseUrl();
    makerServer["description"] = "Device Maker API Server";

    // Security schemes (same as main API)
    JsonObject makerComponents = makerDoc.createNestedObject("components");
    JsonObject makerSecuritySchemes =
        makerComponents.createNestedObject("securitySchemes");

    JsonObject makerBearerAuth =
        makerSecuritySchemes.createNestedObject("bearerAuth");
    makerBearerAuth["type"] = "http";
    makerBearerAuth["scheme"] = "bearer";
    makerBearerAuth["bearerFormat"] = "JWT";

    JsonObject makerTokenParam =
        makerSecuritySchemes.createNestedObject("tokenParam");
    makerTokenParam["type"] = "apiKey";
    makerTokenParam["in"] = "query";
    makerTokenParam["name"] = "access_token";

    JsonObject makerPaths = makerDoc.createNestedObject("paths");

    // Process only routes with the "maker" tag
    int makerProcessedCount = 0;

    for (const auto &routeDoc : openAPIGenerationContext.getApiRoutes()) {
      // Skip routes that aren't Maker API routes
      if (!isMakerAPIRoute(routeDoc)) {
        continue;
      }

      String routePathStr = routeDoc.path;
      makerProcessedCount++;

      // Ensure proper path key and method string
      String pathKey = routeDoc.path;
      String methodStr = wmMethodToString(routeDoc.method);
      methodStr.toLowerCase();

      JsonObject pathItem;
      if (makerPaths.containsKey(pathKey)) {
        pathItem = makerPaths[pathKey];
      } else {
        pathItem = makerPaths.createNestedObject(pathKey);
      }

      JsonObject operation = pathItem.createNestedObject(methodStr);

      // Use documentation from temporary storage
      const OpenAPIDocumentation &docs = routeDoc.docs;

      // Add basic properties
      operation["summary"] =
          docs.summary.isEmpty()
              ? generateDefaultSummary(routePathStr, methodStr)
              : docs.summary;

      operation["operationId"] =
          docs.operationId.isEmpty()
              ? generateOperationId(methodStr, routePathStr)
              : docs.operationId;

      if (!docs.description.isEmpty()) {
        operation["description"] = docs.description;
      }

      // Add tags (but keep only maker tag and module tag)
      JsonArray tags = operation.createNestedArray("tags");
      tags.add("Maker API"); // Always add Maker API tag

      String defaultModuleTag = inferModuleFromPath(routePathStr);
      if (defaultModuleTag != "Maker API") {
        tags.add(defaultModuleTag);
      }

      // Add authentication requirements
      if (!routeDoc.authRequirements.empty()) {
        JsonArray security = operation.createNestedArray("security");
        for (const auto &authType : routeDoc.authRequirements) {
          if (authType == AuthType::TOKEN) {
            JsonObject secObj = security.createNestedObject();
            secObj.createNestedArray("bearerAuth");
            JsonObject tokenSecObj = security.createNestedObject();
            tokenSecObj.createNestedArray("tokenParam");
          }
        }
      }

      // Add parameters, request body and responses using the same helpers as
      // the main API
      addParametersToOperationFromDocs(operation, routeDoc);

      if (routeDoc.method == WebModule::WM_POST ||
          routeDoc.method == WebModule::WM_PUT) {
        addRequestBodyToOperationFromDocs(operation, routeDoc);
      }

      addResponsesToOperationFromDocs(operation, routeDoc);

      // Check memory usage
      if (makerDoc.memoryUsage() > makerDoc.capacity() * 0.9) {
        WARN_PRINTF("WARNING: Maker API JSON document nearly full at route #%d "
                    "(%d/%d bytes)\n",
                    makerProcessedCount, makerDoc.memoryUsage(),
                    makerDoc.capacity());
      }
    }

    // Serialize the Maker API spec to string
    String makerAPIJson;
    size_t makerEstimatedSize = measureJson(makerDoc);
    makerAPIJson.reserve(makerEstimatedSize + 256);

    size_t makerBytesWritten = serializeJson(makerDoc, makerAPIJson);

    if (makerBytesWritten == 0 || makerAPIJson.length() == 0) {
      ERROR_PRINTLN("ERROR: Failed to serialize Maker API spec");
      makerAPISpecReady = false;
    } else if (driver && driver->store(OPENAPI_COLLECTION,
                                       MAKER_OPENAPI_SPEC_KEY, makerAPIJson)) {
      makerAPISpecReady = true;
      DEBUG_PRINTF("WebPlatform: Maker API spec generated and stored (%d "
                   "bytes, %d routes)\n",
                   makerAPIJson.length(), makerProcessedCount);
    } else {
      ERROR_PRINTLN("ERROR: Failed to store Maker API spec in storage system");
      makerAPISpecReady = false;
    }

    // Clear temporary string to free memory
    makerAPIJson = "";
  }
#endif

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

void WebPlatform::streamPreGeneratedMakerAPISpec(WebResponse &res) const {
#if !MAKERAPI_ENABLED
  res.setStatus(501);
  res.setContent("{\"error\":\"Maker API specification generation disabled\"}",
                 "application/json");
  return;
#else
  // Check if we have any Maker API routes
  if (!makerAPISpecReady) {
#if OPENAPI_ENABLED
    // If OpenAPI is enabled, suggest using that endpoint instead
    res.setStatus(404);
    res.setContent("{\"error\":\"No Maker API routes found. Have any routes "
                   "been tagged for inclusion in the maker API?.\"}",
                   "application/json");
#else
    // Standard error if OpenAPI is also disabled
    res.setStatus(404);
    res.setContent(
        "{\"error\":\"No Maker API routes found or generation failed\"}",
        "application/json");
#endif
    return;
  }
  DEBUG_PRINTF("WebPlatform: Maker API spec request - ready flag: %s\n",
               makerAPISpecReady ? "true" : "false");

  if (!makerAPISpecReady) {
    res.setStatus(503);
    res.setContent("{\"error\":\"Maker API specification not ready\"}",
                   "application/json");
    return;
  }

  // Retrieve from storage system
  IDatabaseDriver *driver = StorageManager::driver();
  if (!driver) {
    ERROR_PRINTLN("WebPlatform: Storage driver unavailable for Maker API spec");
    res.setStatus(500);
    res.setContent("{\"error\":\"Storage system unavailable\"}",
                   "application/json");
    return;
  }

  DEBUG_PRINTF(
      "WebPlatform: Attempting to retrieve Maker API spec from storage "
      "(collection: %s, key: %s)\n",
      OPENAPI_COLLECTION.c_str(), MAKER_OPENAPI_SPEC_KEY.c_str());

  String makerAPISpec =
      driver->retrieve(OPENAPI_COLLECTION, MAKER_OPENAPI_SPEC_KEY);

  if (makerAPISpec.isEmpty()) {
    ERROR_PRINTF("WebPlatform: Maker API spec not found in storage! Collection "
                 "exists: %s\n",
                 driver->exists(OPENAPI_COLLECTION, MAKER_OPENAPI_SPEC_KEY)
                     ? "true"
                     : "false");

    // Try to regenerate the spec on-demand if it's missing
    DEBUG_PRINTLN("WebPlatform: Attempting to regenerate OpenAPI spec "
                  "(includes Maker API)...");
    const_cast<WebPlatform *>(this)->generateOpenAPISpec();

    // Try again after regeneration
    makerAPISpec = driver->retrieve(OPENAPI_COLLECTION, MAKER_OPENAPI_SPEC_KEY);
    if (makerAPISpec.isEmpty()) {
      ERROR_PRINTLN("WebPlatform: Failed to regenerate Maker API spec");
      res.setStatus(404);
      res.setContent(
          "{\"error\":\"Maker API specification not found in storage "
          "and regeneration failed\"}",
          "application/json");
      return;
    }

    DEBUG_PRINTF(
        "WebPlatform: Successfully regenerated Maker API spec (%d bytes)\n",
        makerAPISpec.length());
  }

  DEBUG_PRINTLN("WebPlatform: Serving Maker API spec using storage streaming");

  res.setStatus(200);
  res.setHeader("Cache-Control", "public, max-age=300");

  // Use storage streaming for memory efficiency
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