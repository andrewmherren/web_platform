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

// Include all test header files
// #include "include/test_platform_provider.h"
// #include "include/test_web_platform.h"
// #include "include/types/test_navigation_types.h"
// #include "include/types/test_redirect_types.h"

void setUp() {
  // ArduinoFakeReset(); // Commented until tests are enabled
}

void tearDown() {
  // Clean teardown - nothing needed currently
}

#ifdef NATIVE_PLATFORM
int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Register and run all test groups (currently disabled - see note above)
  // register_web_platform_tests();
  // register_platform_provider_tests();
  // register_redirect_types_tests();
  // register_navigation_types_tests();

  UNITY_END();
  return 0;
}
#else
void setup() {
  UNITY_BEGIN();

  // Register and run all test groups (currently disabled - see note above)
  // register_web_platform_tests();
  // register_platform_provider_tests();
  // register_redirect_types_tests();
  // register_navigation_types_tests();

  UNITY_END();
}

void loop() {}
#endif