#include "core/url_utils.h"
#include <cctype>
#include <iomanip>
#include <sstream>

namespace WebPlatform {
namespace Core {

std::string UrlUtils::decode(const std::string &encoded) {
  std::string decoded;
  decoded.reserve(encoded.length()); // Pre-allocate for efficiency

  size_t len = encoded.length();
  for (size_t i = 0; i < len; ++i) {
    char c = encoded[i];

    if (c == '+') {
      // Convert + to space
      decoded += ' ';
    } else if (c == '%' && i + 2 < len) {
      // Try to decode %XX hex sequence
      int high = hexCharToInt(encoded[i + 1]);
      int low = hexCharToInt(encoded[i + 2]);

      if (high >= 0 && low >= 0) {
        // Valid hex sequence
        decoded += static_cast<char>((high << 4) | low);
        i += 2; // Skip the two hex digits
      } else {
        // Invalid hex sequence, keep the % as-is
        decoded += c;
      }
    } else {
      // Regular character
      decoded += c;
    }
  }

  return decoded;
}

std::string UrlUtils::encode(const std::string &raw) {
  std::ostringstream encoded;
  encoded.fill('0');
  encoded << std::hex << std::uppercase;

  for (unsigned char c : raw) {
    if (needsEncoding(c)) {
      // Encode as %XX
      encoded << '%' << std::setw(2) << static_cast<int>(c);
    } else {
      // Keep as-is
      encoded << c;
    }
  }

  return encoded.str();
}

bool UrlUtils::needsEncoding(char c) {
  // Don't encode alphanumeric characters
  if (std::isalnum(static_cast<unsigned char>(c))) {
    return false;
  }

  // Don't encode unreserved characters per RFC 3986
  // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
  if (c == '-' || c == '.' || c == '_' || c == '~') {
    return false;
  }

  // Everything else needs encoding
  return true;
}

int UrlUtils::hexCharToInt(char hexChar) {
  if (hexChar >= '0' && hexChar <= '9') {
    return hexChar - '0';
  }
  if (hexChar >= 'A' && hexChar <= 'F') {
    return hexChar - 'A' + 10;
  }
  if (hexChar >= 'a' && hexChar <= 'f') {
    return hexChar - 'a' + 10;
  }
  return -1; // Invalid hex character
}

char UrlUtils::intToHexChar(int value) {
  if (value >= 0 && value <= 9) {
    return '0' + value;
  }
  if (value >= 10 && value <= 15) {
    return 'A' + (value - 10);
  }
  return '0'; // Invalid value
}

} // namespace Core
} // namespace WebPlatform
