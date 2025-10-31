#include <unity.h>
#include <ArduinoFake.h>
#include "../../../include/types/redirect_types.h"

void setUp(void) {
    ArduinoFakeReset();
}

void tearDown(void) {
    // Nothing to cleanup - string pool is static
}

void test_redirect_rule_basic_construction(void) {
    // Test basic construction of RedirectRule with String parameters
    String from = "/old-path";
    String to = "/new-path";
    
    RedirectRule rule(from, to);
    
    TEST_ASSERT_NOT_NULL(rule.fromPath);
    TEST_ASSERT_NOT_NULL(rule.toPath);
    TEST_ASSERT_EQUAL_STRING("/old-path", rule.fromPath);
    TEST_ASSERT_EQUAL_STRING("/new-path", rule.toPath);
}

void test_redirect_rule_empty_strings(void) {
    // Test construction with empty strings
    String from = "";
    String to = "";
    
    RedirectRule rule(from, to);
    
    TEST_ASSERT_NOT_NULL(rule.fromPath);
    TEST_ASSERT_NOT_NULL(rule.toPath);
    TEST_ASSERT_EQUAL_STRING("", rule.fromPath);
    TEST_ASSERT_EQUAL_STRING("", rule.toPath);
}

void test_redirect_rule_long_paths(void) {
    // Test construction with longer paths
    String from = "/some/very/long/path/that/might/be/used/in/practice";
    String to = "/another/long/destination/path/for/redirects";
    
    RedirectRule rule(from, to);
    
    TEST_ASSERT_NOT_NULL(rule.fromPath);
    TEST_ASSERT_NOT_NULL(rule.toPath);
    TEST_ASSERT_EQUAL_STRING(from.c_str(), rule.fromPath);
    TEST_ASSERT_EQUAL_STRING(to.c_str(), rule.toPath);
}

void test_redirect_rule_multiple_instances(void) {
    // Test that multiple RedirectRule instances work correctly
    String from1 = "/path1";
    String to1 = "/destination1";
    String from2 = "/path2";
    String to2 = "/destination2";
    
    RedirectRule rule1(from1, to1);
    RedirectRule rule2(from2, to2);
    
    // Both rules should have their own distinct string pointers
    TEST_ASSERT_NOT_EQUAL(rule1.fromPath, rule2.fromPath);
    TEST_ASSERT_NOT_EQUAL(rule1.toPath, rule2.toPath);
    
    // But the content should match what was provided
    TEST_ASSERT_EQUAL_STRING("/path1", rule1.fromPath);
    TEST_ASSERT_EQUAL_STRING("/destination1", rule1.toPath);
    TEST_ASSERT_EQUAL_STRING("/path2", rule2.fromPath);
    TEST_ASSERT_EQUAL_STRING("/destination2", rule2.toPath);
}

void test_redirect_rule_special_characters(void) {
    // Test with special characters that might appear in URLs
    String from = "/api/v1/users?param=value&other=123";
    String to = "/api/v2/users?param=value&other=123";
    
    RedirectRule rule(from, to);
    
    TEST_ASSERT_EQUAL_STRING(from.c_str(), rule.fromPath);
    TEST_ASSERT_EQUAL_STRING(to.c_str(), rule.toPath);
}

void test_redirect_rule_memory_efficiency(void) {
    // Test that the string pool is being used efficiently
    // Create multiple rules and verify they don't cause memory issues
    
    RedirectRule rules[5] = {
        RedirectRule(String("/a"), String("/1")),
        RedirectRule(String("/b"), String("/2")),
        RedirectRule(String("/c"), String("/3")),
        RedirectRule(String("/d"), String("/4")),
        RedirectRule(String("/e"), String("/5"))
    };
    
    // Verify all rules are properly initialized
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_NOT_NULL(rules[i].fromPath);
        TEST_ASSERT_NOT_NULL(rules[i].toPath);
    }
    
    // Spot check a few values
    TEST_ASSERT_EQUAL_STRING("/a", rules[0].fromPath);
    TEST_ASSERT_EQUAL_STRING("/1", rules[0].toPath);
    TEST_ASSERT_EQUAL_STRING("/e", rules[4].fromPath);
    TEST_ASSERT_EQUAL_STRING("/5", rules[4].toPath);
}

void register_redirect_types_tests(void) {
    RUN_TEST(test_redirect_rule_basic_construction);
    RUN_TEST(test_redirect_rule_empty_strings);
    RUN_TEST(test_redirect_rule_long_paths);
    RUN_TEST(test_redirect_rule_multiple_instances);
    RUN_TEST(test_redirect_rule_special_characters);
    RUN_TEST(test_redirect_rule_memory_efficiency);
}