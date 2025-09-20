#include "../../include/storage/json_database_driver.h"
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

  // Initialize collection map
  cache[collection] = std::map<String, String>();

  Preferences prefs;
  prefs.begin("storage", false);

  String jsonData = prefs.getString(collection.c_str(), "[]");
  prefs.end();

  // Parse JSON and populate cache
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonData);

  if (!error && doc.is<JsonArray>()) {
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject item : array) {
      String key = item["key"].as<String>();
      String data = item["data"].as<String>();
      if (key.length() > 0) {
        cache[collection][key] = data;
      }
    }
  }
}

void JsonDatabaseDriver::saveCollection(const String &collection) {
  ensureInitialized();

  // Build JSON array from cache
  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  if (cache.find(collection) != cache.end()) {
    for (const auto &pair : cache[collection]) {
      JsonObject item = array.createNestedObject();
      item["key"] = pair.first;
      item["data"] = pair.second;
    }
  }

  String jsonData;
  serializeJson(doc, jsonData);

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
    return "";
  }

  loadCollection(collection);

  if (cache.find(collection) != cache.end()) {
    const auto &collectionMap = cache[collection];
    auto it = collectionMap.find(key);
    if (it != collectionMap.end()) {
      return it->second;
    }
  }

  return "";
}

bool JsonDatabaseDriver::remove(const String &collection, const String &key) {
  if (collection.length() == 0 || key.length() == 0) {
    return false;
  }

  loadCollection(collection);

  if (cache.find(collection) != cache.end()) {
    auto &collectionMap = cache[collection];
    auto it = collectionMap.find(key);
    if (it != collectionMap.end()) {
      collectionMap.erase(it);
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

  if (cache.find(collection) != cache.end()) {
    const auto &collectionMap = cache[collection];
    for (const auto &pair : collectionMap) {
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

  if (cache.find(collection) != cache.end()) {
    const auto &collectionMap = cache[collection];
    return collectionMap.find(key) != collectionMap.end();
  }

  return false;
}

String JsonDatabaseDriver::getDriverName() const { return driverName; }

void JsonDatabaseDriver::clearCache() { cache.clear(); }

void JsonDatabaseDriver::clearCollection(const String &collection) {
  if (cache.find(collection) != cache.end()) {
    cache[collection].clear();
    saveCollection(collection);
  }
}