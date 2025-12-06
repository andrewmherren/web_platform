#include "types/navigation_types.h"
#include <ArduinoFake.h>
#include <unity.h>

void test_navigation_item_basic_constructor(void) {
  // Test basic two-parameter constructor (name, url)
  String name = "Home";
  String url = "/home";

  NavigationItem item(name, url);

  TEST_ASSERT_NOT_NULL(item.name);
  TEST_ASSERT_NOT_NULL(item.url);
  TEST_ASSERT_EQUAL_STRING("Home", item.name);
  TEST_ASSERT_EQUAL_STRING("/home", item.url);
  TEST_ASSERT_EQUAL_STRING("", item.target);
  TEST_ASSERT_EQUAL(NavAuthVisibility::ALWAYS, item.visibility);
}

void test_navigation_item_three_parameter_constructor(void) {
  // Test three-parameter constructor (name, url, target)
  String name = "External";
  String url = "https://example.com";
  String target = "_blank";

  NavigationItem item(name, url, target);

  TEST_ASSERT_EQUAL_STRING("External", item.name);
  TEST_ASSERT_EQUAL_STRING("https://example.com", item.url);
  TEST_ASSERT_EQUAL_STRING("_blank", item.target);
  TEST_ASSERT_EQUAL(NavAuthVisibility::ALWAYS, item.visibility);
}

void test_navigation_item_with_visibility(void) {
  // Test constructor with visibility (name, url, visibility)
  String name = "Admin";
  String url = "/admin";

  NavigationItem item(name, url, NavAuthVisibility::AUTHENTICATED);

  TEST_ASSERT_EQUAL_STRING("Admin", item.name);
  TEST_ASSERT_EQUAL_STRING("/admin", item.url);
  TEST_ASSERT_EQUAL_STRING("", item.target);
  TEST_ASSERT_EQUAL(NavAuthVisibility::AUTHENTICATED, item.visibility);
}

void test_navigation_item_full_constructor(void) {
  // Test four-parameter constructor (name, url, target, visibility)
  String name = "Settings";
  String url = "/settings";
  String target = "_self";

  NavigationItem item(name, url, target, NavAuthVisibility::UNAUTHENTICATED);

  TEST_ASSERT_EQUAL_STRING("Settings", item.name);
  TEST_ASSERT_EQUAL_STRING("/settings", item.url);
  TEST_ASSERT_EQUAL_STRING("_self", item.target);
  TEST_ASSERT_EQUAL(NavAuthVisibility::UNAUTHENTICATED, item.visibility);
}

void test_navigation_item_empty_strings(void) {
  // Test construction with empty strings
  String name = "";
  String url = "";
  String target = "";

  NavigationItem item(name, url, target);

  TEST_ASSERT_NOT_NULL(item.name);
  TEST_ASSERT_NOT_NULL(item.url);
  TEST_ASSERT_NOT_NULL(item.target);
  TEST_ASSERT_EQUAL_STRING("", item.name);
  TEST_ASSERT_EQUAL_STRING("", item.url);
  TEST_ASSERT_EQUAL_STRING("", item.target);
}

void test_navigation_item_multiple_instances(void) {
  // Test that multiple NavigationItem instances work correctly
  String name1 = "Dashboard";
  String url1 = "/dashboard";
  String name2 = "Profile";
  String url2 = "/profile";

  NavigationItem item1(name1, url1);
  NavigationItem item2(name2, url2);

  // Both items should have distinct string pointers
  TEST_ASSERT_NOT_EQUAL(item1.name, item2.name);
  TEST_ASSERT_NOT_EQUAL(item1.url, item2.url);

  // But the content should match
  TEST_ASSERT_EQUAL_STRING("Dashboard", item1.name);
  TEST_ASSERT_EQUAL_STRING("/dashboard", item1.url);
  TEST_ASSERT_EQUAL_STRING("Profile", item2.name);
  TEST_ASSERT_EQUAL_STRING("/profile", item2.url);
}

void test_navigation_item_special_characters(void) {
  // Test with special characters in navigation items
  String name = "API & Documentation";
  String url = "/api-docs?version=1.0&format=json";
  String target = "_blank";

  NavigationItem item(name, url, target);

  TEST_ASSERT_EQUAL_STRING("API & Documentation", item.name);
  TEST_ASSERT_EQUAL_STRING("/api-docs?version=1.0&format=json", item.url);
  TEST_ASSERT_EQUAL_STRING("_blank", item.target);
}

void test_navigation_item_all_visibility_types(void) {
  // Test all visibility enumeration values
  NavigationItem always("Always", "/always", NavAuthVisibility::ALWAYS);
  NavigationItem authenticated("Auth", "/auth",
                               NavAuthVisibility::AUTHENTICATED);
  NavigationItem unauthenticated("Guest", "/guest",
                                 NavAuthVisibility::UNAUTHENTICATED);

  TEST_ASSERT_EQUAL(NavAuthVisibility::ALWAYS, always.visibility);
  TEST_ASSERT_EQUAL(NavAuthVisibility::AUTHENTICATED, authenticated.visibility);
  TEST_ASSERT_EQUAL(NavAuthVisibility::UNAUTHENTICATED,
                    unauthenticated.visibility);
}

void test_navigation_item_memory_efficiency(void) {
  // Test that the string pool is being used efficiently
  NavigationItem items[5] = {NavigationItem(String("Item1"), String("/item1")),
                             NavigationItem(String("Item2"), String("/item2")),
                             NavigationItem(String("Item3"), String("/item3")),
                             NavigationItem(String("Item4"), String("/item4")),
                             NavigationItem(String("Item5"), String("/item5"))};

  // Verify all items are properly initialized
  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_NOT_NULL(items[i].name);
    TEST_ASSERT_NOT_NULL(items[i].url);
  }

  // Spot check values
  TEST_ASSERT_EQUAL_STRING("Item1", items[0].name);
  TEST_ASSERT_EQUAL_STRING("/item1", items[0].url);
  TEST_ASSERT_EQUAL_STRING("Item5", items[4].name);
  TEST_ASSERT_EQUAL_STRING("/item5", items[4].url);
}

void register_navigation_types_tests(void) {
  RUN_TEST(test_navigation_item_basic_constructor);
  RUN_TEST(test_navigation_item_three_parameter_constructor);
  RUN_TEST(test_navigation_item_with_visibility);
  RUN_TEST(test_navigation_item_full_constructor);
  RUN_TEST(test_navigation_item_empty_strings);
  RUN_TEST(test_navigation_item_multiple_instances);
  RUN_TEST(test_navigation_item_special_characters);
  RUN_TEST(test_navigation_item_all_visibility_types);
  RUN_TEST(test_navigation_item_memory_efficiency);
}
