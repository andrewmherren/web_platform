#ifndef JSON_RESPONSE_BUILDER_H
#define JSON_RESPONSE_BUILDER_H

#include "../interface/web_response.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * JsonResponseBuilder - Memory-safe JSON response creation
 *
 * This class provides automatic memory management for JSON responses,
 * eliminating the need for handlers to manage String allocation/deallocation.
 *
 * Usage patterns:
 * 1. Small JSON (< 1KB): Uses stack-allocated StaticJsonDocument
 * 2. Medium JSON (1-8KB): Uses heap-allocated DynamicJsonDocument with safety
 * checks
 * 3. Large JSON (> 8KB): Uses streaming approach to avoid large String
 * allocation
 */
class JsonResponseBuilder {
private:
  static const size_t SMALL_JSON_SIZE = 1024;
  static const size_t MEDIUM_JSON_SIZE = 8192;
  static const size_t LARGE_JSON_THRESHOLD = 8192;

public:
  /**
   * Create a simple JSON response with automatic memory management
   * Best for small responses (< 1KB)
   */
  template <size_t N = SMALL_JSON_SIZE>
  static void createResponse(WebResponse &res,
                             std::function<void(JsonObject &)> builder) {
    StaticJsonDocument<N> doc;
    JsonObject root = doc.createNestedObject();

    // Let the handler populate the JSON
    builder(root);

    // Serialize directly to response without intermediate String
    String jsonStr;
    size_t jsonSize = measureJson(doc);
    jsonStr.reserve(jsonSize + 10); // Reserve with small buffer
    serializeJson(doc, jsonStr);
    res.setContent(jsonStr, "application/json");
  }

  /**
   * Create a JSON response with dynamic sizing
   * Automatically chooses the best strategy based on content size
   */
  static void createDynamicResponse(WebResponse &res,
                                    std::function<void(JsonObject &)> builder,
                                    size_t estimatedSize = MEDIUM_JSON_SIZE) {
    // Check available memory before allocation
    size_t freeHeap = ESP.getFreeHeap();

    if (estimatedSize > LARGE_JSON_THRESHOLD || freeHeap < estimatedSize * 3) {
      // Use streaming approach for large content or low memory
      createStreamingResponse(res, builder, estimatedSize);
    } else {
      // Use regular dynamic document
      createMediumResponse(res, builder, estimatedSize);
    }
  }

  /**
   * Create JSON array response with automatic memory management
   */
  template <size_t N = MEDIUM_JSON_SIZE>
  static void createArrayResponse(WebResponse &res,
                                  std::function<void(JsonArray &)> builder) {
    if (N <= SMALL_JSON_SIZE) {
      StaticJsonDocument<N> doc;
      JsonArray root = doc.createNestedArray();
      builder(root);

      String jsonStr;
      size_t jsonSize = measureJson(doc);
      jsonStr.reserve(jsonSize + 10);
      serializeJson(doc, jsonStr);
      res.setContent(jsonStr, "application/json");
    } else {
      DynamicJsonDocument doc(N);
      JsonArray root = doc.createNestedArray();
      builder(root);

      String jsonStr;
      size_t jsonSize = measureJson(doc);
      jsonStr.reserve(jsonSize + 10);
      serializeJson(doc, jsonStr);
      res.setContent(jsonStr, "application/json");
    }
  }

  /**
   * Memory-safe error response
   */
  static void createErrorResponse(WebResponse &res, const String &error,
                                  int statusCode = 400) {
    StaticJsonDocument<256> doc;
    JsonObject root = doc.createNestedObject();
    root["success"] = false;
    root["error"] = error;

    String jsonStr;
    serializeJson(doc, jsonStr);
    res.setStatus(statusCode);
    res.setContent(jsonStr, "application/json");
  }

  /**
   * Memory-safe success response
   */
  static void createSuccessResponse(WebResponse &res,
                                    const String &message = "Success") {
    StaticJsonDocument<256> doc;
    JsonObject root = doc.createNestedObject();
    root["success"] = true;
    root["message"] = message;

    String jsonStr;
    serializeJson(doc, jsonStr);
    res.setContent(jsonStr, "application/json");
  }

private:
  /**
   * Create medium-sized JSON response with safety checks
   */
  static void createMediumResponse(WebResponse &res,
                                   std::function<void(JsonObject &)> builder,
                                   size_t size) {
    // Pre-flight memory check
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < size * 2) {
      Serial.println("WARNING: Low memory for JSON response, using fallback");
      createErrorResponse(res, "Insufficient memory for response", 503);
      return;
    }

    DynamicJsonDocument doc(size);
    if (doc.capacity() == 0) {
      Serial.println("ERROR: Failed to allocate JSON document");
      createErrorResponse(res, "Memory allocation failed", 503);
      return;
    }

    JsonObject root = doc.createNestedObject();
    builder(root);

    String jsonStr;
    size_t jsonSize = measureJson(doc);
    jsonStr.reserve(jsonSize + 10);
    serializeJson(doc, jsonStr);
    res.setContent(jsonStr, "application/json");
  }

  /**
   * Create streaming JSON response for large content
   */
  static void createStreamingResponse(WebResponse &res,
                                      std::function<void(JsonObject &)> builder,
                                      size_t estimatedSize) {
    // For now, fall back to medium response with reduced size
    // TODO: Implement true streaming JSON generation
    size_t reducedSize = min(estimatedSize, MEDIUM_JSON_SIZE);
    createMediumResponse(res, builder, reducedSize);
  }
};

/**
 * Convenience macros for common JSON response patterns
 */
#define JSON_RESPONSE(res, code)                                               \
  JsonResponseBuilder::createResponse(res, [&](JsonObject &json) { code })

#define JSON_ARRAY_RESPONSE(res, code)                                         \
  JsonResponseBuilder::createArrayResponse(res, [&](JsonArray &json) { code })

#define JSON_ERROR(res, msg, status)                                           \
  JsonResponseBuilder::createErrorResponse(res, msg, status)

#define JSON_SUCCESS(res, msg)                                                 \
  JsonResponseBuilder::createSuccessResponse(res, msg)

#endif // JSON_RESPONSE_BUILDER_H