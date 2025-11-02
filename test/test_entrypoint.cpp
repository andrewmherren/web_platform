#include <unity.h>

// Include native test sources so they compile as part of the root "*" suite
// This keeps PlatformIO happy regardless of its folder-based test discovery.
// We selectively run native or ESP32 entrypoints based on build flags.

// Bring in the test suites (native-safe sources)
#ifdef NATIVE_PLATFORM
#include "native/src/core/test_string_pool.cpp"
#include "native/src/core/test_url_utils.cpp"
#include "native/src/test_platform_provider.cpp"
#include "native/src/types/test_navigation_types.cpp"
#include "native/src/types/test_redirect_types.cpp"
#endif

// Forward declarations for registrars/runners from included sources
void runStringPoolTests();
void runUrlUtilsTests();
void register_navigation_types_tests(void);
void register_redirect_types_tests(void);
void register_platform_provider_tests(void);

// Native entrypoint
#ifdef NATIVE_PLATFORM
// Pull in native-safe production implementations needed by tests
// Note: Including .cpp files is necessary for PlatformIO test discovery
// These are wrapped in test-specific guards to prevent multiple definitions
#ifndef WEB_PLATFORM_TEST_ENTRYPOINT_INCLUDED
#define WEB_PLATFORM_TEST_ENTRYPOINT_INCLUDED
#include "../src/core/string_pool.cpp"
#include "../src/core/url_utils.cpp"
#include "../src/types/navigation_types.cpp"
#include "../src/types/redirect_types.cpp"
#endif // WEB_PLATFORM_TEST_ENTRYPOINT_INCLUDED

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Core platform-agnostic tests (no Arduino dependencies)
  runStringPoolTests();
  runUrlUtilsTests();

  // Type and provider tests (native-mock variants)
  register_navigation_types_tests();
  register_redirect_types_tests();
  register_platform_provider_tests();

  UNITY_END();
  return 0;
}
// ESP32 entrypoint mirrors test/esp32/test_main.cpp
#else
#include <Arduino.h>

void setup() {
  // Allow USB CDC/Serial to enumerate; mirrors existing ESP32 test
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  UNITY_BEGIN();

  // ESP32-specific platform provider tests
  register_platform_provider_tests();

  UNITY_END();
}

void loop() {
  // No-op
}
#endif
