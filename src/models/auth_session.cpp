#include "auth/auth_constants.h"
#include "models/data_models.h"
#include <ArduinoJson.h>
#include <string>
#include <time.h>

// AuthSession implementation
AuthSession::AuthSession(const String &sessionId, const String &userId,
                         const String &username)
    : id(sessionId), userId(userId), username(username),
      createdAt(time(nullptr)),
      expiresAt(time(nullptr) + (AuthConstants::SESSION_DURATION_MS / 1000)) {}

String AuthSession::toJson() const {
  JsonDocument doc;
  doc["id"] = id;
  doc["userId"] = userId;
  doc["username"] = username;
  doc["createdAt"] = createdAt;
  doc["expiresAt"] = expiresAt;

  std::string json;
  serializeJson(doc, json);
  return String(json.c_str());
}

AuthSession AuthSession::fromJson(const String &json) {
  AuthSession session;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json.c_str());

  if (!error) {
    session.id = String(doc["id"].as<std::string>().c_str());
    session.userId = String(doc["userId"].as<std::string>().c_str());
    session.username = String(doc["username"].as<std::string>().c_str());
    session.createdAt = doc["createdAt"].as<unsigned long>();
    session.expiresAt = doc["expiresAt"].as<unsigned long>();
  }

  return session;
}

bool AuthSession::isValid() const {
  return id.length() > 0 && userId.length() > 0 && time(nullptr) < expiresAt;
}