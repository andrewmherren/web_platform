#ifndef AUTH_UTILS_H
#define AUTH_UTILS_H

#ifdef ESP_PLATFORM
#include "esp_random.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/sha256.h"
#endif

#include <interface/string_compat.h>
#include <testing/arduino_string_compat.h>

namespace AuthUtils {
// Generate a secure random token
String generateSecureToken(size_t length = 32);

// Generate a CSRF/page token
String generatePageToken();

// Production-ready password hashing using PBKDF2
String hashPassword(const String &password, const String &salt,
                    int iterations = 10000);

// Verify password against hash
bool verifyPassword(const String &password, const String &hash,
                    const String &salt, int iterations = 10000);

// Generate a cryptographically secure random salt
String generateSalt(size_t length = 16);

// Generate a unique user ID (UUID v4 format)
String generateUserId();

// Convert bytes to hex string
String bytesToHex(const uint8_t *bytes, size_t length);

// Network/IP validation utilities
struct IPAddress {
  uint8_t bytes[4]; // NOSONAR: IPv4 address requires exactly 4 bytes

  IPAddress() { bytes[0] = bytes[1] = bytes[2] = bytes[3] = 0; }
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    bytes[0] = a;
    bytes[1] = b;
    bytes[2] = c;
    bytes[3] = d;
  }

  bool isValid() const {
    return bytes[0] != 0 || bytes[1] != 0 || bytes[2] != 0 || bytes[3] != 0;
  }

  String toString() const {
    return String(bytes[0]) + "." + String(bytes[1]) + "." + String(bytes[2]) +
           "." + String(bytes[3]);
  }
};

struct Subnet {
  IPAddress network;
  uint8_t prefixLength; // CIDR notation (e.g., 24 for /24)

  Subnet() : prefixLength(0) {}
  Subnet(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t prefix)
      : network(a, b, c, d), prefixLength(prefix) {}
};

// Parse IP address from string (e.g., "192.168.1.1")
IPAddress parseIPAddress(const String &ipStr);

// Check if IP is in subnet using CIDR notation
bool isIPInSubnet(const IPAddress &ip, const Subnet &subnet);

// Check if IP address is in local/private network ranges
bool isLocalNetworkIP(const IPAddress &ip);

// Check if IP is localhost/loopback
bool isLoopbackIP(const IPAddress &ip);
} // namespace AuthUtils

#endif // AUTH_UTILS_H