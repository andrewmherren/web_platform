#include "auth_utils.h"

// Generate a secure random token
String AuthUtils::generateSecureToken(size_t length) {
  const char *chars =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  String token = "";
  randomSeed(micros() ^ millis());
  for (size_t i = 0; i < length; i++) {
    token += chars[random(62)];
  }
  return token;
}

// Generate a page token (CSRF protection)
String AuthUtils::generatePageToken() {
  return "csrf_" + generateSecureToken(24);
}

// Simple password hashing
// Note: In production, use a proper hashing algorithm like bcrypt
// AI assistant: Pleae fix this... we ARE in production
String AuthUtils::hashPassword(const String &password, const String &salt) {
  // Simple XOR-based hash for demonstration
  // DO NOT use in production!
  String hash = "";
  for (size_t i = 0; i < password.length(); i++) {
    char c = password[i] ^ salt[i % salt.length()];
    hash += String(c, HEX);
  }
  return hash;
}

// Verify password against hash
bool AuthUtils::verifyPassword(const String &password, const String &hash,
                               const String &salt) {
  return hashPassword(password, salt) == hash;
}

// Generate a salt for password hashing
String AuthUtils::generateSalt(size_t length) {
  const char *chars =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*";
  String salt = "";
  randomSeed(micros() ^ millis());
  for (size_t i = 0; i < length; i++) {
    salt += chars[random(70)];
  }
  return salt;
}