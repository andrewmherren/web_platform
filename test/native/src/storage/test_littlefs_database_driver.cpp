#include "storage/littlefs_database_driver.h"
#include <unity.h>

void test_littlefs_driver_retrieve_missing_key_returns_empty(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  TEST_ASSERT_EQUAL_STRING("", driver.retrieve("users", "nobody").c_str());
}

void test_littlefs_driver_store_and_retrieve_roundtrip(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  TEST_ASSERT_TRUE(driver.store("users", "u1", "{\"username\":\"alice\"}"));
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"alice\"}",
                           driver.retrieve("users", "u1").c_str());
}

void test_littlefs_driver_rejects_invalid_names(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  TEST_ASSERT_FALSE(driver.store("", "u1", "data"));
  TEST_ASSERT_FALSE(driver.store("users", "", "data"));
  TEST_ASSERT_FALSE(driver.store("users", "..", "data"));
  TEST_ASSERT_FALSE(driver.store("users", ".hidden", "data"));
  TEST_ASSERT_FALSE(driver.store("us<er>s", "u1", "data"));
}

void test_littlefs_driver_exists_reflects_stored_keys(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  TEST_ASSERT_FALSE(driver.exists("users", "u1"));
  driver.store("users", "u1", "{}");
  TEST_ASSERT_TRUE(driver.exists("users", "u1"));
}

void test_littlefs_driver_remove_deletes_key(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  driver.store("users", "u1", "{}");
  TEST_ASSERT_TRUE(driver.remove("users", "u1"));
  TEST_ASSERT_FALSE(driver.exists("users", "u1"));
}

void test_littlefs_driver_remove_missing_key_returns_false(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  TEST_ASSERT_FALSE(driver.remove("users", "nobody"));
}

void test_littlefs_driver_list_keys_reflects_all_stored_entries(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  driver.store("users", "u1", "{}");
  driver.store("users", "u2", "{}");
  std::vector<String> keys = driver.listKeys("users");
  TEST_ASSERT_EQUAL(2, keys.size());
  bool hasU1 = false, hasU2 = false;
  for (const String &k : keys) {
    if (k == "u1")
      hasU1 = true;
    if (k == "u2")
      hasU2 = true;
  }
  TEST_ASSERT_TRUE(hasU1);
  TEST_ASSERT_TRUE(hasU2);
}

void test_littlefs_driver_collections_are_independent(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  driver.store("users", "u1", "{\"kind\":\"user\"}");
  driver.store("sessions", "u1", "{\"kind\":\"session\"}");
  TEST_ASSERT_EQUAL_STRING("{\"kind\":\"user\"}",
                           driver.retrieve("users", "u1").c_str());
  TEST_ASSERT_EQUAL_STRING("{\"kind\":\"session\"}",
                           driver.retrieve("sessions", "u1").c_str());
}

void test_littlefs_driver_overwrite_replaces_content(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  driver.store("users", "u1", "{\"v\":1}");
  driver.store("users", "u1", "{\"v\":2}");
  TEST_ASSERT_EQUAL_STRING("{\"v\":2}", driver.retrieve("users", "u1").c_str());
}

void test_littlefs_driver_get_driver_name(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  TEST_ASSERT_EQUAL_STRING("littlefs", driver.getDriverName().c_str());
}

void test_littlefs_driver_remove_collection_removes_all_keys(void) {
  LittleFSDatabaseDriver driver("/test_storage");
  driver.store("users", "u1", "{}");
  driver.store("users", "u2", "{}");
  TEST_ASSERT_TRUE(driver.removeCollection("users"));
  TEST_ASSERT_FALSE(driver.exists("users", "u1"));
  TEST_ASSERT_FALSE(driver.exists("users", "u2"));
}

void register_littlefs_database_driver_tests(void) {
  RUN_TEST(test_littlefs_driver_retrieve_missing_key_returns_empty);
  RUN_TEST(test_littlefs_driver_store_and_retrieve_roundtrip);
  RUN_TEST(test_littlefs_driver_rejects_invalid_names);
  RUN_TEST(test_littlefs_driver_exists_reflects_stored_keys);
  RUN_TEST(test_littlefs_driver_remove_deletes_key);
  RUN_TEST(test_littlefs_driver_remove_missing_key_returns_false);
  RUN_TEST(test_littlefs_driver_list_keys_reflects_all_stored_entries);
  RUN_TEST(test_littlefs_driver_collections_are_independent);
  RUN_TEST(test_littlefs_driver_overwrite_replaces_content);
  RUN_TEST(test_littlefs_driver_get_driver_name);
  RUN_TEST(test_littlefs_driver_remove_collection_removes_all_keys);
}
