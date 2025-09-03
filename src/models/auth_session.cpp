#include "../../include/models/data_models.h"
#include "../../include/auth/auth_constants.h"
#include <ArduinoJson.h>

// AuthSession implementation
AuthSession::AuthSession(const String& sessionId, const String& userId, const String& username) 
    : id(sessionId), userId(userId), username(username), createdAt(millis()),
      expiresAt(millis() + AuthConstants::SESSION_DURATION_MS) {
}

String AuthSession::toJson() const {
    DynamicJsonDocument doc(512);
    doc["id"] = id;
    doc["userId"] = userId;
    doc["username"] = username;
    doc["createdAt"] = createdAt;
    doc["expiresAt"] = expiresAt;
    
    String json;
    serializeJson(doc, json);
    return json;
}

AuthSession AuthSession::fromJson(const String& json) {
    AuthSession session;
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, json);
    
    if (!error) {
        session.id = doc["id"].as<String>();
        session.userId = doc["userId"].as<String>();
        session.username = doc["username"].as<String>();
        session.createdAt = doc["createdAt"].as<unsigned long>();
        session.expiresAt = doc["expiresAt"].as<unsigned long>();
    }
    
    return session;
}

bool AuthSession::isValid() const {
    return id.length() > 0 && userId.length() > 0 && millis() < expiresAt;
}