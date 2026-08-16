#include <unity.h>

// Forward declarations for registrars/runners; the .cpp files are compiled
// via build_src_filter in platformio.ini (no direct includes here)
void runStringPoolTests();
void runUrlUtilsTests();
void register_navigation_types_tests(void);
void register_redirect_types_tests(void);
void register_platform_provider_tests(void);
void register_web_platform_boot_tests(void);
void runAuthUtilsTests();
void register_auth_decision_tests(void);
void register_query_builder_tests(void);
void register_json_database_driver_tests(void);
void register_littlefs_database_driver_tests(void);
void register_storage_manager_tests(void);
void register_auth_storage_tests(void);
void register_openapi_spec_helpers_tests(void);
void register_system_status_helpers_tests(void);
void register_certificate_loader_tests(void);
void register_wifi_credentials_store_tests(void);

// Native entrypoint
#ifdef NATIVE_PLATFORM
#include <LittleFS.h>
#include <native_wifi_cred_eeprom.h>
#include <Preferences.h>
#include <storage/storage_manager.h>

extern "C" void setUp(void) {
  // Reset the in-memory Preferences/LittleFS/EEPROM fakes and
  // StorageManager's static driver registry before every test so storage
  // tests never see state left behind by a previous one.
  NativePreferencesFake::reset();
  NativeFsFake::reset();
  NativeEEPROMFake::reset();
  StorageManager::clearAllDrivers();
}
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
  runAuthUtilsTests();
  register_auth_decision_tests();
  register_query_builder_tests();
  register_json_database_driver_tests();
  register_littlefs_database_driver_tests();
  register_storage_manager_tests();
  register_auth_storage_tests();
  register_openapi_spec_helpers_tests();
  register_system_status_helpers_tests();
  register_certificate_loader_tests();
  register_wifi_credentials_store_tests();

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
  register_web_platform_boot_tests();

  UNITY_END();
}

void loop() {
  // No-op
}
#endif
