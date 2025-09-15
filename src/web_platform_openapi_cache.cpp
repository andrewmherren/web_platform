#include "../include/storage/storage_manager.h"
#include "../include/web_platform.h"
#include <ArduinoJson.h>

// Collection names for our OpenAPI cache
const char *OPENAPI_CACHE_COLLECTION = "openapi_cache";

void WebPlatform::initializeOpenAPICache() {
  if (openApiCacheInitialized)
    return;

  Serial.println("Initializing OpenAPI cache system");

  // Try to load cached specs from storage
  for (int authType = 0; authType <= 3; authType++) { // AuthType enum values
    String key = String(authType);
    String cached = StorageManager::query(OPENAPI_CACHE_COLLECTION)
                        .getDriver()
                        ->retrieve(OPENAPI_CACHE_COLLECTION, key);
    if (!cached.isEmpty() && cached != "null") {
      openApiCache[authType] = cached;
      Serial.println("Loaded cached OpenAPI spec for auth type " +
                     String(authType) + " (" + String(cached.length()) +
                     " bytes)");
    }
  }

  openApiCacheInitialized = true;
}

void WebPlatform::invalidateOpenAPICache() {
  Serial.println("Invalidating OpenAPI cache");

  // Clear memory cache
  openApiCache.clear();

  // Clear persistent cache
  for (int authType = 0; authType <= 3; authType++) {
    String key = String(authType);
    StorageManager::query(OPENAPI_CACHE_COLLECTION).where("_key", key).remove();
  }

  Serial.println("OpenAPI cache invalidated");
}

bool WebPlatform::hasValidOpenAPICache(AuthType filterType) const {
  if (!openApiCacheInitialized) {
    const_cast<WebPlatform *>(this)->initializeOpenAPICache();
  }

  int authTypeInt = (int)filterType;
  auto it = openApiCache.find(authTypeInt);

  if (it != openApiCache.end() && !it->second.isEmpty()) {
    // Additional validation: ensure it's valid JSON with basic structure
    if (it->second.indexOf("\"openapi\"") > 0 &&
        it->second.indexOf("\"paths\"") > 0) {
      return true;
    } else {
      Serial.println("WARNING: Cached OpenAPI spec for auth type " +
                     String(authTypeInt) + " appears invalid");
      // Remove invalid cache entry
      const_cast<WebPlatform *>(this)->openApiCache.erase(authTypeInt);
      String key = String(authTypeInt);
      StorageManager::query(OPENAPI_CACHE_COLLECTION)
          .where("_key", key)
          .remove();
    }
  }

  return false;
}

String WebPlatform::getCachedOpenAPISpec(AuthType filterType) const {
  if (!openApiCacheInitialized) {
    const_cast<WebPlatform *>(this)->initializeOpenAPICache();
  }

  int authTypeInt = (int)filterType;
  auto it = openApiCache.find(authTypeInt);

  if (it != openApiCache.end()) {
    return it->second;
  }

  return "";
}