#include "storage/storage_manager.h"
#include <map>
#include <unity.h>

namespace {

// Same in-memory IDatabaseDriver test double used for QueryBuilder's tests
// (duplicated locally rather than shared - it's a handful of lines and
// keeps each test file independent).
class FakeDatabaseDriver : public IDatabaseDriver {
private:
  std::map<String, std::map<String, String>> data;
  String name;

public:
  explicit FakeDatabaseDriver(const String &driverName) : name(driverName) {}

  bool store(const String &collection, const String &key,
            const String &value) override {
    data[collection][key] = value;
    return true;
  }
  String retrieve(const String &collection, const String &key) override {
    return data[collection][key];
  }
  bool remove(const String &collection, const String &key) override {
    return data[collection].erase(key) > 0;
  }
  std::vector<String> listKeys(const String &collection) override {
    std::vector<String> keys;
    for (const auto &entry : data[collection]) {
      keys.push_back(entry.first);
    }
    return keys;
  }
  bool exists(const String &collection, const String &key) override {
    return data[collection].count(key) > 0;
  }
  String getDriverName() const override { return name; }
};

} // namespace

void test_storage_manager_default_driver_is_json(void) {
  TEST_ASSERT_EQUAL_STRING("json", StorageManager::getDefaultDriverName().c_str());
}

void test_storage_manager_ensures_json_and_littlefs_exist_by_default(void) {
  std::vector<String> names = StorageManager::getDriverNames();
  bool hasJson = false, hasLittlefs = false;
  for (const String &n : names) {
    if (n == "json")
      hasJson = true;
    if (n == "littlefs")
      hasLittlefs = true;
  }
  TEST_ASSERT_TRUE(hasJson);
  TEST_ASSERT_TRUE(hasLittlefs);
}

void test_storage_manager_configure_driver_registers_it(void) {
  StorageManager::configureDriver(
      "fake", std::unique_ptr<IDatabaseDriver>(new FakeDatabaseDriver("fake")));

  IDatabaseDriver &d = StorageManager::driver("fake");
  TEST_ASSERT_EQUAL_STRING("fake", d.getDriverName().c_str());
}

void test_storage_manager_set_default_driver_switches_default(void) {
  StorageManager::configureDriver(
      "fake", std::unique_ptr<IDatabaseDriver>(new FakeDatabaseDriver("fake")));

  TEST_ASSERT_TRUE(StorageManager::setDefaultDriver("fake"));
  TEST_ASSERT_EQUAL_STRING("fake",
                           StorageManager::getDefaultDriverName().c_str());
  TEST_ASSERT_EQUAL_STRING("fake", StorageManager::driver("").getDriverName().c_str());
}

void test_storage_manager_set_default_driver_rejects_unknown_name(void) {
  TEST_ASSERT_FALSE(StorageManager::setDefaultDriver("does-not-exist"));
  TEST_ASSERT_EQUAL_STRING("json", StorageManager::getDefaultDriverName().c_str());
}

void test_storage_manager_driver_falls_back_to_default_when_unknown(void) {
  IDatabaseDriver &d = StorageManager::driver("does-not-exist");
  TEST_ASSERT_EQUAL_STRING("json", d.getDriverName().c_str());
}

void test_storage_manager_query_uses_default_driver(void) {
  StorageManager::configureDriver(
      "fake", std::unique_ptr<IDatabaseDriver>(new FakeDatabaseDriver("fake")));
  StorageManager::setDefaultDriver("fake");

  QueryBuilder qb = StorageManager::query("widgets");
  TEST_ASSERT_TRUE(qb.store("w1", "{\"name\":\"gadget\"}"));
  TEST_ASSERT_EQUAL_STRING("{\"name\":\"gadget\"}", qb.get().c_str());
}

void test_storage_manager_remove_driver_removes_it(void) {
  StorageManager::configureDriver(
      "fake", std::unique_ptr<IDatabaseDriver>(new FakeDatabaseDriver("fake")));
  TEST_ASSERT_TRUE(StorageManager::removeDriver("fake"));

  IDatabaseDriver &d = StorageManager::driver("fake");
  TEST_ASSERT_EQUAL_STRING("json", d.getDriverName().c_str());
}

void test_storage_manager_remove_driver_refuses_json(void) {
  TEST_ASSERT_FALSE(StorageManager::removeDriver("json"));
  IDatabaseDriver &d = StorageManager::driver("json");
  TEST_ASSERT_EQUAL_STRING("json", d.getDriverName().c_str());
}

void test_storage_manager_remove_default_driver_falls_back_to_json(void) {
  StorageManager::configureDriver(
      "fake", std::unique_ptr<IDatabaseDriver>(new FakeDatabaseDriver("fake")));
  StorageManager::setDefaultDriver("fake");

  TEST_ASSERT_TRUE(StorageManager::removeDriver("fake"));
  TEST_ASSERT_EQUAL_STRING("json", StorageManager::getDefaultDriverName().c_str());
}

void test_storage_manager_clear_all_drivers_resets_to_defaults(void) {
  StorageManager::configureDriver(
      "fake", std::unique_ptr<IDatabaseDriver>(new FakeDatabaseDriver("fake")));
  StorageManager::setDefaultDriver("fake");

  StorageManager::clearAllDrivers();

  TEST_ASSERT_EQUAL_STRING("json", StorageManager::getDefaultDriverName().c_str());
  std::vector<String> names = StorageManager::getDriverNames();
  bool hasFake = false;
  for (const String &n : names) {
    if (n == "fake")
      hasFake = true;
  }
  TEST_ASSERT_FALSE(hasFake);
}

void register_storage_manager_tests(void) {
  RUN_TEST(test_storage_manager_default_driver_is_json);
  RUN_TEST(test_storage_manager_ensures_json_and_littlefs_exist_by_default);
  RUN_TEST(test_storage_manager_configure_driver_registers_it);
  RUN_TEST(test_storage_manager_set_default_driver_switches_default);
  RUN_TEST(test_storage_manager_set_default_driver_rejects_unknown_name);
  RUN_TEST(test_storage_manager_driver_falls_back_to_default_when_unknown);
  RUN_TEST(test_storage_manager_query_uses_default_driver);
  RUN_TEST(test_storage_manager_remove_driver_removes_it);
  RUN_TEST(test_storage_manager_remove_driver_refuses_json);
  RUN_TEST(test_storage_manager_remove_default_driver_falls_back_to_json);
  RUN_TEST(test_storage_manager_clear_all_drivers_resets_to_defaults);
}
