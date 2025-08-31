#include "../../include/auth/auth_utils.h"

// Generate a cryptographically secure random token
String AuthUtils::generateSecureToken(size_t length) {
  const char *chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  String token = "";
  token.reserve(length);
  
  for (size_t i = 0; i < length; i++) {
#ifdef ESP32
    uint32_t randomValue = esp_random();
#elif defined(ESP8266)
    uint32_t randomValue = esp_random();
#else
    // Fallback for other platforms
    randomSeed(micros() ^ millis());
    uint32_t randomValue = random();
#endif
    token += chars[randomValue % 62];
  }
  return token;
}

// Generate a page token (CSRF protection)
String AuthUtils::generatePageToken() {
  return "csrf_" + generateSecureToken(24);
}

// Production-ready password hashing using PBKDF2
String AuthUtils::hashPassword(const String &password, const String &salt, int iterations) {
  const size_t hashLength = 32; // 256 bits
  uint8_t hash[hashLength];
  
#ifdef ESP32
  // Use mbedTLS PBKDF2 on ESP32
  mbedtls_md_context_t md_ctx;
  mbedtls_md_init(&md_ctx);
  
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0) {
    mbedtls_md_free(&md_ctx);
    return "";
  }
  
  int result = mbedtls_pkcs5_pbkdf2_hmac(&md_ctx,
                                        (const unsigned char*)password.c_str(),
                                        password.length(),
                                        (const unsigned char*)salt.c_str(),
                                        salt.length(),
                                        iterations,
                                        hashLength,
                                        hash);
  
  mbedtls_md_free(&md_ctx);
  
  if (result != 0) {
    return "";
  }
  
#elif defined(ESP8266)
  // Use BearSSL PBKDF2 on ESP8266
  pbkdf2_sha256((const uint8_t*)password.c_str(), password.length(),
                (const uint8_t*)salt.c_str(), salt.length(),
                iterations, hash, hashLength);
  
#else
  // Fallback simple hash for other platforms (not production ready)
  String combined = password + salt;
  for (size_t i = 0; i < hashLength && i < combined.length(); i++) {
    hash[i] = combined[i];
  }
#endif
  
  return bytesToHex(hash, hashLength);
}

// Verify password against hash
bool AuthUtils::verifyPassword(const String &password, const String &hash, const String &salt, int iterations) {
  String computedHash = hashPassword(password, salt, iterations);
  return computedHash.length() > 0 && computedHash.equalsIgnoreCase(hash);
}

// Generate a cryptographically secure random salt
String AuthUtils::generateSalt(size_t length) {
  uint8_t saltBytes[length];
  
#ifdef ESP32
  esp_fill_random(saltBytes, length);
#elif defined(ESP8266)
  for (size_t i = 0; i < length; i++) {
    uint32_t rand = esp_random();
    saltBytes[i] = rand & 0xFF;
  }
#else
  // Fallback for other platforms
  randomSeed(micros() ^ millis());
  for (size_t i = 0; i < length; i++) {
    saltBytes[i] = random(256);
  }
#endif
  
  return bytesToHex(saltBytes, length);
}

// Convert hex string to bytes
bool AuthUtils::hexToBytes(const String &hex, uint8_t *bytes, size_t maxLen) {
  if (hex.length() % 2 != 0 || hex.length() / 2 > maxLen) {
    return false;
  }
  
  for (size_t i = 0; i < hex.length(); i += 2) {
    char high = hex[i];
    char low = hex[i + 1];
    
    uint8_t highNibble = (high >= '0' && high <= '9') ? (high - '0') :
                        (high >= 'A' && high <= 'F') ? (high - 'A' + 10) :
                        (high >= 'a' && high <= 'f') ? (high - 'a' + 10) : 0;
    
    uint8_t lowNibble = (low >= '0' && low <= '9') ? (low - '0') :
                       (low >= 'A' && low <= 'F') ? (low - 'A' + 10) :
                       (low >= 'a' && low <= 'f') ? (low - 'a' + 10) : 0;
    
    bytes[i / 2] = (highNibble << 4) | lowNibble;
  }
  
  return true;
}

// Convert bytes to hex string
String AuthUtils::bytesToHex(const uint8_t *bytes, size_t length) {
  String hex = "";
  hex.reserve(length * 2);
  
  for (size_t i = 0; i < length; i++) {
    if (bytes[i] < 16) {
      hex += "0";
    }
    hex += String(bytes[i], HEX);
  }
  
  hex.toUpperCase();
  return hex;
}
