#include "../../include/models/data_models.h"
#include <ArduinoJson.h>

// AuthApiToken implementation
AuthApiToken::AuthApiToken(const String& token, const String& userId, const String& username,
                           const String& name, unsigned long expireInDays)
    : token(token), userId(userId), username(username), name(name), createdAt(millis()),
      expiresAt(expireInDays > 0 ? millis() + (expireInDays * 24 * 60 * 60 * 1000) : 0) {
}

String AuthApiToken::toJson() const {
    DynamicJsonDocument doc(512);
    doc["token"] = token;
    doc["userId"] = userId;
    doc["username"] = username;
    doc["name"] = name;
    doc["createdAt"] = createdAt;
    doc["expiresAt"] = expiresAt;
    
    String json;
    serializeJson(doc, json);
    return json;
}

AuthApiToken AuthApiToken::fromJson(const String& json) {
    AuthApiToken apiToken;
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (!error) {
        apiToken.token = doc["token"].as<String>();
        apiToken.userId = doc["userId"].as<String>();
        apiToken.username = doc["username"].as<String>();
        apiToken.name = doc["name"].as<String>();
        apiToken.createdAt = doc["createdAt"].as<unsigned long>();
        apiToken.expiresAt = doc["expiresAt"].as<unsigned long>();
    }
    
    return apiToken;
}

bool AuthApiToken::isValid() const {
    return token.length() > 0 && userId.length() > 0 && 
           (expiresAt == 0 || millis() < expiresAt);
}

float AuthApiToken::getExpirationDaysRemaining() const {
    if (expiresAt == 0) return 0; // Never expires
    if (!isValid()) return -1; // Already expired
    
    unsigned long remainingMs = expiresAt - millis();
    return remainingMs / (24.0f * 60 * 60 * 1000);
}