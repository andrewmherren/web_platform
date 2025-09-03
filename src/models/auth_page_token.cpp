#include "../../include/models/data_models.h"
#include "../../include/auth/auth_constants.h"
#include <ArduinoJson.h>

// AuthPageToken implementation
AuthPageToken::AuthPageToken(const String& token, const String& clientIp)
    : token(token), clientIp(clientIp), createdAt(millis()),
      expiresAt(millis() + AuthConstants::PAGE_TOKEN_DURATION_MS) {
}

String AuthPageToken::toJson() const {
    DynamicJsonDocument doc(512);
    doc["token"] = token;
    doc["clientIp"] = clientIp;
    doc["createdAt"] = createdAt;
    doc["expiresAt"] = expiresAt;
    
    String json;
    serializeJson(doc, json);
    return json;
}

AuthPageToken AuthPageToken::fromJson(const String& json) {
    AuthPageToken pageToken;
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (!error) {
        pageToken.token = doc["token"].as<String>();
        pageToken.clientIp = doc["clientIp"].as<String>();
        pageToken.createdAt = doc["createdAt"].as<unsigned long>();
        pageToken.expiresAt = doc["expiresAt"].as<unsigned long>();
    }
    
    return pageToken;
}

bool AuthPageToken::isValid() const {
    return token.length() > 0 && clientIp.length() > 0 && millis() < expiresAt;
}