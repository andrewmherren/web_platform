#include "platform_provider.h"
#include <unity.h>
#include <web_platform_interface.h>

// Native-only: Mock testing platform provider
#ifndef ESP_PLATFORM
#include <testing/testing_platform_provider.h>
#endif

// ESP32-only: Production provider wiring
#ifdef ESP_PLATFORM
void test_setup_production_platform_provider_creates_instance(void) {
  // Note: instance may be null or already set from a previous test
  // This test verifies that calling setup results in a non-null instance
  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);
}

void test_setup_production_platform_provider_is_idempotent(void) {
  // Call setup multiple times - should not crash and instance should remain set
  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);

  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);

  // If we got here without crashing, the test passes
  TEST_ASSERT_TRUE(true);
}
void test_production_provider_initialization_with_web_platform(void) {
  // Ensure provider is set up and can access platform
  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);
  IWebPlatform &platform = IWebPlatformProvider::instance->getPlatform();
  (void)platform; // ensure reference is usable
}
#endif // Native-safe: Mock-based provider test
#ifndef ESP_PLATFORM
void test_web_platform_provider_class_functionality(void) {
  // Native: verify provider wraps any IWebPlatform implementation
  MockWebPlatform mock;
  WebPlatformProvider provider(&mock);
  IWebPlatform &platform = provider.getPlatform();
  TEST_ASSERT_EQUAL_PTR(&mock, &platform);
}
#endif

void register_platform_provider_tests(void) {
#ifdef ESP_PLATFORM
  // ESP32-only tests (production provider integration)
  RUN_TEST(test_setup_production_platform_provider_creates_instance);
  RUN_TEST(test_setup_production_platform_provider_is_idempotent);
  RUN_TEST(test_production_provider_initialization_with_web_platform);
#else
  // Native-only test (mock-based provider)
  RUN_TEST(test_web_platform_provider_class_functionality);
#endif
}
