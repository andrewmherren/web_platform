#include "../../include/auth/auth_utils.h"

// Generate a cryptographically secure random token
String AuthUtils::generateSecureToken(size_t length) {
  const char *chars =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  String token = "";
  token.reserve(length);

  for (size_t i = 0; i < length; i++) {
    uint32_t randomValue = esp_random();
    token += chars[randomValue % 62];
  }
  return token;
}

// Generate a page token (CSRF protection)
String AuthUtils::generatePageToken() {
  return "csrf_" + generateSecureToken(24);
}

// Production-ready password hashing using PBKDF2
String AuthUtils::hashPassword(const String &password, const String &salt,
                               int iterations) {
  const size_t hashLength = 32; // 256 bits
  uint8_t hash[hashLength];

  // Use mbedTLS PBKDF2 on ESP32
  mbedtls_md_context_t md_ctx;
  mbedtls_md_init(&md_ctx);

  const mbedtls_md_info_t *md_info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0) {
    mbedtls_md_free(&md_ctx);
    return "";
  }

  int result = mbedtls_pkcs5_pbkdf2_hmac(
      &md_ctx, (const unsigned char *)password.c_str(), password.length(),
      (const unsigned char *)salt.c_str(), salt.length(), iterations,
      hashLength, hash);

  mbedtls_md_free(&md_ctx);

  if (result != 0) {
    return "";
  }

  return bytesToHex(hash, hashLength);
}

// Verify password against hash
bool AuthUtils::verifyPassword(const String &password, const String &hash,
                               const String &salt, int iterations) {
  String computedHash = hashPassword(password, salt, iterations);
  return computedHash.length() > 0 && computedHash.equalsIgnoreCase(hash);
}

// Generate a cryptographically secure random salt
String AuthUtils::generateSalt(size_t length) {
  uint8_t saltBytes[length];

  esp_fill_random(saltBytes, length);

  return bytesToHex(saltBytes, length);
}

// Generate a unique user ID (UUID v4 format)
String AuthUtils::generateUserId() {
  uint8_t uuid[16];

  esp_fill_random(uuid, 16);

  // Set version (4) and variant bits for UUID v4
  uuid[6] = (uuid[6] & 0x0F) | 0x40; // Version 4
  uuid[8] = (uuid[8] & 0x3F) | 0x80; // Variant bits

  // Format as UUID string: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  String uuidStr = "";
  uuidStr.reserve(36);

  for (int i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      uuidStr += "-";
    }

    if (uuid[i] < 16) {
      uuidStr += "0";
    }
    uuidStr += String(uuid[i], HEX);
  }

  uuidStr.toLowerCase();
  return uuidStr;
}

// Convert hex string to bytes
bool AuthUtils::hexToBytes(const String &hex, uint8_t *bytes, size_t maxLen) {
  if (hex.length() % 2 != 0 || hex.length() / 2 > maxLen) {
    return false;
  }

  for (size_t i = 0; i < hex.length(); i += 2) {
    char high = hex[i];
    char low = hex[i + 1];

    uint8_t highNibble = (high >= '0' && high <= '9')   ? (high - '0')
                         : (high >= 'A' && high <= 'F') ? (high - 'A' + 10)
                         : (high >= 'a' && high <= 'f') ? (high - 'a' + 10)
                                                        : 0;

    uint8_t lowNibble = (low >= '0' && low <= '9')   ? (low - '0')
                        : (low >= 'A' && low <= 'F') ? (low - 'A' + 10)
                        : (low >= 'a' && low <= 'f') ? (low - 'a' + 10)
                                                     : 0;

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

// Parse IP address from string (e.g., "192.168.1.1")
AuthUtils::IPAddress AuthUtils::parseIPAddress(const String &ipStr) {
  IPAddress ip;

  if (ipStr.isEmpty()) {
    return ip; // Invalid IP (all zeros)
  }

  int dotCount = 0;
  int start = 0;
  int octetIndex = 0;

  for (int i = 0; i <= (int)ipStr.length(); i++) {
    if (i == (int)ipStr.length() || ipStr[i] == '.') {
      if (octetIndex >= 4) {
        return IPAddress(); // Too many octets
      }

      String octetStr = ipStr.substring(start, i);
      int octetValue = octetStr.toInt();

      // Validate octet range (0-255)
      if (octetValue < 0 || octetValue > 255 ||
          (octetValue == 0 && !octetStr.equals("0"))) {
        return IPAddress(); // Invalid octet
      }

      ip.bytes[octetIndex++] = (uint8_t)octetValue;
      start = i + 1;

      if (ipStr[i] == '.') {
        dotCount++;
      }
    }
  }

  // Must have exactly 3 dots and 4 octets
  if (dotCount != 3 || octetIndex != 4) {
    return IPAddress(); // Invalid format
  }

  return ip;
}

// Check if IP is in subnet using CIDR notation
bool AuthUtils::isIPInSubnet(const IPAddress &ip, const Subnet &subnet) {
  if (!ip.isValid() || !subnet.network.isValid() || subnet.prefixLength == 0 ||
      subnet.prefixLength > 32) {
    return false;
  }

  // Calculate network mask
  uint32_t mask = 0xFFFFFFFF << (32 - subnet.prefixLength);

  // Convert IP addresses to 32-bit integers (big-endian)
  uint32_t ipInt = ((uint32_t)ip.bytes[0] << 24) |
                   ((uint32_t)ip.bytes[1] << 16) |
                   ((uint32_t)ip.bytes[2] << 8) | (uint32_t)ip.bytes[3];

  uint32_t networkInt = ((uint32_t)subnet.network.bytes[0] << 24) |
                        ((uint32_t)subnet.network.bytes[1] << 16) |
                        ((uint32_t)subnet.network.bytes[2] << 8) |
                        (uint32_t)subnet.network.bytes[3];

  // Apply mask and compare
  return (ipInt & mask) == (networkInt & mask);
}

// Check if IP address is in local/private network ranges
bool AuthUtils::isLocalNetworkIP(const IPAddress &ip) {
  if (!ip.isValid()) {
    return false;
  }

  // Check for private network ranges (RFC 1918)
  // 10.0.0.0/8 (10.0.0.0 - 10.255.255.255)
  if (ip.bytes[0] == 10) {
    return true;
  }

  // 172.16.0.0/12 (172.16.0.0 - 172.31.255.255)
  if (ip.bytes[0] == 172 && ip.bytes[1] >= 16 && ip.bytes[1] <= 31) {
    return true;
  }

  // 192.168.0.0/16 (192.168.0.0 - 192.168.255.255)
  if (ip.bytes[0] == 192 && ip.bytes[1] == 168) {
    return true;
  }

  // Check for link-local addresses (RFC 3927)
  // 169.254.0.0/16 (169.254.0.0 - 169.254.255.255)
  if (ip.bytes[0] == 169 && ip.bytes[1] == 254) {
    return true;
  }

  // Check for loopback
  if (isLoopbackIP(ip)) {
    return true;
  }

  return false;
}

// Check if IP is localhost/loopback
bool AuthUtils::isLoopbackIP(const IPAddress &ip) {
  if (!ip.isValid()) {
    return false;
  }

  // 127.0.0.0/8 (127.0.0.0 - 127.255.255.255)
  return ip.bytes[0] == 127;
}
