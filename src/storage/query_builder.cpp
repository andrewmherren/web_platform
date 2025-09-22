#include "storage/query_builder.h"
#include <ArduinoJson.h>

QueryBuilder::QueryBuilder(IDatabaseDriver* driver, const String& collectionName) 
    : targetDriver(driver), collection(collectionName), selectFields("*"), 
      limitCount(-1), orderField(""), orderDirection("ASC") {
}

// TODO: optional third arg for =/<> etc
QueryBuilder& QueryBuilder::where(const String& key, const String& value) {
    conditions[key] = value;
    return *this;
}

QueryBuilder& QueryBuilder::select(const String& fields) {
    selectFields = fields;
    return *this;
}

QueryBuilder& QueryBuilder::limit(int count) {
    limitCount = count;
    return *this;
}

// TODO: support multiple order by
QueryBuilder& QueryBuilder::orderBy(const String& field, const String& direction) {
    orderField = field;
    orderDirection = direction;
    return *this;
}

String QueryBuilder::get() {
    if (!targetDriver) {
        return "";
    }
    
    // If no conditions, just get first key
    if (conditions.empty()) {
        std::vector<String> keys = targetDriver->listKeys(collection);
        if (!keys.empty()) {
            return targetDriver->retrieve(collection, keys[0]);
        }
        return "";
    }
    
    // Find first match based on conditions
    std::vector<String> keys = targetDriver->listKeys(collection);
    int count = 0;
    
    for (const String& key : keys) {
        if (limitCount > 0 && count >= limitCount) {
            break;
        }
        
        String data = targetDriver->retrieve(collection, key);
        if (data.length() == 0) continue;
        
        // Check if data matches all conditions
        bool matches = true;
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, data);
        
        if (!error) {
            for (const auto& condition : conditions) {
                String fieldValue = doc[condition.first].as<String>();
                if (fieldValue != condition.second) {
                    matches = false;
                    break;
                }
            }
        } else {
            matches = false;
        }
        
        if (matches) {
            return data;
        }
        count++;
    }
    
    return "";
}

std::vector<String> QueryBuilder::getAll() {
    std::vector<String> results;
    
    if (!targetDriver) {
        return results;
    }
    
    std::vector<String> keys = targetDriver->listKeys(collection);
    int count = 0;
    
    for (const String& key : keys) {
        if (limitCount > 0 && count >= limitCount) {
            break;
        }
        
        String data = targetDriver->retrieve(collection, key);
        if (data.length() == 0) continue;
        
        // If no conditions, include all
        if (conditions.empty()) {
            results.push_back(data);
            count++;
            continue;
        }
        
        // Check if data matches all conditions
        bool matches = true;
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, data);
        
        if (!error) {
            for (const auto& condition : conditions) {
                String fieldValue = doc[condition.first].as<String>();
                if (fieldValue != condition.second) {
                    matches = false;
                    break;
                }
            }
        } else {
            matches = false;
        }
        
        if (matches) {
            results.push_back(data);
        }
        count++;
    }
    
    return results;
}

bool QueryBuilder::exists() {
    if (!targetDriver) {
        return false;
    }
    
    String result = get();
    return result.length() > 0;
}

bool QueryBuilder::store(const String& key, const String& data) {
    if (!targetDriver) {
        return false;
    }
    
    return targetDriver->store(collection, key, data);
}

bool QueryBuilder::remove() {
    if (!targetDriver) {
        return false;
    }
    
    bool anyRemoved = false;
    std::vector<String> keys = targetDriver->listKeys(collection);
    
    for (const String& key : keys) {
        String data = targetDriver->retrieve(collection, key);
        if (data.length() == 0) continue;
        
        // If no conditions, remove all
        if (conditions.empty()) {
            if (targetDriver->remove(collection, key)) {
                anyRemoved = true;
            }
            continue;
        }
        
        // Check if data matches all conditions
        bool matches = true;
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, data);
        
        if (!error) {
            for (const auto& condition : conditions) {
                String fieldValue = doc[condition.first].as<String>();
                if (fieldValue != condition.second) {
                    matches = false;
                    break;
                }
            }
        } else {
            matches = false;
        }
        
        if (matches) {
            if (targetDriver->remove(collection, key)) {
                anyRemoved = true;
            }
        }
    }
    
    return anyRemoved;
}

IDatabaseDriver* QueryBuilder::getDriver() const {
    return targetDriver;
}

String QueryBuilder::getCollection() const {
    return collection;
}