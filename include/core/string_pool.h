#ifndef STRING_POOL_CORE_H
#define STRING_POOL_CORE_H

#include <cstddef>
#include <string>

namespace WebPlatform {
namespace Core {

/**
 * @brief Platform-agnostic string pooling for efficient string storage
 *
 * This class provides deduplication and stable pointer storage for strings.
 * It's designed to be used during route registration to minimize memory usage
 * by storing unique strings only once.
 *
 * Platform-agnostic design allows testing without Arduino dependencies.
 */
class StringPool {
public:
  /**
   * @brief Store a string in the pool
   * @param str The string to store
   * @return Pointer to the stored string (stable until clear() is called), or
   * nullptr for empty strings
   */
  const char *store(const std::string &str);

  /**
   * @brief Store a C-string in the pool
   * @param str The C-string to store
   * @return Pointer to the stored string (stable until clear() is called), or
   * nullptr for null/empty strings
   */
  const char *store(const char *str);

  /**
   * @brief Get an empty string pointer (nullptr)
   * @return nullptr
   */
  const char *empty() const;

  /**
   * @brief Seal the pool to prevent further additions
   *
   * After sealing, attempts to store new strings will fail.
   * This helps catch registration after finalization bugs.
   */
  void seal();

  /**
   * @brief Check if the pool is sealed
   * @return true if sealed, false otherwise
   */
  bool isSealed() const;

  /**
   * @brief Clear all stored strings (only if not sealed)
   */
  void clear();

  /**
   * @brief Get the number of unique strings stored
   * @return Number of stored strings
   */
  size_t size() const;

  /**
   * @brief Get estimated memory usage of stored strings
   * @return Bytes used by stored strings (including null terminators)
   */
  size_t memoryUsage() const;

  /**
   * @brief Get the reserved capacity
   * @return Maximum number of strings that can be stored
   */
  size_t capacity() const;

  /**
   * @brief Set the reserved capacity
   * @param cap Maximum number of strings to reserve space for
   */
  void reserve(size_t cap);

private:
  struct Impl;
  Impl *impl = nullptr;

  // Initialize implementation on first use
  void ensureInitialized();
};

} // namespace Core
} // namespace WebPlatform

#endif // STRING_POOL_CORE_H
