#ifndef AUTH_UTILS_H
#define AUTH_UTILS_H

#include <Arduino.h>
#include "crypto_platform.h"

namespace AuthUtils {
  // Generate a secure random token
  String generateSecureToken(size_t length = 32);
  
  // Generate a CSRF/page token
  String generatePageToken();
  
  // Production-ready password hashing using PBKDF2
  String hashPassword(const String &password, const String &salt, int iterations = 10000);
  
  // Verify password against hash
  bool verifyPassword(const String &password, const String &hash, const String &salt, int iterations = 10000);
  
  // Generate a cryptographically secure random salt
  String generateSalt(size_t length = 16);
  
  // Convert hex string to bytes
  bool hexToBytes(const String &hex, uint8_t *bytes, size_t maxLen);
  
  // Convert bytes to hex string
  String bytesToHex(const uint8_t *bytes, size_t length);
  
#ifdef ESP8266
  // ESP8266-specific PBKDF2 implementation
  void pbkdf2_sha256(const uint8_t *password, size_t password_len,
                     const uint8_t *salt, size_t salt_len,
                     uint32_t iterations,
                     uint8_t *output, size_t output_len);
#endif
}

#endif // AUTH_UTILS_H