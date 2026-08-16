#include "web_platform.h"
#include <unity.h>

// ESP32-only: exercises the real boot path through the new Router/
// ServerManager/WebPlatform-orchestrator split - determinePlatformMode(),
// ServerManager::detectHttpsCapability()/start(), setupConfigPortalMode(),
// and (via handle()'s first-call finalizeRoutes()) Router::bindHttp()
// binding routes onto a live WebServerClass. No stored WiFi credentials or
// embedded certs exist in this test build, so CONFIG_PORTAL/HTTP-only is the
// only mode reachable here - see PROJECT_PLAN.md's testing-reality notes.
#ifdef ESP_PLATFORM
void test_web_platform_begins_in_config_portal_without_stored_wifi(void) {
  webPlatform.resetWiFiCredentials();

  webPlatform.begin("HwTest", "0.0.0-test", false);

  TEST_ASSERT_TRUE(webPlatform.getCurrentMode() == CONFIG_PORTAL);
  TEST_ASSERT_TRUE(webPlatform.getConnectionState() == WIFI_CONFIG_PORTAL);
  TEST_ASSERT_FALSE(webPlatform.isHttpsEnabled());

  // First handle() call triggers finalizeRoutes() -> ServerManager::bindRoutes()
  // -> Router::bindHttp(), which actually registers routes on the live server.
  webPlatform.handle();

  TEST_ASSERT_TRUE(webPlatform.getRouteCount() > 0);
}
#endif

void register_web_platform_boot_tests(void) {
#ifdef ESP_PLATFORM
  RUN_TEST(test_web_platform_begins_in_config_portal_without_stored_wifi);
#endif
}
