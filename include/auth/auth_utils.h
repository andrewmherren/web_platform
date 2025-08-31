#ifndef AUTH_UTILS_H
#define AUTH_UTILS_H

#include <Arduino.h>

namespace AuthUtils {
  // Generate a secure random token
  String generateSecureToken(size_t length = 32);
  
  // Generate a CSRF/page token
  String generatePageToken();
  
  // Simple password hashing (in production, use a proper hashing algorithm)
  String hashPassword(const String &password, const String &salt);
  
  // Verify password against hash
  bool verifyPassword(const String &password, const String &hash, const String &salt);
  
  // Generate a random salt
  String generateSalt(size_t length = 16);
}

#endif // AUTH_UTILS_H