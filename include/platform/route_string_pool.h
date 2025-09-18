#ifndef ROUTE_STRING_POOL_H
#define ROUTE_STRING_POOL_H

#include <Arduino.h>

// Helper class to manage route string storage efficiently
// Uses std::map to avoid pointer invalidation issues
class RouteStringPool {
public:
  // Store a string and return a const char* pointer to it
  // This ensures the string remains valid for the lifetime of the program
  static const char *store(const String &str);

  // Store a const char* (assumes it's already in PROGMEM or static)
  static const char *store(const char *str);

  // Get an empty string pointer (nullptr equivalent but safe for comparisons)
  static const char *empty();

  // Clear all stored strings (for cleanup)
  static void clear();

  // Seal the pool to prevent further additions (call after all routes
  // registered)
  static void seal();

  // Get statistics
  static size_t getStorageCount();
  static size_t getEstimatedMemoryUsage();
};

// Convenience macros for storing route strings
#define STORE_ROUTE_STRING(str) RouteStringPool::store(str)
#define STORE_PROGMEM_STRING(str) RouteStringPool::store(FPSTR(str))

#endif // ROUTE_STRING_POOL_H