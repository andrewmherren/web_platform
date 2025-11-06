#include <unity.h>

// Forward declarations for registrars/runners; the .cpp files are compiled
// via build_src_filter in platformio.ini (no direct includes here)
void runStringPoolTests();
void runUrlUtilsTests();
void register_navigation_types_tests(void);
void register_redirect_types_tests(void);
void register_platform_provider_tests(void);

// Native entrypoint
#ifdef NATIVE_PLATFORM
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
  // Allow USB CDC/Serial to enumerate
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  UNITY_BEGIN();
  // Give the serial monitor a moment to attach before printing results
  delay(500);

  // ESP32-specific platform provider tests
  register_platform_provider_tests();

  UNITY_END();
}

void loop() {
  // No-op
}
#endif
