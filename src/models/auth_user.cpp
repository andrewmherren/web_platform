#include "auth/auth_utils.h"
#include "models/data_models.h"
#include <ArduinoJson.h>
#include <string>
#include <time.h>

// AuthUser implementation
AuthUser::AuthUser(const String &username, const String &hash,
                   const String &salt, bool admin)
    : id(AuthUtils::generateUserId()), username(username), passwordHash(hash),
      salt(salt), isAdmin(admin), createdAt(time(nullptr)) {}

String AuthUser::toJson() const {
  JsonDocument doc;
  doc["id"] = id;
  doc["username"] = username;
  doc["passwordHash"] = passwordHash;
  doc["salt"] = salt;
  doc["isAdmin"] = isAdmin;
  doc["createdAt"] = createdAt;

  std::string json;
  serializeJson(doc, json);
  return String(json.c_str());
}

AuthUser AuthUser::fromJson(const String &json) {
  AuthUser user;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json.c_str());

  if (!error) {
    user.id = String(doc["id"].as<std::string>().c_str());
    user.username = String(doc["username"].as<std::string>().c_str());
    user.passwordHash = String(doc["passwordHash"].as<std::string>().c_str());
    user.salt = String(doc["salt"].as<std::string>().c_str());
    user.isAdmin = doc["isAdmin"].as<bool>(); // Default to false if not present
    user.createdAt = doc["createdAt"].as<unsigned long>();
  }

  return user;
}

bool AuthUser::isValid() const {
  return id.length() > 0 && username.length() > 0 && passwordHash.length() > 0;
}