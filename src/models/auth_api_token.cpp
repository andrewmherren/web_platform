#include "auth/auth_utils.h"
#include "models/data_models.h"
#include <ArduinoJson.h>
#include <string>
#include <time.h>

// AuthApiToken implementation
AuthApiToken::AuthApiToken(const String &token, const String &userId,
                           const String &username, const String &name,
                           unsigned long expireInDays)
    : id(AuthUtils::generateUserId()), token(token), userId(userId),
      username(username), name(name), createdAt(time(nullptr)),
      expiresAt(expireInDays > 0 ? time(nullptr) + (expireInDays * 24 * 60 * 60)
                                 : 0) {}
                                 
String AuthApiToken::toJson() const {
  JsonDocument doc;
  doc["id"] = id;
  doc["token"] = token;
  doc["userId"] = userId;
  doc["username"] = username;
  doc["name"] = name;
  doc["createdAt"] = createdAt;
  doc["expiresAt"] = expiresAt;

  std::string json;
  serializeJson(doc, json);
  return String(json.c_str());
}

AuthApiToken AuthApiToken::fromJson(const String &json) {
  AuthApiToken apiToken;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json.c_str());
  if (!error) {
    apiToken.id = String(doc["id"].as<std::string>().c_str());
    apiToken.token = String(doc["token"].as<std::string>().c_str());
    apiToken.userId = String(doc["userId"].as<std::string>().c_str());
    apiToken.username = String(doc["username"].as<std::string>().c_str());
    apiToken.name = String(doc["name"].as<std::string>().c_str());
    apiToken.createdAt = doc["createdAt"].as<unsigned long>();
    apiToken.expiresAt = doc["expiresAt"].as<unsigned long>();
  }

  return apiToken;
}
bool AuthApiToken::isValid() const {
  return id.length() > 0 && token.length() > 0 && userId.length() > 0 &&
         (expiresAt == 0 || time(nullptr) < expiresAt);
}
float AuthApiToken::getExpirationDaysRemaining() const {
  if (expiresAt == 0)
    return 0; // Never expires
  if (!isValid())
    return -1; // Already expired

  unsigned long remainingSec = expiresAt - time(nullptr);
  return remainingSec / (24.0f * 60 * 60);
}