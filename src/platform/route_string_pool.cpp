#include "route_string_pool.h"

// Static storage for route strings
std::vector<String> RouteStringPool::stringStorage;

const char* RouteStringPool::store(const String& str) {
    if (str.isEmpty()) {
        return nullptr;
    }
    
    // Check if string already exists to avoid duplicates
    for (const auto& existing : stringStorage) {
        if (existing == str) {
            return existing.c_str();
        }
    }
    
    // Store new string
    stringStorage.push_back(str);
    return stringStorage.back().c_str();
}

const char* RouteStringPool::store(const char* str) {
    if (!str || strlen(str) == 0) {
        return nullptr;
    }
    
    // For const char*, we assume it's already in stable memory (PROGMEM or static)
    // so we can return it directly without storing
    return str;
}

const char* RouteStringPool::empty() {
    return nullptr;
}

void RouteStringPool::clear() {
    Serial.printf("RouteStringPool: Clearing %d stored strings\n", stringStorage.size());
    stringStorage.clear();
}

size_t RouteStringPool::getStorageCount() {
    return stringStorage.size();
}

size_t RouteStringPool::getEstimatedMemoryUsage() {
    size_t totalSize = 0;
    for (const auto& str : stringStorage) {
        totalSize += str.length() + 1; // +1 for null terminator
    }
    return totalSize;
}