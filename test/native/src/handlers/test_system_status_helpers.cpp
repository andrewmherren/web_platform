#include "handlers/system_status_helpers.h"
#include <unity.h>

using namespace SystemStatusHelpers;

void test_clamp_percent_within_range_unchanged(void) {
  TEST_ASSERT_EQUAL(50, clampPercent(50));
}

void test_clamp_percent_below_zero_clamps_to_zero(void) {
  TEST_ASSERT_EQUAL(0, clampPercent(-5));
}

void test_clamp_percent_above_hundred_clamps_to_hundred(void) {
  TEST_ASSERT_EQUAL(100, clampPercent(150));
}

void test_clamp_percent_boundary_values(void) {
  TEST_ASSERT_EQUAL(0, clampPercent(0));
  TEST_ASSERT_EQUAL(100, clampPercent(100));
}

void test_heap_health_color_danger_below_20(void) {
  TEST_ASSERT_EQUAL_STRING("danger", heapHealthColor(19).c_str());
  TEST_ASSERT_EQUAL_STRING("danger", heapHealthColor(0).c_str());
}

void test_heap_health_color_warning_between_20_and_39(void) {
  TEST_ASSERT_EQUAL_STRING("warning", heapHealthColor(20).c_str());
  TEST_ASSERT_EQUAL_STRING("warning", heapHealthColor(39).c_str());
}

void test_heap_health_color_good_at_or_above_40(void) {
  TEST_ASSERT_EQUAL_STRING("good", heapHealthColor(40).c_str());
  TEST_ASSERT_EQUAL_STRING("good", heapHealthColor(100).c_str());
}

void test_storage_health_color_good_at_or_below_60(void) {
  TEST_ASSERT_EQUAL_STRING("good", storageHealthColor(0).c_str());
  TEST_ASSERT_EQUAL_STRING("good", storageHealthColor(60).c_str());
}

void test_storage_health_color_warning_between_61_and_80(void) {
  TEST_ASSERT_EQUAL_STRING("warning", storageHealthColor(61).c_str());
  TEST_ASSERT_EQUAL_STRING("warning", storageHealthColor(80).c_str());
}

void test_storage_health_color_danger_above_80(void) {
  TEST_ASSERT_EQUAL_STRING("danger", storageHealthColor(81).c_str());
  TEST_ASSERT_EQUAL_STRING("danger", storageHealthColor(100).c_str());
}

void register_system_status_helpers_tests(void) {
  RUN_TEST(test_clamp_percent_within_range_unchanged);
  RUN_TEST(test_clamp_percent_below_zero_clamps_to_zero);
  RUN_TEST(test_clamp_percent_above_hundred_clamps_to_hundred);
  RUN_TEST(test_clamp_percent_boundary_values);

  RUN_TEST(test_heap_health_color_danger_below_20);
  RUN_TEST(test_heap_health_color_warning_between_20_and_39);
  RUN_TEST(test_heap_health_color_good_at_or_above_40);

  RUN_TEST(test_storage_health_color_good_at_or_below_60);
  RUN_TEST(test_storage_health_color_warning_between_61_and_80);
  RUN_TEST(test_storage_health_color_danger_above_80);
}
