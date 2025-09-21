#include "platform/route_string_pool.h"
#include "utilities/debug_macros.h"
#include <Arduino.h>
#include <vector>

// Pre-allocated string storage with fixed capacity to prevent reallocations
class StableStringStorage {
private:
  std::vector<String> strings;
  bool sealed = false;

public:
  StableStringStorage() {
    // Only route paths are stored now (~43 routes vs previous ~430 strings)
    strings.reserve(64); // Sufficient for route paths only
  }

  const char *store(const String &str) {
    if (str.length() == 0) {
      return nullptr;
    }

    if (sealed) {
      ERROR_PRINTLN(
          "ERROR: Attempted to store string in sealed RouteStringPool");
      return nullptr;
    }

    // Check if we already have this string to avoid duplicates
    for (const auto &existing : strings) {
      if (existing == str) {
        return existing.c_str();
      }
    }

    // Safety check - never exceed reserved capacity
    if (strings.size() >= strings.capacity()) {
      ERROR_PRINTF("ERROR: RouteStringPool capacity exceeded (%d/%d)\n",
                   strings.size(), strings.capacity());
      return nullptr;
    }

    // Store the new string
    strings.push_back(str);
    return strings.back().c_str();
  }

  void seal() {
    sealed = true;
    DEBUG_PRINTF("RouteStringPool: Sealed with %d strings, capacity %d\n",
                 strings.size(), strings.capacity());
  }

  size_t size() const { return strings.size(); }

  size_t memoryUsage() const {
    size_t total = 0;
    for (const auto &str : strings) {
      total += str.length() + 1; // +1 for null terminator
    }
    return total;
  }

  void clear() {
    if (!sealed) {
      strings.clear();
    }
  }
};

// Static singleton instance
static StableStringStorage storage;

const char *RouteStringPool::store(const String &str) {
  return storage.store(str);
}

const char *RouteStringPool::store(const char *str) {
  if (!str || strlen(str) == 0) {
    return nullptr;
  }
  return storage.store(String(str));
}

const char *RouteStringPool::empty() { return nullptr; }

void RouteStringPool::seal() { storage.seal(); }

void RouteStringPool::clear() { storage.clear(); }

size_t RouteStringPool::getStorageCount() { return storage.size(); }

size_t RouteStringPool::getEstimatedMemoryUsage() {
  return storage.memoryUsage();
}