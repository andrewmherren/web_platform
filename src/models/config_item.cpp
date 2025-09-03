#include "../../include/models/data_models.h"
#include <ArduinoJson.h>

// ConfigItem implementation
ConfigItem::ConfigItem(const String& key, const String& value)
    : key(key), value(value), updatedAt(millis()) {
}

String ConfigItem::toJson() const {
    DynamicJsonDocument doc(512);
    doc["key"] = key;
    doc["value"] = value;
    doc["updatedAt"] = updatedAt;
    
    String json;
    serializeJson(doc, json);
    return json;
}

ConfigItem ConfigItem::fromJson(const String& json) {
    ConfigItem item;
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (!error) {
        item.key = doc["key"].as<String>();
        item.value = doc["value"].as<String>();
        item.updatedAt = doc["updatedAt"].as<unsigned long>();
    }
    
    return item;
}

bool ConfigItem::isValid() const {
    return key.length() > 0;
}