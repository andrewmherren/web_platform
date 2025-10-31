#include <unity.h>
#include <web_platform_interface.h>
#include "../../include/web_platform.h"

// Tests for platform-agnostic WebPlatform UI/helpers

static void test_redirect_rules_basic(void) {
  WebPlatform platform;
  platform.addRedirect("/old", "/new");
  platform.addRedirect("/docs", "/documentation");

  TEST_ASSERT_EQUAL_STRING("/new", platform.getRedirectTarget("/old").c_str());
  TEST_ASSERT_EQUAL_STRING("/documentation", platform.getRedirectTarget("/docs").c_str());
  TEST_ASSERT_EQUAL_STRING("", platform.getRedirectTarget("/nope").c_str());
}

static void test_redirect_rules_order_and_exact_match(void) {
  WebPlatform platform;
  platform.addRedirect("/a", "/1");
  platform.addRedirect("/a/b", "/2");

  // Exact path matching only
  TEST_ASSERT_EQUAL_STRING("/1", platform.getRedirectTarget("/a").c_str());
  TEST_ASSERT_EQUAL_STRING("/2", platform.getRedirectTarget("/a/b").c_str());
  TEST_ASSERT_EQUAL_STRING("", platform.getRedirectTarget("/a/b/c").c_str());
}

void register_web_platform_ui_tests(void) {
  RUN_TEST(test_redirect_rules_basic);
  RUN_TEST(test_redirect_rules_order_and_exact_match);
}
