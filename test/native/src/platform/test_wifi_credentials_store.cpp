#include "platform/wifi_credentials_store.h"
#include <native_wifi_cred_eeprom.h>
#include <unity.h>

using namespace WiFiCredentialsStore;

void test_load_before_any_save_returns_false(void) {
  begin();
  String ssid, password;
  TEST_ASSERT_FALSE(load(ssid, password));
}

void test_save_then_load_roundtrips_credentials(void) {
  begin();
  TEST_ASSERT_TRUE(save("MyNetwork", "hunter2"));

  String ssid, password;
  TEST_ASSERT_TRUE(load(ssid, password));
  TEST_ASSERT_EQUAL_STRING("MyNetwork", ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("hunter2", password.c_str());
}

void test_save_then_reset_makes_load_return_false(void) {
  begin();
  save("SomeNetwork", "somepassword");
  reset();

  String ssid, password;
  TEST_ASSERT_FALSE(load(ssid, password));
}

void test_reset_does_not_erase_underlying_bytes(void) {
  // Documents existing behavior carried over from the pre-extraction
  // implementation: reset() only clears the "configured" flag, not the
  // SSID/password bytes themselves - a fresh save() after reset() still
  // overwrites cleanly, but nothing zeroes the old bytes in between.
  begin();
  save("OldNetwork", "oldpassword");
  reset();
  TEST_ASSERT_EQUAL(0, NativeWifiCredEeprom.read(CONFIG_FLAG_ADDR));
  TEST_ASSERT_EQUAL('O', NativeWifiCredEeprom.read(SSID_ADDR));
}

void test_save_overwrites_previous_shorter_credentials(void) {
  begin();
  save("LongerNetworkName", "longerpassword1");
  TEST_ASSERT_TRUE(save("Net", "pw"));

  String ssid, password;
  TEST_ASSERT_TRUE(load(ssid, password));
  TEST_ASSERT_EQUAL_STRING("Net", ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("pw", password.c_str());
}

void test_save_with_empty_password_roundtrips(void) {
  begin();
  TEST_ASSERT_TRUE(save("OpenNetwork", ""));

  String ssid, password;
  TEST_ASSERT_TRUE(load(ssid, password));
  TEST_ASSERT_EQUAL_STRING("OpenNetwork", ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("", password.c_str());
}

void test_save_with_empty_ssid_makes_load_return_false(void) {
  // load() treats an empty stored SSID as "no valid credentials", even
  // though the configured flag was set - matches the pre-extraction
  // implementation exactly.
  begin();
  save("", "somepassword");

  String ssid, password;
  TEST_ASSERT_FALSE(load(ssid, password));
}

void register_wifi_credentials_store_tests(void) {
  RUN_TEST(test_load_before_any_save_returns_false);
  RUN_TEST(test_save_then_load_roundtrips_credentials);
  RUN_TEST(test_save_then_reset_makes_load_return_false);
  RUN_TEST(test_reset_does_not_erase_underlying_bytes);
  RUN_TEST(test_save_overwrites_previous_shorter_credentials);
  RUN_TEST(test_save_with_empty_password_roundtrips);
  RUN_TEST(test_save_with_empty_ssid_makes_load_return_false);
}
