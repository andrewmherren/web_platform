#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <Arduino.h>
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
  unsigned long createdAt; // Creation timestamp

  AuthUser() : createdAt(0) {}
  AuthUser(const String &username, const String &hash, const String &salt);

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

  // JSON serialization
  String toJson() const;
  static ConfigItem fromJson(const String &json);
  bool isValid() const;
};

#endif // DATA_MODELS_H