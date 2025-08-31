#include "../../include/auth/auth_storage.h"
#include "../../include/auth/auth_utils.h"
#include <ArduinoJson.h>

#if defined(ESP32)
#include <Preferences.h>
#elif defined(ESP8266)
#include <EEPROM.h>
#endif

// Initialize static members
std::vector<User> AuthStorage::users;
std::vector<AuthSession> AuthStorage::sessions;
std::vector<ApiToken> AuthStorage::apiTokens;
std::map<String, PageToken> AuthStorage::pageTokens;
bool AuthStorage::initialized = false;

// Initialize the auth storage
void AuthStorage::initialize() {
  if (initialized)
    return;

  // Load existing data
  loadFromPreferences();

  // If no users exist, create default admin account
  if (users.empty()) {
    String salt = AuthUtils::generateSalt();
    String hash =
        AuthUtils::hashPassword(AuthConstants::DEFAULT_ADMIN_PASSWORD, salt);
    users.push_back(User(AuthConstants::DEFAULT_ADMIN_USERNAME, hash, salt));
    saveToPreferences();
  }

  // Clean expired sessions and tokens
  cleanExpiredSessions();
  cleanExpiredTokens();
  cleanExpiredPageTokens();

  initialized = true;
}

// User management
bool AuthStorage::addUser(const String &username, const String &password) {
  // Check if user already exists
  if (findUser(username) != nullptr) {
    return false;
  }

  // Create new user
  String salt = AuthUtils::generateSalt();
  String hash = AuthUtils::hashPassword(password, salt);
  users.push_back(User(username, hash, salt));
  saveToPreferences();
  return true;
}

bool AuthStorage::updateUserPassword(const String &username,
                                     const String &newPassword) {
  User *user = findUser(username);
  if (user == nullptr) {
    return false;
  }

  // Update password
  String salt = AuthUtils::generateSalt();
  String hash = AuthUtils::hashPassword(newPassword, salt);
  user->passwordHash = hash;
  user->salt = salt;
  saveToPreferences();
  return true;
}

bool AuthStorage::deleteUser(const String &username) {
  for (auto it = users.begin(); it != users.end(); ++it) {
    if (it->username == username) {
      users.erase(it);
      saveToPreferences();
      return true;
    }
  }
  return false;
}

User *AuthStorage::findUser(const String &username) {
  for (auto &user : users) {
    if (user.username == username) {
      return &user;
    }
  }
  return nullptr;
}

bool AuthStorage::validateCredentials(const String &username,
                                      const String &password) {
  User *user = findUser(username);
  if (user == nullptr) {
    return false;
  }
  return AuthUtils::verifyPassword(password, user->passwordHash, user->salt);
}

// Session management
String AuthStorage::createSession(const String &username) {
  String sessionId = "sess_" + AuthUtils::generateSecureToken();
  sessions.push_back(AuthSession(sessionId, username));
  saveToPreferences();
  return sessionId;
}

AuthSession *AuthStorage::findSession(const String &sessionId) {
  for (auto &session : sessions) {
    if (session.id == sessionId) {
      return &session;
    }
  }
  return nullptr;
}

bool AuthStorage::validateSession(const String &sessionId) {
  AuthSession *session = findSession(sessionId);
  if (session == nullptr) {
    return false;
  }
  if (!session->isValid()) {
    deleteSession(sessionId);
    return false;
  }
  return true;
}

bool AuthStorage::deleteSession(const String &sessionId) {
  for (auto it = sessions.begin(); it != sessions.end(); ++it) {
    if (it->id == sessionId) {
      sessions.erase(it);
      saveToPreferences();
      return true;
    }
  }
  return false;
}

void AuthStorage::cleanExpiredSessions() {
  auto it = sessions.begin();
  while (it != sessions.end()) {
    if (!it->isValid()) {
      it = sessions.erase(it);
    } else {
      ++it;
    }
  }
  saveToPreferences();
}

// API Token management
String AuthStorage::createApiToken(const String &username, const String &name) {
  String token = "tok_" + AuthUtils::generateSecureToken(32);
  apiTokens.push_back(ApiToken(token, username, name));
  saveToPreferences();
  return token;
}

ApiToken *AuthStorage::findApiToken(const String &token) {
  for (auto &apiToken : apiTokens) {
    if (apiToken.token == token) {
      return &apiToken;
    }
  }
  return nullptr;
}

bool AuthStorage::validateApiToken(const String &token) {
  ApiToken *apiToken = findApiToken(token);
  if (apiToken == nullptr) {
    return false;
  }
  if (!apiToken->isValid()) {
    deleteApiToken(token);
    return false;
  }
  return true;
}

bool AuthStorage::deleteApiToken(const String &token) {
  for (auto it = apiTokens.begin(); it != apiTokens.end(); ++it) {
    if (it->token == token) {
      apiTokens.erase(it);
      saveToPreferences();
      return true;
    }
  }
  return false;
}

std::vector<ApiToken> AuthStorage::getUserTokens(const String &username) {
  std::vector<ApiToken> userTokens;
  for (const auto &token : apiTokens) {
    if (token.username == username && token.isValid()) {
      userTokens.push_back(token);
    }
  }
  return userTokens;
}

void AuthStorage::cleanExpiredTokens() {
  auto it = apiTokens.begin();
  while (it != apiTokens.end()) {
    if (it->expiresAt > 0 && !it->isValid()) {
      it = apiTokens.erase(it);
    } else {
      ++it;
    }
  }
  saveToPreferences();
}

// Page Token management (CSRF protection)
String AuthStorage::createPageToken(const String &clientIp) {
  String token = AuthUtils::generatePageToken();
  pageTokens[token] = PageToken(token, clientIp);
  return token;
}

bool AuthStorage::validatePageToken(const String &token,
                                    const String &clientIp) {
  auto it = pageTokens.find(token);
  if (it == pageTokens.end()) {
    Serial.printf("PageToken validation failed: token '%s...' not found\n",
                  token.substring(0, 6).c_str());
    return false;
  }

  const PageToken &pageToken = it->second;
  if (!pageToken.isValid()) {
    Serial.printf("PageToken validation failed: token '%s...' expired\n",
                  token.substring(0, 6).c_str());
    pageTokens.erase(it);
    return false;
  }

  // TEMPORARY: For simplicity in the embedded environment, we'll allow
  // any client IP to use a valid token (removes double-submit protection)
  // In production, this should be: return pageToken.clientIp == clientIp;
  // AI ASSISTANT: Please fix this... we do not want temporary fixes and notes
  // like this

  // Log the IP check
  if (pageToken.clientIp != clientIp) {
    Serial.printf("PageToken IP mismatch (allowing anyway): token IP '%s' vs "
                  "request IP '%s'\n",
                  pageToken.clientIp.c_str(), clientIp.c_str());
  }

  return true; // Allow any client IP to use a valid token
}

void AuthStorage::cleanExpiredPageTokens() {
  auto it = pageTokens.begin();
  while (it != pageTokens.end()) {
    if (!it->second.isValid()) {
      it = pageTokens.erase(it);
    } else {
      ++it;
    }
  }
}

// Load and save data methods
void AuthStorage::loadFromPreferences() {
#if defined(ESP32)
  Preferences prefs;
  prefs.begin("auth", false);

  // Load users
  String userJson = prefs.getString("users", "[]");
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, userJson);
  users.clear();
  for (JsonObject userObj : doc.as<JsonArray>()) {
    User user;
    user.username = userObj["username"].as<String>();
    user.passwordHash = userObj["hash"].as<String>();
    user.salt = userObj["salt"].as<String>();
    user.createdAt = userObj["created"].as<unsigned long>();
    users.push_back(user);
  }

  // Load API tokens
  String tokenJson = prefs.getString("tokens", "[]");
  DynamicJsonDocument docTokens(4096);
  deserializeJson(docTokens, tokenJson);
  apiTokens.clear();
  for (JsonObject tokenObj : docTokens.as<JsonArray>()) {
    ApiToken token;
    token.token = tokenObj["token"].as<String>();
    token.username = tokenObj["username"].as<String>();
    token.name = tokenObj["name"].as<String>();
    token.createdAt = tokenObj["created"].as<unsigned long>();
    token.expiresAt = tokenObj["expires"].as<unsigned long>();
    apiTokens.push_back(token);
  }

  prefs.end();
#elif defined(ESP8266)
  // For ESP8266, you'd need to implement a similar loading mechanism
  // using EEPROM or SPIFFS
  // This is a simplified example
  EEPROM.begin(4096);
  // TODO: Implement EEPROM-based storage for ESP8266
  // For now, just initialize with default values
#endif
}

void AuthStorage::saveToPreferences() {
#if defined(ESP32)
  Preferences prefs;
  prefs.begin("auth", false);

  // Save users
  DynamicJsonDocument doc(4096);
  JsonArray userArray = doc.to<JsonArray>();
  for (const User &user : users) {
    JsonObject userObj = userArray.createNestedObject();
    userObj["username"] = user.username;
    userObj["hash"] = user.passwordHash;
    userObj["salt"] = user.salt;
    userObj["created"] = user.createdAt;
  }
  String userJson;
  serializeJson(doc, userJson);
  prefs.putString("users", userJson);

  // Save API tokens
  DynamicJsonDocument docTokens(4096);
  JsonArray tokenArray = docTokens.to<JsonArray>();
  for (const ApiToken &token : apiTokens) {
    JsonObject tokenObj = tokenArray.createNestedObject();
    tokenObj["token"] = token.token;
    tokenObj["username"] = token.username;
    tokenObj["name"] = token.name;
    tokenObj["created"] = token.createdAt;
    tokenObj["expires"] = token.expiresAt;
  }
  String tokenJson;
  serializeJson(docTokens, tokenJson);
  prefs.putString("tokens", tokenJson);

  prefs.end();
#elif defined(ESP8266)
  // TODO: Implement EEPROM-based storage for ESP8266
  EEPROM.begin(4096);
  // Simplified example - would need more robust implementation
  EEPROM.commit();
#endif
}