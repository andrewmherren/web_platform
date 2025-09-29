#include "storage/auth_storage.h"
#include "auth/auth_constants.h"
#include "auth/auth_utils.h"
#include "models/data_models.h"
#include "utilities/debug_macros.h"

#include <ArduinoJson.h>

// Initialize static members
bool AuthStorage::initialized = false;
String AuthStorage::driverName = "";

// Collection names
const String AuthStorage::USERS_COLLECTION = "users";
const String AuthStorage::SESSIONS_COLLECTION = "sessions";
const String AuthStorage::API_TOKENS_COLLECTION = "api_tokens";
const String AuthStorage::PAGE_TOKENS_COLLECTION = "page_tokens";

void AuthStorage::ensureInitialized() {
  if (!initialized) {
    initialize();
  }
}

void AuthStorage::createDefaultAdminUser() {
  // Check if any users exist
  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  std::vector<String> userKeys = driver->listKeys(USERS_COLLECTION);

  if (userKeys.empty()) {
    // No users exist, create default admin
    String salt = AuthUtils::generateSalt();
    String hash =
        AuthUtils::hashPassword(AuthConstants::DEFAULT_ADMIN_PASSWORD, salt);

    AuthUser admin(AuthConstants::DEFAULT_ADMIN_USERNAME, hash, salt);

    bool stored = driver->store(USERS_COLLECTION, admin.id, admin.toJson());

    if (stored) {
      DEBUG_PRINTF("AuthStorage: Created default admin user (ID: %s)\n",
                   admin.id.c_str());
    } else {
      ERROR_PRINTLN("AuthStorage: ERROR - Failed to create default admin user");
    }
  }
}

void AuthStorage::cleanExpiredData() {
  cleanExpiredSessions();
  cleanExpiredApiTokens();
  cleanExpiredPageTokens();
}

void AuthStorage::initialize(const String &driver) {
  if (initialized) {
    return;
  }

  driverName = driver;

  // Initialize storage manager (ensures JSON driver exists)
  if (driverName.length() > 0) {
    IDatabaseDriver *targetDriver = &StorageManager::driver(driverName);
    if (!targetDriver) {
      DEBUG_PRINTF(
          "AuthStorage: Warning - driver '%s' not found, using default\n",
          driverName.c_str());
      driverName = "";
    }
  }

  // Create default admin user if needed
  createDefaultAdminUser();

  initialized = true;

  // Clean expired data
  cleanExpiredData();

  DEBUG_PRINTF("AuthStorage: Initialized with driver '%s'\n",
               driverName.length() > 0 ? driverName.c_str() : "default");
}

// User management

String AuthStorage::createUser(const String &username, const String &password) {
  ensureInitialized();

  if (username.length() == 0 || password.length() == 0) {
    return "";
  }

  // Check if username already exists
  AuthUser existing = findUserByUsername(username);
  if (existing.isValid()) {
    DEBUG_PRINTF("AuthStorage: User '%s' already exists\n", username.c_str());
    return "";
  }

  // Create new user
  String salt = AuthUtils::generateSalt();
  String hash = AuthUtils::hashPassword(password, salt);
  AuthUser user(username, hash, salt);

  IDatabaseDriver *driver = &StorageManager::driver(driverName);

  if (driver->store(USERS_COLLECTION, user.id, user.toJson())) {
    DEBUG_PRINTF("AuthStorage: Created user '%s' (ID: %s)\n", username.c_str(),
                 user.id.c_str());
    return user.id;
  }

  return "";
}

AuthUser AuthStorage::findUserById(const String &userId) {
  ensureInitialized();

  if (userId.length() == 0) {
    return AuthUser();
  }

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  String userData = driver->retrieve(USERS_COLLECTION, userId);

  if (userData.length() > 0) {
    return AuthUser::fromJson(userData);
  }

  return AuthUser();
}

AuthUser AuthStorage::findUserByUsername(const String &username) {
  ensureInitialized();

  if (username.length() == 0) {
    return AuthUser();
  }

  // Use QueryBuilder to find by username
  QueryBuilder query = StorageManager::query(USERS_COLLECTION);
  if (driverName.length() > 0) {
    query = QueryBuilder(&StorageManager::driver(driverName), USERS_COLLECTION);
  }

  String userData = query.where("username", username).get();

  if (userData.length() > 0) {
    return AuthUser::fromJson(userData);
  }

  return AuthUser();
}

bool AuthStorage::updateUserPassword(const String &userId,
                                     const String &newPassword) {
  ensureInitialized();

  if (userId.length() == 0 || newPassword.length() == 0) {
    return false;
  }

  AuthUser user = findUserById(userId);
  if (!user.isValid()) {
    return false;
  }

  // Update password
  user.salt = AuthUtils::generateSalt();
  user.passwordHash = AuthUtils::hashPassword(newPassword, user.salt);

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  bool success = driver->store(USERS_COLLECTION, userId, user.toJson());

  if (success) {
    DEBUG_PRINTF("AuthStorage: Updated password for user ID %s\n",
                 userId.c_str());
  }

  return success;
}

bool AuthStorage::deleteUser(const String &userId) {
  ensureInitialized();

  if (userId.length() == 0) {
    return false;
  }

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  bool success = driver->remove(USERS_COLLECTION, userId);

  if (success) {
    DEBUG_PRINTF("AuthStorage: Deleted user ID %s\n", userId.c_str());

    // Clean up user's sessions and tokens
    // Use QueryBuilder to remove by userId
    QueryBuilder sessionsQuery = StorageManager::query(SESSIONS_COLLECTION);
    if (driverName.length() > 0) {
      sessionsQuery = QueryBuilder(&StorageManager::driver(driverName),
                                   SESSIONS_COLLECTION);
    }
    sessionsQuery.where("userId", userId).remove();

    QueryBuilder tokensQuery = StorageManager::query(API_TOKENS_COLLECTION);
    if (driverName.length() > 0) {
      tokensQuery = QueryBuilder(&StorageManager::driver(driverName),
                                 API_TOKENS_COLLECTION);
    }
    tokensQuery.where("userId", userId).remove();
  }

  return success;
}

String AuthStorage::validateCredentials(const String &username,
                                        const String &password) {
  ensureInitialized();

  AuthUser user = findUserByUsername(username);
  if (!user.isValid()) {
    return "";
  }

  if (AuthUtils::verifyPassword(password, user.passwordHash, user.salt)) {
    return user.id;
  }

  return "";
}

std::vector<AuthUser> AuthStorage::getAllUsers() {
  ensureInitialized();

  std::vector<AuthUser> users;

  // Use QueryBuilder to get all users
  QueryBuilder query = StorageManager::query(USERS_COLLECTION);
  if (driverName.length() > 0) {
    query = QueryBuilder(&StorageManager::driver(driverName), USERS_COLLECTION);
  }

  std::vector<String> userDataList = query.getAll();

  for (const String &userData : userDataList) {
    AuthUser user = AuthUser::fromJson(userData);
    if (user.isValid()) {
      users.push_back(user);
    }
  }

  return users;
}

// Session management

String AuthStorage::createSession(const String &userId) {
  ensureInitialized();

  if (userId.length() == 0) {
    return "";
  }

  // Get user to verify it exists and get username
  AuthUser user = findUserById(userId);
  if (!user.isValid()) {
    return "";
  }

  String sessionId = "sess_" + AuthUtils::generateSecureToken();
  AuthSession session(sessionId, userId, user.username);

  IDatabaseDriver *driver = &StorageManager::driver(driverName);

  if (driver->store(SESSIONS_COLLECTION, sessionId, session.toJson())) {
    return sessionId;
  }

  return "";
}

AuthSession AuthStorage::findSession(const String &sessionId) {
  ensureInitialized();

  if (sessionId.length() == 0) {
    return AuthSession();
  }

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  String sessionData = driver->retrieve(SESSIONS_COLLECTION, sessionId);

  if (sessionData.length() > 0) {
    return AuthSession::fromJson(sessionData);
  }

  return AuthSession();
}

String AuthStorage::validateSession(const String &sessionId,
                                    const String &clientIp) {
  ensureInitialized();

  if (sessionId.isEmpty()) {
    return "";
  }

  AuthSession session = findSession(sessionId);
  if (!session.isValid()) {
    // Clean up expired session
    if (session.id.length() > 0) {
      deleteSession(sessionId);
    }
    return "";
  }

  // Update expiration time to extend the session
  unsigned long now = time(nullptr);
  session.expiresAt = now + (AuthConstants::SESSION_DURATION_MS / 1000);

  // Store the updated session
  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  driver->store(SESSIONS_COLLECTION, sessionId, session.toJson());

  return session.userId;
}

bool AuthStorage::deleteSession(const String &sessionId) {
  ensureInitialized();

  if (sessionId.length() == 0) {
    return false;
  }

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  return driver->remove(SESSIONS_COLLECTION, sessionId);
}

int AuthStorage::cleanExpiredSessions() {
  ensureInitialized();

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  std::vector<String> sessionKeys = driver->listKeys(SESSIONS_COLLECTION);
  int cleaned = 0;

  for (const String &key : sessionKeys) {
    String sessionData = driver->retrieve(SESSIONS_COLLECTION, key);
    if (sessionData.length() > 0) {
      AuthSession session = AuthSession::fromJson(sessionData);
      if (!session.isValid()) {
        if (driver->remove(SESSIONS_COLLECTION, key)) {
          cleaned++;
        }
      }
    }
  }

  if (cleaned > 0) {
    DEBUG_PRINTF("AuthStorage: Cleaned %d expired sessions\n", cleaned);
  }

  return cleaned;
}

// API Token management

String AuthStorage::createApiToken(const String &userId, const String &name,
                                   unsigned long expireInDays) {
  ensureInitialized();

  if (userId.length() == 0 || name.length() == 0) {
    return "";
  }

  // Get user to verify it exists and get username
  AuthUser user = findUserById(userId);
  if (!user.isValid()) {
    return "";
  }
  String token = "tok_" + AuthUtils::generateSecureToken(32);
  AuthApiToken apiToken(token, userId, user.username, name, expireInDays);

  IDatabaseDriver *driver = &StorageManager::driver(driverName);

  if (driver->store(API_TOKENS_COLLECTION, apiToken.id, apiToken.toJson())) {
    return token;
  }

  return "";
}
AuthApiToken AuthStorage::findApiToken(const String &token) {
  ensureInitialized();

  if (token.length() == 0) {
    return AuthApiToken();
  }

  // Using QueryBuilder to find by token value
  QueryBuilder query = StorageManager::query(API_TOKENS_COLLECTION);
  if (driverName.length() > 0) {
    query = QueryBuilder(&StorageManager::driver(driverName),
                         API_TOKENS_COLLECTION);
  }

  String tokenData = query.where("token", token).get();

  if (tokenData.length() > 0) {
    return AuthApiToken::fromJson(tokenData);
  }

  return AuthApiToken();
}

String AuthStorage::validateApiToken(const String &token) {
  ensureInitialized();

  AuthApiToken apiToken = findApiToken(token);
  if (!apiToken.isValid()) {
    // Clean up expired token
    if (apiToken.token.length() > 0) {
      deleteApiToken(token);
    }
    return "";
  }

  return apiToken.userId;
}
bool AuthStorage::deleteApiToken(const String &token) {
  ensureInitialized();

  if (token.length() == 0) {
    return false;
  }

  // Find the token first to get its ID
  AuthApiToken apiToken = findApiToken(token);
  if (!apiToken.isValid()) {
    return false;
  }

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  return driver->remove(API_TOKENS_COLLECTION, apiToken.id);
}

std::vector<AuthApiToken> AuthStorage::getUserApiTokens(const String &userId) {
  ensureInitialized();

  std::vector<AuthApiToken> tokens;

  if (userId.length() == 0) {
    return tokens;
  }

  // Use QueryBuilder to find by userId
  QueryBuilder query = StorageManager::query(API_TOKENS_COLLECTION);
  if (driverName.length() > 0) {
    query = QueryBuilder(&StorageManager::driver(driverName),
                         API_TOKENS_COLLECTION);
  }

  std::vector<String> tokenDataList = query.where("userId", userId).getAll();

  for (const String &tokenData : tokenDataList) {
    AuthApiToken token = AuthApiToken::fromJson(tokenData);
    if (token.isValid()) {
      tokens.push_back(token);
    }
  }

  return tokens;
}

int AuthStorage::cleanExpiredApiTokens() {
  ensureInitialized();

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  std::vector<String> tokenKeys = driver->listKeys(API_TOKENS_COLLECTION);
  int cleaned = 0;

  for (const String &key : tokenKeys) {
    String tokenData = driver->retrieve(API_TOKENS_COLLECTION, key);
    if (tokenData.length() > 0) {
      AuthApiToken token = AuthApiToken::fromJson(tokenData);
      if (!token.isValid()) {
        if (driver->remove(API_TOKENS_COLLECTION, key)) {
          cleaned++;
        }
      }
    }
  }

  if (cleaned > 0) {
    DEBUG_PRINTF("AuthStorage: Cleaned %d expired API tokens\n", cleaned);
  }

  return cleaned;
}

// Page Token management
String AuthStorage::createPageToken(const String &clientIp) {
  ensureInitialized();

  if (clientIp.length() == 0) {
    return "";
  }

  String token = AuthUtils::generatePageToken();
  AuthPageToken pageToken(token, clientIp);

  IDatabaseDriver *driver = &StorageManager::driver(driverName);

  if (driver->store(PAGE_TOKENS_COLLECTION, pageToken.id, pageToken.toJson())) {
    return token;
  }

  return "";
}

bool AuthStorage::validatePageToken(const String &token,
                                    const String &clientIp) {
  ensureInitialized();

  if (token.length() == 0 || clientIp.length() == 0) {
    return false;
  }

  // Use QueryBuilder to find by token value
  QueryBuilder query = StorageManager::query(PAGE_TOKENS_COLLECTION);
  if (driverName.length() > 0) {
    query = QueryBuilder(&StorageManager::driver(driverName),
                         PAGE_TOKENS_COLLECTION);
  }

  String tokenData = query.where("token", token).get();

  if (tokenData.length() == 0) {
    DEBUG_PRINTF("PageToken validation failed: token '%s...' not found\n",
                 token.substring(0, 6).c_str());
    return false;
  }

  AuthPageToken pageToken = AuthPageToken::fromJson(tokenData);
  if (!pageToken.isValid()) {
    DEBUG_PRINTF("PageToken validation failed: token '%s...' expired\n",
                 token.substring(0, 6).c_str());
    IDatabaseDriver *driver = &StorageManager::driver(driverName);
    driver->remove(PAGE_TOKENS_COLLECTION, pageToken.id);
    return false;
  }

  if (pageToken.clientIp != clientIp) {
    DEBUG_PRINTF("PageToken IP mismatch: token IP '%s' vs request IP '%s'\n",
                 pageToken.clientIp.c_str(), clientIp.c_str());
    return false;
  }

  return true;
}

int AuthStorage::cleanExpiredPageTokens() {
  ensureInitialized();

  IDatabaseDriver *driver = &StorageManager::driver(driverName);
  std::vector<String> tokenKeys = driver->listKeys(PAGE_TOKENS_COLLECTION);
  int cleaned = 0;

  for (const String &key : tokenKeys) {
    String tokenData = driver->retrieve(PAGE_TOKENS_COLLECTION, key);
    if (tokenData.length() > 0) {
      AuthPageToken token = AuthPageToken::fromJson(tokenData);
      if (!token.isValid()) {
        if (driver->remove(PAGE_TOKENS_COLLECTION, key)) {
          cleaned++;
        }
      }
    }
  }

  if (cleaned > 0) {
    DEBUG_PRINTF("AuthStorage: Cleaned %d expired page tokens\n", cleaned);
  }

  return cleaned;
}

// Utility methods

String AuthStorage::getDriverName() {
  return driverName.length() > 0 ? driverName : "default";
}

int AuthStorage::cleanupExpiredData() {
  ensureInitialized();

  int total = 0;
  total += cleanExpiredSessions();
  total += cleanExpiredApiTokens();
  total += cleanExpiredPageTokens();

  if (total > 0) {
    DEBUG_PRINTF("AuthStorage: Total cleanup - removed %d expired records\n",
                 total);
  }

  return total;
}

String AuthStorage::getStorageStats() {
  ensureInitialized();

  DynamicJsonDocument doc(512);

  IDatabaseDriver *driver = &StorageManager::driver(driverName);

  doc["driver"] = getDriverName();
  doc["users"] = driver->listKeys(USERS_COLLECTION).size();
  doc["sessions"] = driver->listKeys(SESSIONS_COLLECTION).size();
  doc["api_tokens"] = driver->listKeys(API_TOKENS_COLLECTION).size();
  doc["page_tokens"] = driver->listKeys(PAGE_TOKENS_COLLECTION).size();

  String stats;
  serializeJson(doc, stats);
  return stats;
}