#include "platform/route_string_pool.h"
#include "core/string_pool.h"
#include "utilities/debug_macros.h"
#include <Arduino.h>
#include <string>

// Arduino adapter for platform-agnostic StringPool
// This thin layer converts Arduino String to std::string and delegates to core
static WebPlatform::Core::StringPool corePool;

const char *RouteStringPool::store(const String &str) {
  // Convert Arduino String to std::string and delegate to core
  if (str.length() == 0) {
    return nullptr;
  }
  std::string stdStr(str.c_str());
  const char *result = corePool.store(stdStr);

  if (!result && !corePool.isSealed()) {
    ERROR_PRINTF("ERROR: RouteStringPool capacity exceeded (%d/%d)\n",
                 corePool.size(), corePool.capacity());
  } else if (!result && corePool.isSealed()) {
    ERROR_PRINTLN("ERROR: Attempted to store string in sealed RouteStringPool");
  }

  return result;
}

const char *RouteStringPool::store(const char *str) {
  if (!str || *str == '\0') { // NOSONAR: Safer than strlen for null-check
    return nullptr;
  }
  const char *result = corePool.store(str);

  if (!result && !corePool.isSealed()) {
    ERROR_PRINTF("ERROR: RouteStringPool capacity exceeded (%d/%d)\n",
                 corePool.size(), corePool.capacity());
  } else if (!result && corePool.isSealed()) {
    ERROR_PRINTLN("ERROR: Attempted to store string in sealed RouteStringPool");
  }

  return result;
}

const char *RouteStringPool::empty() { return corePool.empty(); }

void RouteStringPool::seal() {
  corePool.seal();
  DEBUG_PRINTF("RouteStringPool: Sealed with %d strings, capacity %d\n",
               corePool.size(), corePool.capacity());
}

void RouteStringPool::clear() { corePool.clear(); }

size_t RouteStringPool::getStorageCount() { return corePool.size(); }

size_t RouteStringPool::getEstimatedMemoryUsage() {
  return corePool.memoryUsage();
}