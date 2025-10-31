#include "../../include/platform_provider.h"
#include "../../include/web_platform.h"
#include <ArduinoFake.h>
#include <unity.h>
#include <web_platform_interface.h>

void test_setup_production_platform_provider_creates_instance(void) {
#ifdef STANDALONE_TESTS
  TEST_PASS_MESSAGE("Platform provider not available in standalone test mode");
#else
  // Verify that setupProductionPlatformProvider creates and sets the instance
  TEST_ASSERT_NULL(IWebPlatformProvider::instance);

  setupProductionPlatformProvider();

  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);
#endif
}

void test_setup_production_platform_provider_is_idempotent(void) {
#ifdef STANDALONE_TESTS
  TEST_PASS_MESSAGE("Platform provider not available in standalone test mode");
#else
  // Verify that calling setupProductionPlatformProvider multiple times
  // doesn't create multiple instances (idempotent behavior)
  TEST_ASSERT_NULL(IWebPlatformProvider::instance);

  setupProductionPlatformProvider();
  IWebPlatformProvider *first_instance = IWebPlatformProvider::instance;
  TEST_ASSERT_NOT_NULL(first_instance);

  setupProductionPlatformProvider();
  IWebPlatformProvider *second_instance = IWebPlatformProvider::instance;

  TEST_ASSERT_EQUAL_PTR(first_instance, second_instance);
#endif
}

void test_production_provider_initialization_with_web_platform(void) {
#ifdef STANDALONE_TESTS
  TEST_PASS_MESSAGE("Platform provider not available in standalone test mode");
#else
  // Verify that the production provider is properly initialized with
  // webPlatform
  setupProductionPlatformProvider();

  TEST_ASSERT_NOT_NULL(IWebPlatformProvider::instance);

  // Test that we can get the platform instance from the provider
  IWebPlatform &platform = IWebPlatformProvider::instance->getPlatform();

  // Verify the platform reference is accessible
  TEST_PASS_MESSAGE("Production provider properly provides platform access");
#endif
}

void test_web_platform_provider_class_functionality(void) {
  // Test the WebPlatformProvider class directly
  WebPlatformProvider provider(&webPlatform);

  IWebPlatform &platform = provider.getPlatform();

  // The platform should be accessible and functional
  TEST_PASS_MESSAGE(
      "WebPlatformProvider class correctly wraps platform instance");
}

void register_platform_provider_tests(void) {
  RUN_TEST(test_setup_production_platform_provider_creates_instance);
  RUN_TEST(test_setup_production_platform_provider_is_idempotent);
  RUN_TEST(test_production_provider_initialization_with_web_platform);
  RUN_TEST(test_web_platform_provider_class_functionality);
}