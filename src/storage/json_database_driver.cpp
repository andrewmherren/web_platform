#include "../../include/storage/json_database_driver.h"
#include <ArduinoJson.h>

#if defined(ESP32)
#include <Preferences.h>
#elif defined(ESP8266)
#include <EEPROM.h>
#endif

JsonDatabaseDriver::JsonDatabaseDriver() : driverName("json"), initialized(false) {
#if defined(ESP8266)
    EEPROM.begin(4096);
#endif
}

JsonDatabaseDriver::~JsonDatabaseDriver() {
    // Cleanup if needed
}

void JsonDatabaseDriver::ensureInitialized() {
    if (!initialized) {
        initialized = true;
    }
}

void JsonDatabaseDriver::loadCollection(const String& collection) {
    ensureInitialized();
    
    // Check if already loaded in cache
    if (cache.find(collection) != cache.end()) {
        return;
    }
    
    // Initialize collection map
    cache[collection] = std::map<String, String>();
    
#if defined(ESP32)
    Preferences prefs;
    prefs.begin("storage", false);
    
    String jsonData = prefs.getString(collection.c_str(), "[]");
    prefs.end();
    
#elif defined(ESP8266)
    // Calculate storage position based on collection name hash
    unsigned int collectionHash = 0;
    for (unsigned int i = 0; i < collection.length(); i++) {
        collectionHash = collectionHash * 31 + collection[i];
    }
    
    // Use hash to determine storage position (avoid collisions with simple offset)
    int storagePos = (collectionHash % 10) * 400; // Max 10 collections, 400 bytes each
    if (storagePos + 4 >= 4096) storagePos = 0; // Fallback to start
    
    // Read length
    int jsonLength = 0;
    EEPROM.get(storagePos, jsonLength);
    
    String jsonData = "[]";
    if (jsonLength > 0 && jsonLength < 390) { // Leave room for length int
        char* buffer = new char[jsonLength + 1];
        for (int i = 0; i < jsonLength; i++) {
            buffer[i] = EEPROM.read(storagePos + 4 + i);
        }
        buffer[jsonLength] = '\0';
        jsonData = String(buffer);
        delete[] buffer;
    }
#endif
    
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

void JsonDatabaseDriver::saveCollection(const String& collection) {
    ensureInitialized();
    
    // Build JSON array from cache
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();
    
    if (cache.find(collection) != cache.end()) {
        for (const auto& pair : cache[collection]) {
            JsonObject item = array.createNestedObject();
            item["key"] = pair.first;
            item["data"] = pair.second;
        }
    }
    
    String jsonData;
    serializeJson(doc, jsonData);
    
#if defined(ESP32)
    Preferences prefs;
    prefs.begin("storage", false);
    prefs.putString(collection.c_str(), jsonData);
    prefs.end();
    
#elif defined(ESP8266)
    // Calculate storage position (same as load)
    unsigned int collectionHash = 0;
    for (unsigned int i = 0; i < collection.length(); i++) {
        collectionHash = collectionHash * 31 + collection[i];
    }
    
    int storagePos = (collectionHash % 10) * 400;
    if (storagePos + 4 >= 4096) storagePos = 0;
    
    // Write length
    int jsonLength = jsonData.length();
    if (jsonLength >= 390) {
        Serial.printf("Warning: Collection '%s' data too large (%d bytes), truncating\n", 
                     collection.c_str(), jsonLength);
        jsonData = jsonData.substring(0, 389);
        jsonLength = 389;
    }
    
    EEPROM.put(storagePos, jsonLength);
    
    // Write data
    for (int i = 0; i < jsonLength; i++) {
        EEPROM.write(storagePos + 4 + i, jsonData[i]);
    }
    
    EEPROM.commit();
#endif
}

bool JsonDatabaseDriver::store(const String& collection, const String& key, const String& data) {
    if (collection.length() == 0 || key.length() == 0) {
        return false;
    }
    
    loadCollection(collection);
    cache[collection][key] = data;
    saveCollection(collection);
    return true;
}

String JsonDatabaseDriver::retrieve(const String& collection, const String& key) {
    if (collection.length() == 0 || key.length() == 0) {
        return "";
    }
    
    loadCollection(collection);
    
    if (cache.find(collection) != cache.end()) {
        const auto& collectionMap = cache[collection];
        auto it = collectionMap.find(key);
        if (it != collectionMap.end()) {
            return it->second;
        }
    }
    
    return "";
}

bool JsonDatabaseDriver::remove(const String& collection, const String& key) {
    if (collection.length() == 0 || key.length() == 0) {
        return false;
    }
    
    loadCollection(collection);
    
    if (cache.find(collection) != cache.end()) {
        auto& collectionMap = cache[collection];
        auto it = collectionMap.find(key);
        if (it != collectionMap.end()) {
            collectionMap.erase(it);
            saveCollection(collection);
            return true;
        }
    }
    
    return false;
}

std::vector<String> JsonDatabaseDriver::listKeys(const String& collection) {
    std::vector<String> keys;
    
    if (collection.length() == 0) {
        return keys;
    }
    
    loadCollection(collection);
    
    if (cache.find(collection) != cache.end()) {
        const auto& collectionMap = cache[collection];
        for (const auto& pair : collectionMap) {
            keys.push_back(pair.first);
        }
    }
    
    return keys;
}

bool JsonDatabaseDriver::exists(const String& collection, const String& key) {
    if (collection.length() == 0 || key.length() == 0) {
        return false;
    }
    
    loadCollection(collection);
    
    if (cache.find(collection) != cache.end()) {
        const auto& collectionMap = cache[collection];
        return collectionMap.find(key) != collectionMap.end();
    }
    
    return false;
}

String JsonDatabaseDriver::getDriverName() const {
    return driverName;
}

void JsonDatabaseDriver::clearCache() {
    cache.clear();
}

void JsonDatabaseDriver::clearCollection(const String& collection) {
    if (cache.find(collection) != cache.end()) {
        cache[collection].clear();
        saveCollection(collection);
    }
}