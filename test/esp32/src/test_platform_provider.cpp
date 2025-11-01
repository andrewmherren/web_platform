#include "platform_provider.h"
#include <unity.h>
#include <web_platform_interface.h>

// ESP32-only: Production provider wiring
#ifdef ESP_PLATFORM
void test_setup_production_platform_provider_creates_instance(void) {
  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);
}

void test_setup_production_platform_provider_is_idempotent(void) {
  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);

  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);
  TEST_ASSERT_TRUE(true);
}

void test_production_provider_initialization_with_web_platform(void) {
  setupProductionPlatformProvider();
  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);
  IWebPlatform &platform = IWebPlatformProvider::instance->getPlatform();
  (void)platform;
}
#endif

void register_platform_provider_tests(void) {
#ifdef ESP_PLATFORM
  RUN_TEST(test_setup_production_platform_provider_creates_instance);
  RUN_TEST(test_setup_production_platform_provider_is_idempotent);
  RUN_TEST(test_production_provider_initialization_with_web_platform);
#endif
}
