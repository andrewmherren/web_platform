#include <unity.h>

// Native tests for web_platform are a work-in-progress
// Currently blocked by:
// - ArduinoFake String class lacks .write() method needed by ArduinoJson
// - WebPlatform class requires significant ESP32-specific dependencies
//
// Alternative testing strategy (following architecture):
// 1. web_platform_interface: Pure C++ tests (working ✅)
// 2. Module tests (maker_api, etc): Test via interface with mocks (working ✅)
// 3. web_platform: ESP32 integration tests (future work)
//
// TODO: Extract core logic into testable utilities following core/wrapper
// pattern

// Forward declarations for test registrars we enable incrementally
void register_navigation_types_tests(void);
void register_redirect_types_tests(void);

// Pull in native-safe production implementations needed by tests
#include "../src/types/navigation_types.cpp"
#include "../src/types/redirect_types.cpp"

#ifdef NATIVE_PLATFORM
// Unity expects these even if empty
extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Enable type tests first (safe for native)
  register_navigation_types_tests();
  register_redirect_types_tests();

  UNITY_END();
  return 0;
}
#else
void setup() {
  UNITY_BEGIN();

  // Enable type tests first (safe for native)
  register_navigation_types_tests();
  register_redirect_types_tests();

  UNITY_END();
}

void loop() {
  // No-op loop required for Arduino test harness
}
#endif