#include "auth/auth_constants.h"
#include "auth/auth_utils.h"
#include "models/data_models.h"
#include <ArduinoJson.h>
#include <string>
#include <time.h>

// AuthPageToken implementation
AuthPageToken::AuthPageToken(const String &token, const String &clientIp)
    : id(AuthUtils::generateUserId()), token(token), clientIp(clientIp),
      createdAt(time(nullptr)),
      expiresAt(time(nullptr) +
                (AuthConstants::PAGE_TOKEN_DURATION_MS / 1000)) {}
                
String AuthPageToken::toJson() const {
  JsonDocument doc;
  doc["id"] = id;
  doc["token"] = token;
  doc["clientIp"] = clientIp;
  doc["createdAt"] = createdAt;
  doc["expiresAt"] = expiresAt;

  std::string json;
  serializeJson(doc, json);
  return String(json.c_str());
}

AuthPageToken AuthPageToken::fromJson(const String &json) {
  AuthPageToken pageToken;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json.c_str());
  if (!error) {
    pageToken.id = String(doc["id"].as<std::string>().c_str());
    pageToken.token = String(doc["token"].as<std::string>().c_str());
    pageToken.clientIp = String(doc["clientIp"].as<std::string>().c_str());
    pageToken.createdAt = doc["createdAt"].as<unsigned long>();
    pageToken.expiresAt = doc["expiresAt"].as<unsigned long>();
  }

  return pageToken;
}
bool AuthPageToken::isValid() const {
  return id.length() > 0 && token.length() > 0 && clientIp.length() > 0 &&
         time(nullptr) < expiresAt;
}