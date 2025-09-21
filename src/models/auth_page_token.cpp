#include "auth/auth_constants.h"
#include "auth/auth_utils.h"
#include "models/data_models.h"
#include <ArduinoJson.h>
#include <time.h>

// AuthPageToken implementation
AuthPageToken::AuthPageToken(const String &token, const String &clientIp)
    : id(AuthUtils::generateUserId()), token(token), clientIp(clientIp),
      createdAt(time(nullptr)),
      expiresAt(time(nullptr) +
                (AuthConstants::PAGE_TOKEN_DURATION_MS / 1000)) {}
                
String AuthPageToken::toJson() const {
  DynamicJsonDocument doc(512);
  doc["id"] = id;
  doc["token"] = token;
  doc["clientIp"] = clientIp;
  doc["createdAt"] = createdAt;
  doc["expiresAt"] = expiresAt;

  String json;
  serializeJson(doc, json);
  return json;
}

AuthPageToken AuthPageToken::fromJson(const String &json) {
  AuthPageToken pageToken;

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, json);
  if (!error) {
    pageToken.id = doc["id"].as<String>();
    pageToken.token = doc["token"].as<String>();
    pageToken.clientIp = doc["clientIp"].as<String>();
    pageToken.createdAt = doc["createdAt"].as<unsigned long>();
    pageToken.expiresAt = doc["expiresAt"].as<unsigned long>();
  }

  return pageToken;
}
bool AuthPageToken::isValid() const {
  return id.length() > 0 && token.length() > 0 && clientIp.length() > 0 &&
         time(nullptr) < expiresAt;
}