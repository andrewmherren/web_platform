#include "../../include/auth/auth_utils.h"
#include "../../include/models/data_models.h"
#include <ArduinoJson.h>
#include <time.h> // ConfigItem implementation

ConfigItem::ConfigItem(const String &key, const String &value)
    : id(AuthUtils::generateUserId()), key(key), value(value),
      updatedAt(time(nullptr)) {}
String ConfigItem::toJson() const {
  DynamicJsonDocument doc(512);
  doc["id"] = id;
  doc["key"] = key;
  doc["value"] = value;
  doc["updatedAt"] = updatedAt;

  String json;
  serializeJson(doc, json);
  return json;
}

ConfigItem ConfigItem::fromJson(const String &json) {
  ConfigItem item;

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, json);
  if (!error) {
    item.id = doc["id"].as<String>();
    item.key = doc["key"].as<String>();
    item.value = doc["value"].as<String>();
    item.updatedAt = doc["updatedAt"].as<unsigned long>();
  }

  return item;
}

bool ConfigItem::isValid() const { return id.length() > 0 && key.length() > 0; }
