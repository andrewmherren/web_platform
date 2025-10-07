#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <Arduino.h>
#include <utility> // for std::move
#include <vector>

/**
 * Core data models for the storage-based systems
 *
 * These models replace the old struct-based approach with proper
 * JSON serialization for the storage driver system.
 */

// Authentication user model
struct AuthUser {
  String id;               // Primary key: UUID
  String username;         // Display name (can be changed)
  String passwordHash;     // PBKDF2 hash
  String salt;             // Random salt for password hashing
  bool isAdmin;            // Whether user has admin privileges
  unsigned long createdAt; // Creation timestamp

  AuthUser() : isAdmin(false), createdAt(0) {}
  AuthUser(const String &username, const String &hash, const String &salt,
           bool admin = false);

  // Rule of Five: Destructor, copy constructor, copy assignment, move
  // constructor, move assignment
  ~AuthUser() = default;
  AuthUser(const AuthUser &other) = default;
  AuthUser &operator=(const AuthUser &other) = default;

  // Move semantics
  AuthUser(AuthUser &&other) noexcept
      : id(std::move(other.id)), username(std::move(other.username)),
        passwordHash(std::move(other.passwordHash)),
        salt(std::move(other.salt)), isAdmin(other.isAdmin),
        createdAt(other.createdAt) {}

  AuthUser &operator=(AuthUser &&other) noexcept {
    if (this != &other) {
      id = std::move(other.id);
      username = std::move(other.username);
      passwordHash = std::move(other.passwordHash);
      salt = std::move(other.salt);
      isAdmin = other.isAdmin;
      createdAt = other.createdAt;
    }
    return *this;
  }

  // JSON serialization
  String toJson() const;
  static AuthUser fromJson(const String &json);
  bool isValid() const;
};

// Session model for web authentication
struct AuthSession {
  String id;       // Session ID (primary key)
  String userId;   // Foreign key to AuthUser.id
  String username; // Denormalized for quick access
  unsigned long createdAt;
  unsigned long expiresAt;

  AuthSession() : createdAt(0), expiresAt(0) {}
  AuthSession(const String &sessionId, const String &userId,
              const String &username);

  // Rule of Five: Destructor, copy constructor, copy assignment, move
  // constructor, move assignment
  ~AuthSession() = default;
  AuthSession(const AuthSession &other) = default;
  AuthSession &operator=(const AuthSession &other) = default;

  // Move semantics
  AuthSession(AuthSession &&other) noexcept
      : id(std::move(other.id)), userId(std::move(other.userId)),
        username(std::move(other.username)), createdAt(other.createdAt),
        expiresAt(other.expiresAt) {}

  AuthSession &operator=(AuthSession &&other) noexcept {
    if (this != &other) {
      id = std::move(other.id);
      userId = std::move(other.userId);
      username = std::move(other.username);
      createdAt = other.createdAt;
      expiresAt = other.expiresAt;
    }
    return *this;
  }

  // JSON serialization
  String toJson() const;
  static AuthSession fromJson(const String &json);
  bool isValid() const;
};

// API token model for programmatic authentication
struct AuthApiToken {
  String id;       // Primary key: UUID
  String token;    // Token value (unique)
  String userId;   // Foreign key to AuthUser.id
  String username; // Denormalized for quick access
  String name;     // Human-readable token name
  unsigned long createdAt;
  unsigned long expiresAt; // 0 = never expires

  AuthApiToken() : createdAt(0), expiresAt(0) {}
  AuthApiToken(const String &token, const String &userId,
               const String &username, const String &name,
               unsigned long expireInDays = 0);

  // Rule of Five: Destructor, copy constructor, copy assignment, move
  // constructor, move assignment
  ~AuthApiToken() = default;
  AuthApiToken(const AuthApiToken &other) = default;
  AuthApiToken &operator=(const AuthApiToken &other) = default;

  // Move semantics
  AuthApiToken(AuthApiToken &&other) noexcept
      : id(std::move(other.id)), token(std::move(other.token)),
        userId(std::move(other.userId)), username(std::move(other.username)),
        name(std::move(other.name)), createdAt(other.createdAt),
        expiresAt(other.expiresAt) {}

  AuthApiToken &operator=(AuthApiToken &&other) noexcept {
    if (this != &other) {
      id = std::move(other.id);
      token = std::move(other.token);
      userId = std::move(other.userId);
      username = std::move(other.username);
      name = std::move(other.name);
      createdAt = other.createdAt;
      expiresAt = other.expiresAt;
    }
    return *this;
  }

  // JSON serialization
  String toJson() const;
  static AuthApiToken fromJson(const String &json);
  bool isValid() const;

  // Get expiration days remaining (0 for never expires, negative for expired)
  float getExpirationDaysRemaining() const;
};

// Page token model for CSRF protection
struct AuthPageToken {
  String id;       // Primary key: UUID
  String token;    // Token value (unique)
  String clientIp; // IP address token was issued to
  unsigned long createdAt;
  unsigned long expiresAt;

  AuthPageToken() : createdAt(0), expiresAt(0) {}
  AuthPageToken(const String &token, const String &clientIp);

  // Rule of Five: Destructor, copy constructor, copy assignment, move
  // constructor, move assignment
  ~AuthPageToken() = default;
  AuthPageToken(const AuthPageToken &other) = default;
  AuthPageToken &operator=(const AuthPageToken &other) = default;

  // Move semantics
  AuthPageToken(AuthPageToken &&other) noexcept
      : id(std::move(other.id)), token(std::move(other.token)),
        clientIp(std::move(other.clientIp)), createdAt(other.createdAt),
        expiresAt(other.expiresAt) {}

  AuthPageToken &operator=(AuthPageToken &&other) noexcept {
    if (this != &other) {
      id = std::move(other.id);
      token = std::move(other.token);
      clientIp = std::move(other.clientIp);
      createdAt = other.createdAt;
      expiresAt = other.expiresAt;
    }
    return *this;
  }

  // JSON serialization
  String toJson() const;
  static AuthPageToken fromJson(const String &json);
  bool isValid() const;
};

// Configuration model for key/value settings
struct ConfigItem {
  String id;    // Primary key: UUID
  String key;   // Setting name (unique)
  String value; // Setting value (JSON string)
  unsigned long updatedAt;

  ConfigItem() : updatedAt(0) {}
  ConfigItem(const String &key, const String &value);

  // Rule of Five: Destructor, copy constructor, copy assignment, move
  // constructor, move assignment
  ~ConfigItem() = default;
  ConfigItem(const ConfigItem &other) = default;
  ConfigItem &operator=(const ConfigItem &other) = default;

  // Move semantics
  ConfigItem(ConfigItem &&other) noexcept
      : id(std::move(other.id)), key(std::move(other.key)),
        value(std::move(other.value)), updatedAt(other.updatedAt) {}

  ConfigItem &operator=(ConfigItem &&other) noexcept {
    if (this != &other) {
      id = std::move(other.id);
      key = std::move(other.key);
      value = std::move(other.value);
      updatedAt = other.updatedAt;
    }
    return *this;
  }

  // JSON serialization
  String toJson() const;
  static ConfigItem fromJson(const String &json);
  bool isValid() const;
};

#endif // DATA_MODELS_H