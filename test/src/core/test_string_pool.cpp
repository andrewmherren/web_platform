#include "core/string_pool.h"
#include <string>
#include <unity.h>

using namespace WebPlatform::Core;

void test_store_returns_valid_pointer() {
  StringPool pool;
  const char *result = pool.store("test");
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("test", result);
}

void test_store_empty_string_returns_nullptr() {
  StringPool pool;
  const char *result = pool.store("");
  TEST_ASSERT_NULL(result);
}

void test_store_nullptr_returns_nullptr() {
  StringPool pool;
  const char *result = pool.store(nullptr);
  TEST_ASSERT_NULL(result);
}

void test_store_deduplicates_strings() {
  StringPool pool;
  const char *first = pool.store("duplicate");
  const char *second = pool.store("duplicate");

  // Should return same pointer for duplicate strings
  TEST_ASSERT_EQUAL_PTR(first, second);
  TEST_ASSERT_EQUAL_UINT(1, pool.size());
}

void test_store_different_strings() {
  StringPool pool;
  const char *first = pool.store("first");
  const char *second = pool.store("second");

  // Should be different pointers
  TEST_ASSERT_NOT_EQUAL(first, second);
  TEST_ASSERT_EQUAL_STRING("first", first);
  TEST_ASSERT_EQUAL_STRING("second", second);
  TEST_ASSERT_EQUAL_UINT(2, pool.size());
}

void test_empty_returns_nullptr() {
  StringPool pool;
  TEST_ASSERT_NULL(pool.empty());
}

void test_size_starts_at_zero() {
  StringPool pool;
  TEST_ASSERT_EQUAL_UINT(0, pool.size());
}

void test_size_increases_with_unique_strings() {
  StringPool pool;
  pool.store("one");
  TEST_ASSERT_EQUAL_UINT(1, pool.size());

  pool.store("two");
  TEST_ASSERT_EQUAL_UINT(2, pool.size());

  pool.store("one"); // Duplicate
  TEST_ASSERT_EQUAL_UINT(2, pool.size());
}

void test_memory_usage_calculation() {
  StringPool pool;
  pool.store("abc");    // 4 bytes (3 + null terminator)
  pool.store("abcdef"); // 7 bytes (6 + null terminator)

  size_t usage = pool.memoryUsage();
  TEST_ASSERT_EQUAL_UINT(11, usage); // 4 + 7
}

void test_seal_prevents_new_stores() {
  StringPool pool;
  pool.store("before_seal");
  TEST_ASSERT_EQUAL_UINT(1, pool.size());

  pool.seal();
  TEST_ASSERT_TRUE(pool.isSealed());

  const char *result = pool.store("after_seal");
  TEST_ASSERT_NULL(result);               // Should fail
  TEST_ASSERT_EQUAL_UINT(1, pool.size()); // Size unchanged
}

void test_clear_removes_strings() {
  StringPool pool;
  pool.store("test1");
  pool.store("test2");
  TEST_ASSERT_EQUAL_UINT(2, pool.size());

  pool.clear();
  TEST_ASSERT_EQUAL_UINT(0, pool.size());
}

void test_clear_does_nothing_when_sealed() {
  StringPool pool;
  pool.store("test");
  pool.seal();
  TEST_ASSERT_EQUAL_UINT(1, pool.size());

  pool.clear();
  TEST_ASSERT_EQUAL_UINT(1, pool.size()); // Still 1
}

void test_capacity_management() {
  StringPool pool;
  TEST_ASSERT_EQUAL_UINT(64, pool.capacity()); // Default capacity

  pool.reserve(128);
  TEST_ASSERT_EQUAL_UINT(128, pool.capacity());
}

void test_capacity_exceeded() {
  StringPool pool;
  // Create a pool with very small capacity
  pool.reserve(2);

  const char *first = pool.store("first");
  TEST_ASSERT_NOT_NULL(first);

  const char *second = pool.store("second");
  TEST_ASSERT_NOT_NULL(second);

  // Third should fail due to capacity
  const char *third = pool.store("third");
  TEST_ASSERT_NULL(third);
  TEST_ASSERT_EQUAL_UINT(2, pool.size());
}

void test_std_string_overload() {
  StringPool pool;
  std::string stdStr = "std_string_test";
  const char *result = pool.store(stdStr);

  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("std_string_test", result);
}

void test_pointer_stability() {
  StringPool pool;
  const char *ptr1 = pool.store("stable");
  pool.store("another");
  pool.store("yet_another");

  // Original pointer should still be valid
  TEST_ASSERT_EQUAL_STRING("stable", ptr1);
}

// Test runner
void runStringPoolTests() {
  RUN_TEST(test_store_returns_valid_pointer);
  RUN_TEST(test_store_empty_string_returns_nullptr);
  RUN_TEST(test_store_nullptr_returns_nullptr);
  RUN_TEST(test_store_deduplicates_strings);
  RUN_TEST(test_store_different_strings);
  RUN_TEST(test_empty_returns_nullptr);
  RUN_TEST(test_size_starts_at_zero);
  RUN_TEST(test_size_increases_with_unique_strings);
  RUN_TEST(test_memory_usage_calculation);
  RUN_TEST(test_seal_prevents_new_stores);
  RUN_TEST(test_clear_removes_strings);
  RUN_TEST(test_clear_does_nothing_when_sealed);
  RUN_TEST(test_capacity_management);
  RUN_TEST(test_capacity_exceeded);
  RUN_TEST(test_std_string_overload);
  RUN_TEST(test_pointer_stability);
}
