#include "platform/openapi_spec_helpers.h"
#include "route_entry.h"
#include "storage/storage_manager.h"
#include "web_platform.h"
#include <ArduinoJson.h>
#include <interface/openapi_types.h>
#include <interface/web_module_types.h>
#include <map>


// OpenAPI specification generation and serving functions
// This file contains the comprehensive OpenAPI generation logic that was moved
// from the old web_platform_openapi.cpp to work with the new pre-generation
// architecture

// Static constants for OpenAPI storage
const String WebPlatform::OPENAPI_COLLECTION = "openapi";
const String WebPlatform::OPENAPI_SPEC_KEY = "spec";
const String WebPlatform::MAKER_OPENAPI_SPEC_KEY = "maker";

// Helper function to generate and store a spec
bool WebPlatform::generateAndStoreSpec(
    size_t targetSize, const String &title, const String &description,
    std::function<bool(const OpenAPIGenerationContext::RouteDocumentation &)>
        routeFilter,
    std::function<void(JsonArray &,
                       const OpenAPIGenerationContext::RouteDocumentation &)>
        tagModifier,
    const String &storageKey, const String &specType) {

  JsonDocument doc;
  OpenApiSpecHelpers::createOpenAPIDocumentStructure(
      doc, title, description, getSystemVersion(), getBaseUrl());

  JsonObject paths = doc["paths"].to<JsonObject>();

  std::vector<std::pair<String, String>> registeredModuleInfo;
  registeredModuleInfo.reserve(registeredModules.size());
  for (const auto &regModule : registeredModules) {
    registeredModuleInfo.emplace_back(regModule.basePath,
                                      regModule.webModule->getModuleName());
  }

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
    if (!paths[pathKey].isNull()) {
      pathItem = paths[pathKey];
    } else {
      pathItem = paths[pathKey].to<JsonObject>();
    }

    JsonObject operation = pathItem[methodStr].to<JsonObject>();
    const OpenAPIDocumentation &docs = routeDoc.docs;

    // Add basic operation properties
#if OPENAPI_ENABLED
    if (!docs.getSummary().isEmpty()) {
      operation["summary"] = docs.getSummary();
    } else {
      operation["summary"] =
          OpenApiSpecHelpers::generateDefaultSummary(routePathStr, methodStr);
    }

    if (!docs.getOperationId().isEmpty()) {
      operation["operationId"] = docs.getOperationId();
    } else {
      operation["operationId"] =
          OpenApiSpecHelpers::generateOperationId(methodStr, routePathStr);
    }

    if (!docs.getDescription().isEmpty()) {
      operation["description"] = docs.getDescription();
    }

    // Handle tags using the provided modifier
    JsonArray tags = operation["tags"].to<JsonArray>();
    String defaultModuleTag = OpenApiSpecHelpers::inferModuleFromPath(
        routePathStr, registeredModuleInfo);

    if (!docs.getTags().empty()) {
      // Add default webModule tag first
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
      // Just add the default webModule tag
      tags.add(defaultModuleTag);
    }

    // Apply custom tag modification (e.g., for Maker API)
    tagModifier(tags, routeDoc);
#else
    // When OpenAPI is disabled, just generate basic operation info
    operation["summary"] =
        OpenApiSpecHelpers::generateDefaultSummary(routePathStr, methodStr);
    operation["operationId"] =
        OpenApiSpecHelpers::generateOperationId(methodStr, routePathStr);

    JsonArray tags = operation["tags"].to<JsonArray>();
    tags.add(OpenApiSpecHelpers::inferModuleFromPath(routePathStr,
                                                      registeredModuleInfo));
    tagModifier(tags, routeDoc);
#endif

    // Add authentication requirements
    if (!routeDoc.authRequirements.empty()) {
      JsonArray security = operation["security"].to<JsonArray>();
      for (const auto &authType : routeDoc.authRequirements) {
        if (authType == AuthType::TOKEN) {
          JsonObject secObj = security.add<JsonObject>();
          secObj["bearerAuth"].to<JsonArray>();
          JsonObject tokenSecObj = security.add<JsonObject>();
          tokenSecObj["tokenParam"].to<JsonArray>();
        } else if (authType == AuthType::SESSION) {
          JsonObject secObj = security.add<JsonObject>();
          secObj["cookieAuth"].to<JsonArray>();
        }
      }
    }

    // Add parameters, request body, and responses
    OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

    // Add request body for POST/PUT operations
#if OPENAPI_ENABLED
    if ((routeDoc.method == WebModule::WM_POST ||
         routeDoc.method == WebModule::WM_PUT) &&
        (!docs.getRequestSchema().isEmpty() ||
         !docs.getRequestExample().isEmpty())) {
      OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation,
                                                            routeDoc);
    }
#else
    if (routeDoc.method == WebModule::WM_POST ||
        routeDoc.method == WebModule::WM_PUT) {
      OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation,
                                                            routeDoc);
    }
#endif

    // Add responses
#if OPENAPI_ENABLED
    if (!docs.getResponsesJson().isEmpty() ||
        !docs.getResponseSchema().isEmpty() ||
        !docs.getResponseExample().isEmpty()) {
      OpenApiSpecHelpers::addResponsesToOperationFromDocs(operation, routeDoc);
    } else {
#else
    {
#endif
      // Add basic responses
      JsonObject responses = operation["responses"].to<JsonObject>();
      JsonObject response200 = responses["200"].to<JsonObject>();
      response200["description"] = "Successful operation";

      // Add auth error responses if needed
      if (!routeDoc.authRequirements.empty()) {
        JsonObject response401 = responses["401"].to<JsonObject>();
        response401["description"] = "Unauthorized - Authentication required";
        JsonObject response403 = responses["403"].to<JsonObject>();
        response403["description"] = "Forbidden - Insufficient permissions";
      }

      JsonObject response500 = responses["500"].to<JsonObject>();
      response500["description"] = "Internal server error";
    }

    // Check if we're running low on memory - only warn after processing some
    // routes and if we're within 10% of the caller's intended size budget.
    // JsonDocument (v7) grows elastically rather than using a fixed buffer,
    // so there's no true "capacity" to check anymore, and memoryUsage() is a
    // deprecated stub that always returns 0 - measureJson() (still fully
    // supported) estimates serialized content size, which is what
    // targetSize was budgeting for in the first place.
    size_t usedBytes = measureJson(doc);
    if (processedCount > 2 && usedBytes > targetSize * 0.9) {
      size_t remainingBytes = targetSize > usedBytes ? targetSize - usedBytes : 0;
      WARN_PRINTF("WARNING: %s JSON document nearly full at route #%d (%d/%d "
                  "bytes, %d remaining)\n",
                  specType.c_str(), processedCount, usedBytes, targetSize,
                  remainingBytes);

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
  IDatabaseDriver *driver = &StorageManager::driver("littlefs");
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

  IDatabaseDriver *driver = &StorageManager::driver("littlefs");
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
  // Generate Maker API spec - always generate, even if empty
  // This ensures a valid (empty) spec is available rather than returning 503
  DEBUG_PRINTLN("WebPlatform: Generating Maker API OpenAPI specification...");

  // Count routes for logging
  int makerRouteCount = 0;
  for (const auto &routeDoc : openAPIGenerationContext.getApiRoutes()) {
    if (OpenApiSpecHelpers::isMakerAPIRoute(routeDoc, makerApiTags)) {
      makerRouteCount++;
    }
  }

  if (makerRouteCount == 0) {
    DEBUG_PRINTLN("WebPlatform: No routes with configured maker tags found");
    DEBUG_PRINT("WebPlatform: Configured maker tags: ");
    for (size_t i = 0; i < makerApiTags.size(); i++) {
      DEBUG_PRINT(makerApiTags[i]);
      if (i < makerApiTags.size() - 1) DEBUG_PRINT(", ");
    }
    DEBUG_PRINTLN();
  } else {
    DEBUG_PRINTF("WebPlatform: Found %d routes matching maker tags\n", makerRouteCount);
  }

  auto makerRoutesFilter =
      [this](const OpenAPIGenerationContext::RouteDocumentation &routeDoc)
      -> bool {
    return OpenApiSpecHelpers::isMakerAPIRoute(routeDoc, makerApiTags);
  };

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
  IDatabaseDriver *driver = &StorageManager::driver("littlefs");
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
    ERROR_PRINTLN("WebPlatform: Maker API spec generation failed during initialization");
    res.setStatus(500);
    res.setContent("{\"error\":\"Maker API specification failed to generate during initialization\"}",
                   "application/json");
    return;
  }

  // Retrieve from storage system
  IDatabaseDriver *driver = &StorageManager::driver("littlefs");
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
