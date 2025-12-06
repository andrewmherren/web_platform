#include "core/url_utils.h"
#include <unity.h>

using namespace WebPlatform::Core;

// ========== URL Decode Tests ==========

void test_decode_empty_string() {
  std::string result = UrlUtils::decode("");
  TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_decode_plain_text() {
  std::string result = UrlUtils::decode("hello");
  TEST_ASSERT_EQUAL_STRING("hello", result.c_str());
}

void test_decode_plus_to_space() {
  std::string result = UrlUtils::decode("hello+world");
  TEST_ASSERT_EQUAL_STRING("hello world", result.c_str());
}

void test_decode_percent_20_space() {
  std::string result = UrlUtils::decode("hello%20world");
  TEST_ASSERT_EQUAL_STRING("hello world", result.c_str());
}

void test_decode_mixed_spaces() {
  std::string result = UrlUtils::decode("hello+world%20test");
  TEST_ASSERT_EQUAL_STRING("hello world test", result.c_str());
}

void test_decode_special_characters() {
  // @ = %40, # = %23, $ = %24
  std::string result = UrlUtils::decode("user%40example.com%23test%24price");
  TEST_ASSERT_EQUAL_STRING("user@example.com#test$price", result.c_str());
}

void test_decode_slash_and_question() {
  // / = %2F, ? = %3F
  std::string result = UrlUtils::decode("path%2Fto%2Ffile%3Fquery");
  TEST_ASSERT_EQUAL_STRING("path/to/file?query", result.c_str());
}

void test_decode_lowercase_hex() {
  std::string result = UrlUtils::decode("test%2fpath%3fquery");
  TEST_ASSERT_EQUAL_STRING("test/path?query", result.c_str());
}

void test_decode_uppercase_hex() {
  std::string result = UrlUtils::decode("test%2Fpath%3Fquery");
  TEST_ASSERT_EQUAL_STRING("test/path?query", result.c_str());
}

void test_decode_invalid_percent_at_end() {
  // Incomplete %XX sequence at end
  std::string result = UrlUtils::decode("test%");
  TEST_ASSERT_EQUAL_STRING("test%", result.c_str());
}

void test_decode_invalid_percent_one_char() {
  // Only one char after %
  std::string result = UrlUtils::decode("test%2");
  TEST_ASSERT_EQUAL_STRING("test%2", result.c_str());
}

void test_decode_invalid_hex_chars() {
  // Non-hex characters after %
  std::string result = UrlUtils::decode("test%ZZ");
  TEST_ASSERT_EQUAL_STRING("test%ZZ", result.c_str());
}

void test_decode_mixed_invalid_sequences() {
  std::string result = UrlUtils::decode("valid%20test%ZZinvalid%2Fmore");
  TEST_ASSERT_EQUAL_STRING("valid test%ZZinvalid/more", result.c_str());
}

void test_decode_null_byte() {
  // %00 is null byte
  std::string result = UrlUtils::decode("test%00end");
  TEST_ASSERT_EQUAL_INT(8, result.length()); // "test" + \0 + "end"
  TEST_ASSERT_EQUAL_CHAR('\0', result[4]);
}

void test_decode_high_ascii() {
  // %FF = 255 (high ASCII)
  std::string result = UrlUtils::decode("test%FF");
  TEST_ASSERT_EQUAL_INT(5, result.length());
  TEST_ASSERT_EQUAL_INT(0xFF, (unsigned char)result[4]);
}

void test_decode_utf8_encoded() {
  // €(euro) = E2 82 AC in UTF-8
  std::string result = UrlUtils::decode("price%E2%82%AC100");
  TEST_ASSERT_EQUAL_STRING("price€100", result.c_str());
}

// ========== URL Encode Tests ==========

void test_encode_empty_string() {
  std::string result = UrlUtils::encode("");
  TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_encode_plain_alphanumeric() {
  std::string result = UrlUtils::encode("HelloWorld123");
  TEST_ASSERT_EQUAL_STRING("HelloWorld123", result.c_str());
}

void test_encode_space() {
  std::string result = UrlUtils::encode("hello world");
  TEST_ASSERT_EQUAL_STRING("hello%20world", result.c_str());
}

void test_encode_unreserved_chars() {
  // - . _ ~ are unreserved and should NOT be encoded
  std::string result = UrlUtils::encode("test-file_name.txt~backup");
  TEST_ASSERT_EQUAL_STRING("test-file_name.txt~backup", result.c_str());
}

void test_encode_special_characters() {
  std::string result = UrlUtils::encode("user@example.com");
  TEST_ASSERT_EQUAL_STRING("user%40example.com", result.c_str());
}

void test_encode_slash_and_question() {
  std::string result = UrlUtils::encode("path/to/file?query");
  TEST_ASSERT_EQUAL_STRING("path%2Fto%2Ffile%3Fquery", result.c_str());
}

void test_encode_percent_sign() {
  std::string result = UrlUtils::encode("100%");
  TEST_ASSERT_EQUAL_STRING("100%25", result.c_str());
}

void test_encode_equals_and_ampersand() {
  std::string result = UrlUtils::encode("key=value&other=test");
  TEST_ASSERT_EQUAL_STRING("key%3Dvalue%26other%3Dtest", result.c_str());
}

void test_encode_plus_sign() {
  // + should be encoded as %2B (not left as +)
  std::string result = UrlUtils::encode("1+1=2");
  TEST_ASSERT_EQUAL_STRING("1%2B1%3D2", result.c_str());
}

void test_encode_high_ascii() {
  // Test encoding of high ASCII character (0xFF)
  std::string input = "test";
  input += (char)0xFF;
  std::string result = UrlUtils::encode(input);
  TEST_ASSERT_EQUAL_STRING("test%FF", result.c_str());
}

void test_encode_null_byte() {
  std::string input = "test";
  input += '\0';
  input += "end";
  std::string result = UrlUtils::encode(input);
  TEST_ASSERT_EQUAL_STRING("test%00end", result.c_str());
}

void test_encode_utf8() {
  // € (euro) is a multi-byte UTF-8 character
  std::string result = UrlUtils::encode("price€100");
  TEST_ASSERT_EQUAL_STRING("price%E2%82%AC100", result.c_str());
}

// ========== Round-trip Tests ==========

void test_roundtrip_simple() {
  std::string original = "hello world";
  std::string encoded = UrlUtils::encode(original);
  std::string decoded = UrlUtils::decode(encoded);
  TEST_ASSERT_EQUAL_STRING(original.c_str(), decoded.c_str());
}

void test_roundtrip_complex() {
  std::string original = "user@example.com?key=value&param=100%";
  std::string encoded = UrlUtils::encode(original);
  std::string decoded = UrlUtils::decode(encoded);
  TEST_ASSERT_EQUAL_STRING(original.c_str(), decoded.c_str());
}

void test_roundtrip_utf8() {
  std::string original = "Hello 世界 €100";
  std::string encoded = UrlUtils::encode(original);
  std::string decoded = UrlUtils::decode(encoded);
  TEST_ASSERT_EQUAL_STRING(original.c_str(), decoded.c_str());
}

// ========== Helper Function Tests ==========

void test_needsEncoding_alphanumeric() {
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('A'));
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('z'));
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('0'));
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('9'));
}

void test_needsEncoding_unreserved() {
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('-'));
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('.'));
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('_'));
  TEST_ASSERT_FALSE(UrlUtils::needsEncoding('~'));
}

void test_needsEncoding_special() {
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding(' '));
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding('@'));
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding('/'));
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding('?'));
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding('='));
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding('&'));
  TEST_ASSERT_TRUE(UrlUtils::needsEncoding('%'));
}

void test_hexCharToInt_valid() {
  TEST_ASSERT_EQUAL_INT(0, UrlUtils::hexCharToInt('0'));
  TEST_ASSERT_EQUAL_INT(9, UrlUtils::hexCharToInt('9'));
  TEST_ASSERT_EQUAL_INT(10, UrlUtils::hexCharToInt('A'));
  TEST_ASSERT_EQUAL_INT(15, UrlUtils::hexCharToInt('F'));
  TEST_ASSERT_EQUAL_INT(10, UrlUtils::hexCharToInt('a'));
  TEST_ASSERT_EQUAL_INT(15, UrlUtils::hexCharToInt('f'));
}

void test_hexCharToInt_invalid() {
  TEST_ASSERT_EQUAL_INT(-1, UrlUtils::hexCharToInt('G'));
  TEST_ASSERT_EQUAL_INT(-1, UrlUtils::hexCharToInt('Z'));
  TEST_ASSERT_EQUAL_INT(-1, UrlUtils::hexCharToInt(' '));
  TEST_ASSERT_EQUAL_INT(-1, UrlUtils::hexCharToInt('@'));
}

void test_intToHexChar_valid() {
  TEST_ASSERT_EQUAL_CHAR('0', UrlUtils::intToHexChar(0));
  TEST_ASSERT_EQUAL_CHAR('9', UrlUtils::intToHexChar(9));
  TEST_ASSERT_EQUAL_CHAR('A', UrlUtils::intToHexChar(10));
  TEST_ASSERT_EQUAL_CHAR('F', UrlUtils::intToHexChar(15));
}

void test_intToHexChar_invalid() {
  TEST_ASSERT_EQUAL_CHAR('0', UrlUtils::intToHexChar(-1));
  TEST_ASSERT_EQUAL_CHAR('0', UrlUtils::intToHexChar(16));
}

// ========== Test Runner ==========

void runUrlUtilsTests() {
  // Decode tests
  RUN_TEST(test_decode_empty_string);
  RUN_TEST(test_decode_plain_text);
  RUN_TEST(test_decode_plus_to_space);
  RUN_TEST(test_decode_percent_20_space);
  RUN_TEST(test_decode_mixed_spaces);
  RUN_TEST(test_decode_special_characters);
  RUN_TEST(test_decode_slash_and_question);
  RUN_TEST(test_decode_lowercase_hex);
  RUN_TEST(test_decode_uppercase_hex);
  RUN_TEST(test_decode_invalid_percent_at_end);
  RUN_TEST(test_decode_invalid_percent_one_char);
  RUN_TEST(test_decode_invalid_hex_chars);
  RUN_TEST(test_decode_mixed_invalid_sequences);
  RUN_TEST(test_decode_null_byte);
  RUN_TEST(test_decode_high_ascii);
  RUN_TEST(test_decode_utf8_encoded);

  // Encode tests
  RUN_TEST(test_encode_empty_string);
  RUN_TEST(test_encode_plain_alphanumeric);
  RUN_TEST(test_encode_space);
  RUN_TEST(test_encode_unreserved_chars);
  RUN_TEST(test_encode_special_characters);
  RUN_TEST(test_encode_slash_and_question);
  RUN_TEST(test_encode_percent_sign);
  RUN_TEST(test_encode_equals_and_ampersand);
  RUN_TEST(test_encode_plus_sign);
  RUN_TEST(test_encode_high_ascii);
  RUN_TEST(test_encode_null_byte);
  RUN_TEST(test_encode_utf8);

  // Round-trip tests
  RUN_TEST(test_roundtrip_simple);
  RUN_TEST(test_roundtrip_complex);
  RUN_TEST(test_roundtrip_utf8);

  // Helper function tests
  RUN_TEST(test_needsEncoding_alphanumeric);
  RUN_TEST(test_needsEncoding_unreserved);
  RUN_TEST(test_needsEncoding_special);
  RUN_TEST(test_hexCharToInt_valid);
  RUN_TEST(test_hexCharToInt_invalid);
  RUN_TEST(test_intToHexChar_valid);
  RUN_TEST(test_intToHexChar_invalid);
}
