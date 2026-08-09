#include "storage/query_builder.h"
#include <map>
#include <unity.h>

namespace {

// Minimal in-memory IDatabaseDriver test double - QueryBuilder only talks to
// drivers through the abstract interface, so this is enough to exercise its
// where()/limit()/get()/getAll()/store()/remove() logic without any real
// backend (Preferences/LittleFS aren't natively fakeable today - see
// PROJECT_PLAN.md's Phase 2b storage note).
class FakeDatabaseDriver : public IDatabaseDriver {
private:
  std::map<String, std::map<String, String>> data;

public:
  bool store(const String &collection, const String &key,
            const String &value) override {
    data[collection][key] = value;
    return true;
  }

  String retrieve(const String &collection, const String &key) override {
    auto colIt = data.find(collection);
    if (colIt == data.end())
      return "";
    auto keyIt = colIt->second.find(key);
    return keyIt == colIt->second.end() ? "" : keyIt->second;
  }

  bool remove(const String &collection, const String &key) override {
    auto colIt = data.find(collection);
    if (colIt == data.end())
      return false;
    return colIt->second.erase(key) > 0;
  }

  std::vector<String> listKeys(const String &collection) override {
    std::vector<String> keys;
    auto colIt = data.find(collection);
    if (colIt == data.end())
      return keys;
    for (const auto &entry : colIt->second) {
      keys.push_back(entry.first);
    }
    return keys;
  }

  bool exists(const String &collection, const String &key) override {
    auto colIt = data.find(collection);
    if (colIt == data.end())
      return false;
    return colIt->second.find(key) != colIt->second.end();
  }

  String getDriverName() const override { return "fake"; }
};

} // namespace

void test_query_builder_null_driver_is_safe(void) {
  QueryBuilder qb(nullptr, "users");
  TEST_ASSERT_EQUAL_STRING("", qb.get().c_str());
  TEST_ASSERT_TRUE(qb.getAll().empty());
  TEST_ASSERT_FALSE(qb.exists());
  TEST_ASSERT_FALSE(qb.store("k", "v"));
  TEST_ASSERT_FALSE(qb.remove());
}

void test_query_builder_get_without_conditions_returns_first_key(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");

  QueryBuilder qb(&driver, "users");
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"alice\"}", qb.get().c_str());
}

void test_query_builder_where_filters_by_matching_field(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");
  driver.store("users", "u2", "{\"username\":\"bob\"}");

  QueryBuilder qb(&driver, "users");
  String result = qb.where("username", "bob").get();
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"bob\"}", result.c_str());
}

void test_query_builder_where_no_match_returns_empty(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");

  QueryBuilder qb(&driver, "users");
  String result = qb.where("username", "nobody").get();
  TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_query_builder_where_ignores_malformed_json(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "not valid json");
  driver.store("users", "u2", "{\"username\":\"bob\"}");

  QueryBuilder qb(&driver, "users");
  String result = qb.where("username", "bob").get();
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"bob\"}", result.c_str());
}

void test_query_builder_get_all_returns_every_record(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");
  driver.store("users", "u2", "{\"username\":\"bob\"}");

  QueryBuilder qb(&driver, "users");
  std::vector<String> results = qb.getAll();
  TEST_ASSERT_EQUAL(2, results.size());
}

void test_query_builder_get_all_filters_by_condition(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"role\":\"admin\"}");
  driver.store("users", "u2", "{\"role\":\"user\"}");
  driver.store("users", "u3", "{\"role\":\"admin\"}");

  QueryBuilder qb(&driver, "users");
  std::vector<String> results = qb.where("role", "admin").getAll();
  TEST_ASSERT_EQUAL(2, results.size());
}

void test_query_builder_limit_caps_get_all_results(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"role\":\"admin\"}");
  driver.store("users", "u2", "{\"role\":\"admin\"}");
  driver.store("users", "u3", "{\"role\":\"admin\"}");

  QueryBuilder qb(&driver, "users");
  std::vector<String> results = qb.limit(2).getAll();
  TEST_ASSERT_EQUAL(2, results.size());
}

void test_query_builder_exists_true_when_match_found(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");

  QueryBuilder qb(&driver, "users");
  TEST_ASSERT_TRUE(qb.where("username", "alice").exists());
}

void test_query_builder_exists_false_when_no_match(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");

  QueryBuilder qb(&driver, "users");
  TEST_ASSERT_FALSE(qb.where("username", "nobody").exists());
}

void test_query_builder_store_writes_through_to_driver(void) {
  FakeDatabaseDriver driver;
  QueryBuilder qb(&driver, "users");

  TEST_ASSERT_TRUE(qb.store("u1", "{\"username\":\"alice\"}"));
  TEST_ASSERT_EQUAL_STRING("{\"username\":\"alice\"}",
                           driver.retrieve("users", "u1").c_str());
}

void test_query_builder_remove_without_conditions_removes_all(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"username\":\"alice\"}");
  driver.store("users", "u2", "{\"username\":\"bob\"}");

  QueryBuilder qb(&driver, "users");
  TEST_ASSERT_TRUE(qb.remove());
  TEST_ASSERT_TRUE(driver.listKeys("users").empty());
}

void test_query_builder_remove_with_condition_removes_only_matches(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"role\":\"admin\"}");
  driver.store("users", "u2", "{\"role\":\"user\"}");

  QueryBuilder qb(&driver, "users");
  TEST_ASSERT_TRUE(qb.where("role", "admin").remove());
  TEST_ASSERT_EQUAL(1, driver.listKeys("users").size());
  TEST_ASSERT_EQUAL_STRING("{\"role\":\"user\"}",
                           driver.retrieve("users", "u2").c_str());
}

void test_query_builder_remove_no_match_returns_false(void) {
  FakeDatabaseDriver driver;
  driver.store("users", "u1", "{\"role\":\"user\"}");

  QueryBuilder qb(&driver, "users");
  TEST_ASSERT_FALSE(qb.where("role", "admin").remove());
}

void test_query_builder_getters_reflect_construction(void) {
  FakeDatabaseDriver driver;
  QueryBuilder qb(&driver, "sessions");
  TEST_ASSERT_EQUAL_PTR(&driver, qb.getDriver());
  TEST_ASSERT_EQUAL_STRING("sessions", qb.getCollection().c_str());
}

void register_query_builder_tests(void) {
  RUN_TEST(test_query_builder_null_driver_is_safe);
  RUN_TEST(test_query_builder_get_without_conditions_returns_first_key);
  RUN_TEST(test_query_builder_where_filters_by_matching_field);
  RUN_TEST(test_query_builder_where_no_match_returns_empty);
  RUN_TEST(test_query_builder_where_ignores_malformed_json);
  RUN_TEST(test_query_builder_get_all_returns_every_record);
  RUN_TEST(test_query_builder_get_all_filters_by_condition);
  RUN_TEST(test_query_builder_limit_caps_get_all_results);
  RUN_TEST(test_query_builder_exists_true_when_match_found);
  RUN_TEST(test_query_builder_exists_false_when_no_match);
  RUN_TEST(test_query_builder_store_writes_through_to_driver);
  RUN_TEST(test_query_builder_remove_without_conditions_removes_all);
  RUN_TEST(test_query_builder_remove_with_condition_removes_only_matches);
  RUN_TEST(test_query_builder_remove_no_match_returns_false);
  RUN_TEST(test_query_builder_getters_reflect_construction);
}
