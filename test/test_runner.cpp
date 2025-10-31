#include <ArduinoFake.h>
#include <unity.h>

// Include all test header files
// #include "include/test_platform_provider.h"
// #include "include/test_web_platform.h"
// #include "include/types/test_navigation_types.h"
// #include "include/types/test_redirect_types.h"

void setUp() { ArduinoFakeReset(); }

void tearDown() {
  // Clean teardown - nothing needed currently
}

#ifdef NATIVE_PLATFORM
int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Register and run all test groups
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

  // Register and run all test groups
  // register_web_platform_tests();
  // register_platform_provider_tests();
  // register_redirect_types_tests();
  // register_navigation_types_tests();

  UNITY_END();
}

void loop() {}
#endif