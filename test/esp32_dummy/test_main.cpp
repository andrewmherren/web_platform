#include <Arduino.h>
#include <unity.h>

// Forward declaration for platform provider tests
void register_platform_provider_tests(void);

void test_dummy() { TEST_ASSERT_TRUE(true); }

void setup() {
  // Give USB CDC time to enumerate after reset
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  UNITY_BEGIN();

  // Run ESP32-specific platform provider tests
  register_platform_provider_tests();

  UNITY_END();
}

void loop() {
  // No-op
}
