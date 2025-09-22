#include "storage/json_database_driver.h"
#include <ArduinoJson.h>

#include <Preferences.h>

JsonDatabaseDriver::JsonDatabaseDriver()
    : driverName("json"), initialized(false) {}

JsonDatabaseDriver::~JsonDatabaseDriver() {
  // Cleanup if needed
}

void JsonDatabaseDriver::ensureInitialized() {
  if (!initialized) {
    initialized = true;
  }
}

void JsonDatabaseDriver::loadCollection(const String &collection) {
  ensureInitialized();

  // Check if already loaded in cache
  if (cache.find(collection) != cache.end()) {
    return;
  }

  // Evict old collections if cache is getting too large
  if (cache.size() >= MAX_CACHED_COLLECTIONS) {
    evictOldCollections();
  }

  // Initialize collection map
  cache[collection] = std::map<String, String>();

  Preferences prefs;
  prefs.begin("storage", false);

  String jsonData = prefs.getString(collection.c_str(), "[]");
  prefs.end();

  // Calculate dynamic buffer size based on data length
  size_t jsonSize = jsonData.length() + 512; // Add 512 byte buffer for parsing
  if (jsonSize < 1024) jsonSize = 1024; // Minimum size

  // Parse JSON and populate cache
  DynamicJsonDocument doc(jsonSize);
  DeserializationError error = deserializeJson(doc, jsonData);

  if (!error && doc.is<JsonArray>()) {
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject item : array) {
      if (!item.isNull()) {
        const char* keyStr = item["key"];
        const char* dataStr = item["data"];
        if (keyStr && dataStr && strlen(keyStr) > 0) {
          cache[collection][String(keyStr)] = String(dataStr ? dataStr : "");
        }
      }
    }
  }
}

void JsonDatabaseDriver::saveCollection(const String &collection) {
  ensureInitialized();

  auto collectionIt = cache.find(collection);
  if (collectionIt == cache.end()) {
    return; // Nothing to save
  }

  // Calculate dynamic buffer size based on collection data
  size_t jsonSize = calculateJsonSize(collectionIt->second);

  // Build JSON array from cache
  DynamicJsonDocument doc(jsonSize);
  
  // Check if document allocation succeeded
  if (doc.capacity() == 0) {
    return; // Failed to allocate memory
  }
  
  JsonArray array = doc.to<JsonArray>();
  if (array.isNull()) {
    return; // Failed to create array
  }

  for (const auto &pair : collectionIt->second) {
    JsonObject item = array.createNestedObject();
    if (!item.isNull()) {
      item["key"] = pair.first.c_str();
      item["data"] = pair.second.c_str();
    }
  }

  String jsonData;
  size_t reserveSize = jsonSize > 256 ? jsonSize - 256 : 256;
  jsonData.reserve(reserveSize);
  
  size_t serializedSize = serializeJson(doc, jsonData);
  if (serializedSize == 0) {
    return; // Serialization failed
  }

  Preferences prefs;
  prefs.begin("storage", false);
  prefs.putString(collection.c_str(), jsonData);
  prefs.end();
}

bool JsonDatabaseDriver::store(const String &collection, const String &key,
                               const String &data) {
  if (collection.length() == 0 || key.length() == 0) {
    return false;
  }

  loadCollection(collection);
  cache[collection][key] = data;
  saveCollection(collection);
  return true;
}

String JsonDatabaseDriver::retrieve(const String &collection,
                                    const String &key) {
  if (collection.length() == 0 || key.length() == 0) {
    return String();
  }

  loadCollection(collection);

  auto collectionIt = cache.find(collection);
  if (collectionIt != cache.end()) {
    auto keyIt = collectionIt->second.find(key);
    if (keyIt != collectionIt->second.end()) {
      return keyIt->second;
    }
  }

  return String();
}

bool JsonDatabaseDriver::remove(const String &collection, const String &key) {
  if (collection.length() == 0 || key.length() == 0) {
    return false;
  }

  loadCollection(collection);

  auto collectionIt = cache.find(collection);
  if (collectionIt != cache.end()) {
    auto keyIt = collectionIt->second.find(key);
    if (keyIt != collectionIt->second.end()) {
      collectionIt->second.erase(keyIt);
      saveCollection(collection);
      return true;
    }
  }

  return false;
}

std::vector<String> JsonDatabaseDriver::listKeys(const String &collection) {
  std::vector<String> keys;

  if (collection.length() == 0) {
    return keys;
  }

  loadCollection(collection);

  auto collectionIt = cache.find(collection);
  if (collectionIt != cache.end()) {
    keys.reserve(collectionIt->second.size()); // Pre-allocate vector
    for (const auto &pair : collectionIt->second) {
      keys.push_back(pair.first);
    }
  }

  return keys;
}

bool JsonDatabaseDriver::exists(const String &collection, const String &key) {
  if (collection.length() == 0 || key.length() == 0) {
    return false;
  }

  loadCollection(collection);

  auto collectionIt = cache.find(collection);
  if (collectionIt != cache.end()) {
    return collectionIt->second.find(key) != collectionIt->second.end();
  }

  return false;
}

String JsonDatabaseDriver::getDriverName() const { return driverName; }

void JsonDatabaseDriver::clearCache() { cache.clear(); }

void JsonDatabaseDriver::clearCollection(const String &collection) {
  auto collectionIt = cache.find(collection);
  if (collectionIt != cache.end()) {
    collectionIt->second.clear();
    saveCollection(collection);
  }
}

void JsonDatabaseDriver::evictOldCollections() {
  // Simple eviction: clear all collections if we're at the limit
  // In a more sophisticated implementation, we could use LRU eviction
  if (cache.size() >= MAX_CACHED_COLLECTIONS) {
    cache.clear();
  }
}

size_t JsonDatabaseDriver::calculateJsonSize(const std::map<String, String>& collectionData) {
  size_t estimatedSize = 200; // Base JSON structure overhead
  size_t maxSingleItem = 0;
  
  for (const auto &pair : collectionData) {
    // More conservative estimate for nested JSON and escaping
    size_t keySize = pair.first.length();
    size_t dataSize = pair.second.length();
    
    // Track largest single item for validation
    size_t itemSize = keySize + dataSize;
    if (itemSize > maxSingleItem) {
      maxSingleItem = itemSize;
    }
    
    // Account for potential JSON escaping (quotes, backslashes, etc.)
    // and nested structures in data field
    estimatedSize += keySize * 2 + dataSize * 2 + 100;
    
    // If a single item is very large, we need much more buffer
    if (dataSize > 5000) { // Large document like OpenAPI spec
      estimatedSize += dataSize; // Additional buffer for large documents
    }
  }
  
  // Add 50% buffer for JSON formatting, escaping, and safety margin
  estimatedSize = (estimatedSize * 3) / 2;
  
  // Ensure minimum size
  if (estimatedSize < 2048) {
    estimatedSize = 2048;
  }
  
  // For very large single items, ensure we have enough space
  if (maxSingleItem > 10000) {
    size_t largeItemBuffer = maxSingleItem * 3; // 300% for very large items
    if (largeItemBuffer > estimatedSize) {
      estimatedSize = largeItemBuffer;
    }
  }
  
  // Cap maximum size to prevent excessive allocation
  if (estimatedSize > 65536) { // 64KB limit (increased from 32KB)
    estimatedSize = 65536;
  }
  
  return estimatedSize;
}

size_t JsonDatabaseDriver::getCacheSize() const {
  return cache.size();
}

void JsonDatabaseDriver::evictCollection(const String &collection) {
  auto it = cache.find(collection);
  if (it != cache.end()) {
    cache.erase(it);
  }
}