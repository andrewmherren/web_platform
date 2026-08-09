#include "storage/json_database_driver.h"
#include <unity.h>

void test_json_driver_retrieve_missing_key_returns_empty(void) {
  JsonDatabaseDriver driver;
  TEST_ASSERT_EQUAL_STRING("", driver.retrieve("users", "nobody").c_str());
}

void test_json_driver_store_and_retrieve_roundtrip(void) {
  JsonDatabaseDriver driver;
  TEST_ASSERT_TRUE(driver.store("users", "u1", "{\"username\":\"alice\"}"));
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"alice\"}",
                           driver.retrieve("users", "u1").c_str());
}

void test_json_driver_store_rejects_empty_collection_or_key(void) {
  JsonDatabaseDriver driver;
  TEST_ASSERT_FALSE(driver.store("", "u1", "data"));
  TEST_ASSERT_FALSE(driver.store("users", "", "data"));
}

void test_json_driver_exists_reflects_stored_keys(void) {
  JsonDatabaseDriver driver;
  TEST_ASSERT_FALSE(driver.exists("users", "u1"));
  driver.store("users", "u1", "{\"a\":1}");
  TEST_ASSERT_TRUE(driver.exists("users", "u1"));
}

void test_json_driver_remove_deletes_key(void) {
  JsonDatabaseDriver driver;
  driver.store("users", "u1", "{\"a\":1}");
  TEST_ASSERT_TRUE(driver.remove("users", "u1"));
  TEST_ASSERT_FALSE(driver.exists("users", "u1"));
}

void test_json_driver_remove_missing_key_returns_false(void) {
  JsonDatabaseDriver driver;
  TEST_ASSERT_FALSE(driver.remove("users", "nobody"));
}

void test_json_driver_list_keys_reflects_all_stored_entries(void) {
  JsonDatabaseDriver driver;
  driver.store("users", "u1", "{}");
  driver.store("users", "u2", "{}");
  std::vector<String> keys = driver.listKeys("users");
  TEST_ASSERT_EQUAL(2, keys.size());
}

void test_json_driver_collections_are_independent(void) {
  JsonDatabaseDriver driver;
  driver.store("users", "u1", "{\"kind\":\"user\"}");
  driver.store("sessions", "u1", "{\"kind\":\"session\"}");
  TEST_ASSERT_EQUAL_STRING("{\"kind\":\"user\"}",
                           driver.retrieve("users", "u1").c_str());
  TEST_ASSERT_EQUAL_STRING("{\"kind\":\"session\"}",
                           driver.retrieve("sessions", "u1").c_str());
}

void test_json_driver_persists_across_instances(void) {
  // Each store()/retrieve() call opens and closes its own Preferences
  // session (mirrors real NVS usage) - a *new* driver instance backed by
  // the same fake NVS store should still see previously stored data,
  // unlike its in-process `cache` map which is per-instance.
  {
    JsonDatabaseDriver first;
    first.store("users", "u1", "{\"username\":\"alice\"}");
  }
  JsonDatabaseDriver second;
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"alice\"}",
                           second.retrieve("users", "u1").c_str());
}

void test_json_driver_get_driver_name(void) {
  JsonDatabaseDriver driver;
  TEST_ASSERT_EQUAL_STRING("json", driver.getDriverName().c_str());
}

void register_json_database_driver_tests(void) {
  RUN_TEST(test_json_driver_retrieve_missing_key_returns_empty);
  RUN_TEST(test_json_driver_store_and_retrieve_roundtrip);
  RUN_TEST(test_json_driver_store_rejects_empty_collection_or_key);
  RUN_TEST(test_json_driver_exists_reflects_stored_keys);
  RUN_TEST(test_json_driver_remove_deletes_key);
  RUN_TEST(test_json_driver_remove_missing_key_returns_false);
  RUN_TEST(test_json_driver_list_keys_reflects_all_stored_entries);
  RUN_TEST(test_json_driver_collections_are_independent);
  RUN_TEST(test_json_driver_persists_across_instances);
  RUN_TEST(test_json_driver_get_driver_name);
}
