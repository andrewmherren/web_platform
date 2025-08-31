#ifndef AUTH_STORAGE_H
#define AUTH_STORAGE_H

#include "auth_constants.h"
#include <Arduino.h>
#include <map>
#include <vector>

// User model
struct User {
  String username;
  String passwordHash;
  String salt;
  unsigned long createdAt;

  User() : createdAt(0) {}
  User(const String &name, const String &hash, const String &s)
      : username(name), passwordHash(hash), salt(s), createdAt(millis()) {}
};

// Session model
struct AuthSession {
  String id;
  String username;
  unsigned long createdAt;
  unsigned long expiresAt;

  AuthSession() : createdAt(0), expiresAt(0) {}
  AuthSession(const String &sessionId, const String &user)
      : id(sessionId), username(user), createdAt(millis()),
        expiresAt(millis() + AuthConstants::SESSION_DURATION_MS) {}

  bool isValid() const { return millis() < expiresAt; }
};

// API Token model
struct ApiToken {
  String token;
  String username;
  String name;
  unsigned long createdAt;
  unsigned long expiresAt; // 0 = never expires

  ApiToken() : createdAt(0), expiresAt(0) {}
  ApiToken(const String &t, const String &user, const String &tokenName)
      : token(t), username(user), name(tokenName), createdAt(millis()),
        expiresAt(0) {}
  
  // Constructor with expiration
  ApiToken(const String &t, const String &user, const String &tokenName, unsigned long expireInDays)
      : token(t), username(user), name(tokenName), createdAt(millis()),
        expiresAt(expireInDays > 0 ? millis() + (expireInDays * 24 * 60 * 60 * 1000) : 0) {}

  bool isValid() const { return expiresAt == 0 || millis() < expiresAt; }
  
  // Get expiration days remaining (0 for never expires, negative for expired)
  float getExpirationDaysRemaining() const {
    if (expiresAt == 0) return 0; // Never expires
    if (!isValid()) return -1; // Already expired
    
    unsigned long remainingMs = expiresAt - millis();
    return remainingMs / (24.0f * 60 * 60 * 1000);
  }
};

// Page Token model (for CSRF protection)
struct PageToken {
  String token;
  String clientIp;
  unsigned long createdAt;
  unsigned long expiresAt;

  PageToken() : createdAt(0), expiresAt(0) {}
  PageToken(const String &t, const String &ip)
      : token(t), clientIp(ip), createdAt(millis()),
        expiresAt(millis() + AuthConstants::PAGE_TOKEN_DURATION_MS) {}

  bool isValid() const { return millis() < expiresAt; }
};

// Authentication storage class
class AuthStorage {
private:
  static std::vector<User> users;
  static std::vector<AuthSession> sessions;
  static std::vector<ApiToken> apiTokens;

private:
  static std::map<String, PageToken> pageTokens;
  // Has initialization happened
  static bool initialized;

  // Load and save data methods
  static void loadFromPreferences();
  static void saveToPreferences();

public:
  // Initialize with default admin account
  static void initialize();

  // User management
  static bool addUser(const String &username, const String &password);
  static bool updateUserPassword(const String &username,
                                 const String &newPassword);
  static bool deleteUser(const String &username);
  static User *findUser(const String &username);
  static bool validateCredentials(const String &username,
                                  const String &password);

  // Session management
  static String createSession(const String &username);
  static AuthSession *findSession(const String &sessionId);
  static bool validateSession(const String &sessionId);
  static bool deleteSession(const String &sessionId);
  static void cleanExpiredSessions();

  // API Token management
  static String createApiToken(const String &username, const String &name, unsigned long expireInDays = 0);
  static ApiToken *findApiToken(const String &token);
  static bool validateApiToken(const String &token);
  static bool deleteApiToken(const String &token);
  static std::vector<ApiToken> getUserTokens(const String &username);
  static void cleanExpiredTokens();

  // Page Token management (CSRF)
  static String createPageToken(const String &clientIp);
  static bool validatePageToken(const String &token, const String &clientIp);
  static void cleanExpiredPageTokens();
};

#endif // AUTH_STORAGE_H