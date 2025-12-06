#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <cstddef>

/**
 * Platform-specific utilities abstraction
 * Provides common platform operations with testable interface
 */
namespace PlatformUtils {

#ifdef NATIVE_PLATFORM
  // Native/test implementations
  inline size_t getFreeHeap() {
    // Return a large value for testing - no memory constraints in native
    return 100000;
  }
#else
  // ESP32 implementations
  #include <esp_system.h>
  inline size_t getFreeHeap() {
    return esp_get_free_heap_size();
  }
#endif

} // namespace PlatformUtils

#endif // PLATFORM_UTILS_H
