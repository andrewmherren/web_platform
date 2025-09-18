#include "../include/web_platform.h"
#include <ArduinoJson.h>
#include <map>

void WebPlatform::streamOpenAPISpec(WebResponse &res) const {
  Serial.println("Streaming OpenAPI spec");

  // Force garbage collection and memory cleanup first
  ESP.getFreeHeap(); // Can trigger cleanup

  // Small delay to allow cleanup
  delay(50);

  // Check available heap memory before allocation
  size_t freeHeap = ESP.getFreeHeap();
  Serial.println("Free heap before allocation: " + String(freeHeap) + " bytes");
#if defined(ESP32)
  size_t maxBlock = ESP.getMaxAllocHeap();
  Serial.println("Largest free block: " + String(maxBlock) + " bytes");

  // Log heap fragmentation ratio
  double fragmentation = 1.0 - ((double)maxBlock / (double)freeHeap);
  Serial.println("Heap fragmentation: " + String(fragmentation * 100, 1) + "%");
#endif

  // Use progressive allocation strategy - start smaller and only grow if needed
  size_t targetSize;

#if defined(ESP32)
  // On ESP32, use largest available block minus safety margin
  size_t maxAllowable = (size_t)(maxBlock * 0.8);
  targetSize = (maxAllowable < 24576)
                   ? maxAllowable
                   : 24576; // Max 24KB or 80% of largest block
#else
  // On ESP8266, be more conservative due to smaller heap
  size_t maxAllowable = (size_t)(freeHeap * 0.3);
  targetSize = (maxAllowable < 16384) ? maxAllowable
                                      : 16384; // Max 16KB or 30% of free heap
#endif

  // Minimum viable size - must be at least 8KB for basic OpenAPI structure
  if (targetSize < 8192) {
    Serial.println("ERROR: Insufficient memory for OpenAPI generation!");
    Serial.println("Available: " + String(freeHeap) +
                   " bytes, minimum required: 8192 bytes");
    res.setStatus(503);                 // Service Unavailable
    res.setHeader("Retry-After", "60"); // Suggest retry after 60 seconds
    res.setContent("{\"error\":\"Insufficient memory for OpenAPI "
                   "generation\",\"required\":8192,\"available\":" +
                       String(freeHeap) +
                       ",\"suggestion\":\"Try again in a few moments or "
                       "restart the device\"}",
                   "application/json");
    return;
  }

  Serial.println("Attempting allocation of " + String(targetSize) +
                 " bytes for JSON document");

  DynamicJsonDocument doc(targetSize);

  // Verify allocation succeeded
  if (doc.capacity() == 0) {
    Serial.println("ERROR: Failed to allocate " + String(targetSize) +
                   " bytes for JSON document!");

    // Try one more time with absolute minimum
    Serial.println("Trying emergency allocation of 8KB...");
    DynamicJsonDocument emergencyDoc(8192);

    if (emergencyDoc.capacity() == 0) {
      Serial.println("ERROR: Cannot allocate even 8KB for OpenAPI document!");
      res.setStatus(503);
      res.setHeader("Retry-After", "60");
      res.setContent("{\"error\":\"Critical memory shortage - OpenAPI "
                     "generation impossible\"}",
                     "application/json");
      return;
    }

    // Use the emergency document
    doc = std::move(emergencyDoc);
    Serial.println(
        "Using emergency 8KB allocation - document may be truncated");
  }

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
    String routePathStr = route.path ? String(route.path) : String("");
    if (routePathStr.indexOf("/api/") == -1) {
      continue;
    }

    // Get or create the path object
    JsonObject pathItem;
    const char *pathKey = route.path ? route.path : "";
    if (paths.containsKey(pathKey)) {
      pathItem = paths[pathKey];
    } else {
      pathItem = paths.createNestedObject(pathKey);
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
    if (!(!route.summary || strlen(route.summary) == 0)) {
      operation["summary"] = route.summary ? route.summary : "";
    } else {
      operation["summary"] = generateDefaultSummary(routePathStr, methodStr);
      // Log warning for missing documentation on token routes
      if (hasTokenAuth(route.authRequirements)) {
        Serial.print("WARNING: Token route ");
        Serial.print(route.path ? route.path : "<null>");
        Serial.println(" missing documentation summary");
      }
    }

    // Use module-provided operationId or generate default
    String operationId;
    if (!(!(route.operationId ? route.operationId : "") ||
          strlen((route.operationId ? route.operationId : "")) == 0)) {
      operationId = (route.operationId ? route.operationId : "");
    } else {
      operationId = generateOperationId(methodStr, routePathStr);
    }
    operation["operationId"] = operationId;

    // Add tags - use module-provided tags or infer from module
    JsonArray tags = operation.createNestedArray("tags");

    // Get the default module tag from the path
    String defaultModuleTag = inferModuleFromPath(routePathStr);

    if (!(!(route.tags ? route.tags : "") ||
          strlen((route.tags ? route.tags : "")) == 0)) {
      // Always add the module tag first
      tags.add(defaultModuleTag);

      // Parse comma-separated tags provided by the module
      String tagsCopy = route.tags ? String(route.tags) : String("");
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

  // Before serializing, check document size and allocate properly
  size_t estimatedSize = measureJson(doc);
  if (estimatedSize <= 2) {
    Serial.println("ERROR: Document appears to be empty or corrupted!");
    Serial.println("Document content check:");
    String debugOutput;
    serializeJson(doc, debugOutput);
    Serial.println("Raw JSON: " + debugOutput);
    res.setStatus(500);
    res.setContent(
        "{\"error\":\"OpenAPI spec generation failed - empty document\"}",
        "application/json");
    return;
  }

  if (estimatedSize > doc.capacity() * 0.9) {
    Serial.println("WARNING: OpenAPI document very close to capacity limit!");
  }

  // Check if document will likely be truncated
  bool documentTruncated = (estimatedSize > doc.capacity() * 0.95);
  if (documentTruncated) {
    Serial.println("WARNING: Document will likely be truncated!");
    Serial.println("Estimated size: " + String(estimatedSize) +
                   ", capacity: " + String(doc.capacity()));

    // Add warning to the document itself
    if (doc.containsKey("info")) {
      JsonObject info = doc["info"];
      info["x-generation-warning"] =
          "Document may be truncated due to memory constraints";
      info["x-estimated-size"] = estimatedSize;
      info["x-actual-capacity"] = doc.capacity();
    }
  }

  // Check if we have enough memory for String allocation
  size_t freeHeapAfterDoc = ESP.getFreeHeap();
  Serial.println("Free heap after document creation: " +
                 String(freeHeapAfterDoc) + " bytes");

  // We need at least 1.5x the estimated size for safe String allocation
  size_t requiredForString = estimatedSize + (estimatedSize / 2);

  if (freeHeapAfterDoc < requiredForString) {
    Serial.println("WARNING: Insufficient heap for String allocation!");
    Serial.println("Required: " + String(requiredForString) +
                   ", Available: " + String(freeHeapAfterDoc));

    // Force garbage collection attempt
    ESP.getFreeHeap(); // This can trigger cleanup on some platforms
    delay(10);

    size_t freeHeapAfterGC = ESP.getFreeHeap();
    Serial.println("Free heap after GC attempt: " + String(freeHeapAfterGC) +
                   " bytes");

    if (freeHeapAfterGC < requiredForString) {
      Serial.println("ERROR: Cannot allocate String for JSON output - "
                     "insufficient memory");
      res.setStatus(503);
      res.setHeader("Retry-After", "30");
      res.setContent("{\"error\":\"Memory fragmentation prevents OpenAPI "
                     "generation\",\"requiredMemory\":" +
                         String(requiredForString) +
                         ",\"availableMemory\":" + String(freeHeapAfterGC) +
                         ",\"suggestion\":\"Device restart recommended for "
                         "optimal performance\"}",
                     "application/json");
      return;
    }
  }

  // Set response headers first
  res.setHeader("Content-Type", "application/json");
  res.setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  res.setHeader("Pragma", "no-cache");
  res.setHeader("Expires", "0");

  // Add generation metadata headers
  res.setHeader("X-Generation-Time", String(millis()));
  res.setHeader("X-Free-Heap-Before", String(freeHeap));
  res.setHeader("X-Free-Heap-After", String(ESP.getFreeHeap()));
  res.setHeader("X-Doc-Size", String(estimatedSize));

  // Check if document will likely be truncated
  if (documentTruncated) {
    res.setHeader("X-Content-Truncated", "true");
    res.setHeader("X-Expected-Size", String(estimatedSize));
  }

  // Use streaming JSON serialization to avoid String allocation
  Serial.println("Using streaming JSON serialization for OpenAPI spec");
  streamOpenAPIJson(doc, res);
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
    }
  }

  // If we found a registered module, format its name
  if (!moduleName.isEmpty()) {
    return formatModuleName(moduleName);
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
  if (!(!route.parameters || strlen(route.parameters) == 0)) {
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
          if (!(!route.parameterConstraints ||
                strlen(route.parameterConstraints) == 0)) {
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
  String contentType = (!route.contentType || strlen(route.contentType) == 0)
                           ? "application/json"
                           : String(route.contentType);
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add response schema if provided
  if (!(!route.responseSchema || strlen(route.responseSchema) == 0)) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, route.responseSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add response example if provided
  if (!(!route.responseExample || strlen(route.responseExample) == 0)) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, route.responseExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Add module-provided response info
  if (!(!route.responseInfo || strlen(route.responseInfo) == 0)) {
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
  String contentType = (!route.contentType || strlen(route.contentType) == 0)
                           ? "application/json"
                           : String(route.contentType);
  JsonObject mediaType = content.createNestedObject(contentType);

  // Add request schema if provided
  if (!(!route.requestSchema || strlen(route.requestSchema) == 0)) {
    DynamicJsonDocument schemaDoc(2048);
    if (deserializeJson(schemaDoc, route.requestSchema) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  // Add request example if provided
  if (!(!route.requestExample || strlen(route.requestExample) == 0)) {
    DynamicJsonDocument exampleDoc(2048);
    if (deserializeJson(exampleDoc, route.requestExample) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  // Make request body required for POST/PUT operations by default
  requestBody["required"] = true;
}

void WebPlatform::streamOpenAPIJson(const JsonDocument &doc,
                                    WebResponse &res) const {
  Serial.println("Starting true streaming JSON serialization");

  // Check available memory before proceeding
  size_t freeHeap = ESP.getFreeHeap();
  Serial.println("Free heap before streaming: " + String(freeHeap) + " bytes");

  if (freeHeap < 2048) {
    Serial.println("ERROR: Insufficient memory for streaming - falling back to "
                   "error response");
    res.setContent("{\"error\":\"Insufficient memory for JSON streaming\"}",
                   "application/json");
    return;
  }

  // For ESP32 HTTPS, we need to handle this differently than regular HTTP
  // The WebResponse class doesn't support true streaming yet, so we fall back
  // to the safer String approach but with better memory management

  // Estimate size for headers
  size_t estimatedSize = measureJson(doc);
  Serial.println("Estimated JSON size: " + String(estimatedSize) + " bytes");

  // Check if we have enough memory for String allocation
  size_t requiredMemory = estimatedSize + 512; // Extra for String overhead
  if (freeHeap < requiredMemory) {
    Serial.println("ERROR: Insufficient memory for JSON String allocation");
    Serial.println("Required: " + String(requiredMemory) +
                   ", Available: " + String(freeHeap));

    res.setStatus(503);
    res.setContent("{\"error\":\"Insufficient memory for OpenAPI "
                   "generation\",\"required\":" +
                       String(requiredMemory) +
                       ",\"available\":" + String(freeHeap) + "}",
                   "application/json");
    return;
  }

  // Use a more memory-efficient approach
  String jsonOutput;

  // Reserve memory upfront to avoid fragmentation
  bool reserveSuccess = jsonOutput.reserve(estimatedSize + 256);
  if (!reserveSuccess) {
    Serial.println("ERROR: Failed to reserve String memory");
    res.setStatus(507); // Insufficient Storage
    res.setContent(
        "{\"error\":\"Failed to reserve memory for JSON generation\"}",
        "application/json");
    return;
  }

  Serial.println("Successfully reserved " + String(estimatedSize + 256) +
                 " bytes for JSON String");

  // Serialize the JSON
  size_t bytesWritten = serializeJson(doc, jsonOutput);

  Serial.println("JSON serialization completed: " + String(bytesWritten) +
                 " bytes");
  Serial.println("String length: " + String(jsonOutput.length()));
  Serial.println("Free heap after serialization: " + String(ESP.getFreeHeap()) +
                 " bytes");

  // Verify serialization was successful
  if (bytesWritten == 0 || jsonOutput.length() == 0) {
    Serial.println("ERROR: JSON serialization failed");
    res.setStatus(500);
    res.setContent("{\"error\":\"JSON serialization failed\"}",
                   "application/json");
    return;
  }

  // Verify JSON is complete (basic check)
  if (!jsonOutput.endsWith("}")) {
    Serial.println("ERROR: JSON appears truncated - does not end with '}'");
    res.setStatus(500);
    res.setContent("{\"error\":\"JSON generation incomplete\"}",
                   "application/json");
    return;
  }

  // Set content length header
  res.setHeader("Content-Length", String(jsonOutput.length()));

  // Set the content
  res.setContent(jsonOutput, "application/json");

  Serial.println("OpenAPI JSON response set successfully");
  Serial.println("Final free heap: " + String(ESP.getFreeHeap()) + " bytes");
}
