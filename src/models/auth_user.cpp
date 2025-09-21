#include "auth/auth_utils.h"
#include "models/data_models.h"
#include <ArduinoJson.h>
#include <time.h>

// AuthUser implementation
AuthUser::AuthUser(const String &username, const String &hash,
                   const String &salt)
    : id(AuthUtils::generateUserId()), username(username), passwordHash(hash),
      salt(salt), createdAt(time(nullptr)) {}

String AuthUser::toJson() const {
  DynamicJsonDocument doc(512);
  doc["id"] = id;
  doc["username"] = username;
  doc["passwordHash"] = passwordHash;
  doc["salt"] = salt;
  doc["createdAt"] = createdAt;

  String json;
  serializeJson(doc, json);
  return json;
}

AuthUser AuthUser::fromJson(const String &json) {
  AuthUser user;

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, json);

  if (!error) {
    user.id = doc["id"].as<String>();
    user.username = doc["username"].as<String>();
    user.passwordHash = doc["passwordHash"].as<String>();
    user.salt = doc["salt"].as<String>();
    user.createdAt = doc["createdAt"].as<unsigned long>();
  }

  return user;
}

bool AuthUser::isValid() const {
  return id.length() > 0 && username.length() > 0 && passwordHash.length() > 0;
}