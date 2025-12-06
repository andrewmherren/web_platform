#ifndef URL_UTILS_CORE_H
#define URL_UTILS_CORE_H

#include <string>

namespace WebPlatform {
namespace Core {

/**
 * @brief Platform-agnostic URL encoding and decoding utilities
 *
 * This class provides URL encoding/decoding without Arduino dependencies,
 * allowing for native testing and portability.
 *
 * Design follows the core/wrapper pattern:
 * - Core logic uses std::string (this file)
 * - Arduino wrapper can convert String <-> std::string as needed
 */
class UrlUtils {
public:
  /**
   * @brief Decode a URL-encoded string
   *
   * Handles:
   * - %XX hex encoding (e.g., %20 for space)
   * - + to space conversion
   * - Invalid sequences (incomplete %, non-hex chars)
   *
   * @param encoded The URL-encoded string
   * @return Decoded string
   */
  static std::string decode(const std::string &encoded);

  /**
   * @brief Encode a string for use in URLs
   *
   * Encodes all characters except:
   * - Alphanumeric (A-Z, a-z, 0-9)
   * - Unreserved characters (- _ . ~)
   *
   * Spaces are encoded as %20 (not +) for consistency.
   *
   * @param raw The raw string to encode
   * @return URL-encoded string
   */
  static std::string encode(const std::string &raw);

  /**
   * @brief Check if a character should be encoded in URLs
   *
   * @param c Character to check
   * @return true if character needs encoding
   */
  static bool needsEncoding(char c);

  /**
   * @brief Convert a hex digit to its numeric value
   *
   * @param hexChar Hex character (0-9, A-F, a-f)
   * @return Numeric value (0-15), or -1 if invalid
   */
  static int hexCharToInt(char hexChar);

  /**
   * @brief Convert a numeric value to hex character
   *
   * @param value Value (0-15)
   * @return Hex character (0-9, A-F), or '0' if invalid
   */
  static char intToHexChar(int value);
};

} // namespace Core
} // namespace WebPlatform

#endif // URL_UTILS_CORE_H
