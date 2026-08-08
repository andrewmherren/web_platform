#include "auth/auth_utils.h"
#include <unity.h>

// ===========================================================================
// generateSecureToken / generatePageToken
// ===========================================================================

void test_generateSecureToken_default_length() {
  String token = AuthUtils::generateSecureToken();
  TEST_ASSERT_EQUAL(32, token.length());
}

void test_generateSecureToken_custom_length() {
  String token = AuthUtils::generateSecureToken(10);
  TEST_ASSERT_EQUAL(10, token.length());
}

void test_generateSecureToken_uses_expected_charset() {
  String token = AuthUtils::generateSecureToken(200);
  for (unsigned int i = 0; i < token.length(); i++) {
    char c = token[i];
    bool isAlnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z');
    TEST_ASSERT_TRUE_MESSAGE(isAlnum, "Token character outside expected charset");
  }
}

void test_generateSecureToken_is_not_trivially_constant() {
  // Not a strict randomness test - just guards against a hardcoded/static
  // return value slipping in.
  String a = AuthUtils::generateSecureToken(32);
  String b = AuthUtils::generateSecureToken(32);
  TEST_ASSERT_FALSE(a.equals(b));
}

void test_generatePageToken_has_csrf_prefix() {
  String token = AuthUtils::generatePageToken();
  TEST_ASSERT_TRUE(token.startsWith("csrf_"));
  // "csrf_" (5 chars) + 24-char secure token
  TEST_ASSERT_EQUAL(29, token.length());
}

// ===========================================================================
// hashPassword / verifyPassword
// ===========================================================================

void test_hashPassword_returns_nonempty_hash() {
  String hash = AuthUtils::hashPassword("correct horse", "somesalt");
  TEST_ASSERT_TRUE(hash.length() > 0);
}

void test_hashPassword_is_deterministic_for_same_input() {
  String hash1 = AuthUtils::hashPassword("mypassword", "fixedsalt", 100);
  String hash2 = AuthUtils::hashPassword("mypassword", "fixedsalt", 100);
  TEST_ASSERT_TRUE(hash1.equals(hash2));
}

void test_hashPassword_differs_with_different_salt() {
  String hash1 = AuthUtils::hashPassword("mypassword", "salt1", 100);
  String hash2 = AuthUtils::hashPassword("mypassword", "salt2", 100);
  TEST_ASSERT_FALSE(hash1.equals(hash2));
}

void test_hashPassword_differs_with_different_password() {
  String hash1 = AuthUtils::hashPassword("password1", "samesalt", 100);
  String hash2 = AuthUtils::hashPassword("password2", "samesalt", 100);
  TEST_ASSERT_FALSE(hash1.equals(hash2));
}

void test_verifyPassword_succeeds_for_correct_password() {
  String salt = AuthUtils::generateSalt();
  String hash = AuthUtils::hashPassword("correct-password", salt);
  TEST_ASSERT_TRUE(AuthUtils::verifyPassword("correct-password", hash, salt));
}

void test_verifyPassword_fails_for_wrong_password() {
  String salt = AuthUtils::generateSalt();
  String hash = AuthUtils::hashPassword("correct-password", salt);
  TEST_ASSERT_FALSE(AuthUtils::verifyPassword("wrong-password", hash, salt));
}

void test_verifyPassword_fails_for_wrong_salt() {
  String hash = AuthUtils::hashPassword("correct-password", "salt-a");
  TEST_ASSERT_FALSE(AuthUtils::verifyPassword("correct-password", hash, "salt-b"));
}

void test_verifyPassword_is_case_insensitive_on_hash() {
  // verifyPassword uses equalsIgnoreCase on the hash - hex case shouldn't
  // matter, only content.
  String salt = "fixedsalt";
  String hash = AuthUtils::hashPassword("mypassword", salt);
  String lowerHash = hash;
  lowerHash.toLowerCase();
  TEST_ASSERT_TRUE(AuthUtils::verifyPassword("mypassword", lowerHash, salt));
}

// ===========================================================================
// generateSalt / generateUserId / bytesToHex
// ===========================================================================

void test_generateSalt_default_length_produces_32_hex_chars() {
  // 16 bytes -> 32 hex characters
  String salt = AuthUtils::generateSalt();
  TEST_ASSERT_EQUAL(32, salt.length());
}

void test_generateSalt_custom_length() {
  String salt = AuthUtils::generateSalt(8);
  TEST_ASSERT_EQUAL(16, salt.length()); // 8 bytes -> 16 hex chars
}

void test_generateUserId_matches_uuid_v4_format() {
  String id = AuthUtils::generateUserId();
  // xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  TEST_ASSERT_EQUAL(36, id.length());
  TEST_ASSERT_EQUAL('-', id[8]);
  TEST_ASSERT_EQUAL('-', id[13]);
  TEST_ASSERT_EQUAL('-', id[18]);
  TEST_ASSERT_EQUAL('-', id[23]);
  TEST_ASSERT_EQUAL('4', id[14]); // version nibble

  char variantChar = id[19];
  bool validVariant = (variantChar == '8' || variantChar == '9' ||
                       variantChar == 'a' || variantChar == 'b');
  TEST_ASSERT_TRUE_MESSAGE(validVariant, "UUID variant nibble out of range");
}

void test_generateUserId_is_lowercase() {
  String id = AuthUtils::generateUserId();
  String upper = id;
  upper.toUpperCase();
  // If it were already all-lowercase, it should differ from the uppercased
  // version wherever there's a letter present (UUIDs always contain hex
  // letters with overwhelming probability given 32 hex digits).
  TEST_ASSERT_FALSE(id.equals(upper));
}

void test_bytesToHex_encodes_correctly() {
  uint8_t bytes[] = {0x00, 0x0F, 0xFF, 0xA5};
  String hex = AuthUtils::bytesToHex(bytes, 4);
  TEST_ASSERT_EQUAL_STRING("000FFFA5", hex.c_str());
}

// ===========================================================================
// parseIPAddress
// ===========================================================================

void test_parseIPAddress_valid_address() {
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("192.168.1.42");
  TEST_ASSERT_TRUE(ip.isValid());
  TEST_ASSERT_EQUAL(192, ip.bytes[0]);
  TEST_ASSERT_EQUAL(168, ip.bytes[1]);
  TEST_ASSERT_EQUAL(1, ip.bytes[2]);
  TEST_ASSERT_EQUAL(42, ip.bytes[3]);
}

void test_parseIPAddress_empty_string_is_invalid() {
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("");
  TEST_ASSERT_FALSE(ip.isValid());
}

void test_parseIPAddress_too_many_octets_is_invalid() {
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("1.2.3.4.5");
  TEST_ASSERT_FALSE(ip.isValid());
}

void test_parseIPAddress_too_few_octets_is_invalid() {
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("1.2.3");
  TEST_ASSERT_FALSE(ip.isValid());
}

void test_parseIPAddress_octet_out_of_range_is_invalid() {
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("1.2.3.256");
  TEST_ASSERT_FALSE(ip.isValid());
}

void test_parseIPAddress_non_numeric_octet_is_invalid() {
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("1.2.3.abc");
  TEST_ASSERT_FALSE(ip.isValid());
}

void test_parseIPAddress_all_zeros_parses_but_reads_as_invalid() {
  // Documents existing behavior: 0.0.0.0 parses successfully (dotCount==3,
  // octetIndex==4) but happens to equal the all-zero sentinel IPAddress()
  // returned on parse failure, so isValid() is false either way. Not a bug
  // fix - just pinning down the current, slightly surprising behavior.
  AuthUtils::IPAddress ip = AuthUtils::parseIPAddress("0.0.0.0");
  TEST_ASSERT_FALSE(ip.isValid());
}

// ===========================================================================
// isIPInSubnet
// ===========================================================================

void test_isIPInSubnet_matches_within_range() {
  AuthUtils::IPAddress ip(192, 168, 1, 100);
  AuthUtils::Subnet subnet(192, 168, 1, 0, 24);
  TEST_ASSERT_TRUE(AuthUtils::isIPInSubnet(ip, subnet));
}

void test_isIPInSubnet_rejects_outside_range() {
  AuthUtils::IPAddress ip(192, 168, 2, 100);
  AuthUtils::Subnet subnet(192, 168, 1, 0, 24);
  TEST_ASSERT_FALSE(AuthUtils::isIPInSubnet(ip, subnet));
}

void test_isIPInSubnet_rejects_invalid_prefix_length() {
  AuthUtils::IPAddress ip(192, 168, 1, 100);
  AuthUtils::Subnet subnet(192, 168, 1, 0, 0); // prefixLength 0 is rejected
  TEST_ASSERT_FALSE(AuthUtils::isIPInSubnet(ip, subnet));
}

void test_isIPInSubnet_rejects_invalid_ip() {
  AuthUtils::IPAddress invalidIp; // all zeros -> !isValid()
  AuthUtils::Subnet subnet(192, 168, 1, 0, 24);
  TEST_ASSERT_FALSE(AuthUtils::isIPInSubnet(invalidIp, subnet));
}

// ===========================================================================
// isLocalNetworkIP / isLoopbackIP
// ===========================================================================

void test_isLocalNetworkIP_class_A_private_range() {
  AuthUtils::IPAddress ip(10, 1, 2, 3);
  TEST_ASSERT_TRUE(AuthUtils::isLocalNetworkIP(ip));
}

void test_isLocalNetworkIP_class_B_private_range() {
  AuthUtils::IPAddress inRange(172, 20, 0, 1);
  AuthUtils::IPAddress belowRange(172, 15, 0, 1);
  AuthUtils::IPAddress aboveRange(172, 32, 0, 1);
  TEST_ASSERT_TRUE(AuthUtils::isLocalNetworkIP(inRange));
  TEST_ASSERT_FALSE(AuthUtils::isLocalNetworkIP(belowRange));
  TEST_ASSERT_FALSE(AuthUtils::isLocalNetworkIP(aboveRange));
}

void test_isLocalNetworkIP_class_C_private_range() {
  AuthUtils::IPAddress ip(192, 168, 50, 1);
  TEST_ASSERT_TRUE(AuthUtils::isLocalNetworkIP(ip));
}

void test_isLocalNetworkIP_link_local_range() {
  AuthUtils::IPAddress ip(169, 254, 1, 1);
  TEST_ASSERT_TRUE(AuthUtils::isLocalNetworkIP(ip));
}

void test_isLocalNetworkIP_rejects_public_ip() {
  AuthUtils::IPAddress ip(8, 8, 8, 8);
  TEST_ASSERT_FALSE(AuthUtils::isLocalNetworkIP(ip));
}

void test_isLocalNetworkIP_rejects_invalid_ip() {
  AuthUtils::IPAddress invalidIp;
  TEST_ASSERT_FALSE(AuthUtils::isLocalNetworkIP(invalidIp));
}

void test_isLocalNetworkIP_includes_loopback() {
  AuthUtils::IPAddress ip(127, 0, 0, 1);
  TEST_ASSERT_TRUE(AuthUtils::isLocalNetworkIP(ip));
}

void test_isLoopbackIP_matches_127_range() {
  AuthUtils::IPAddress ip(127, 5, 6, 7);
  TEST_ASSERT_TRUE(AuthUtils::isLoopbackIP(ip));
}

void test_isLoopbackIP_rejects_non_loopback() {
  AuthUtils::IPAddress ip(192, 168, 1, 1);
  TEST_ASSERT_FALSE(AuthUtils::isLoopbackIP(ip));
}

void test_isLoopbackIP_rejects_invalid_ip() {
  AuthUtils::IPAddress invalidIp;
  TEST_ASSERT_FALSE(AuthUtils::isLoopbackIP(invalidIp));
}

// ===========================================================================
// Test runner
// ===========================================================================

void runAuthUtilsTests() {
  RUN_TEST(test_generateSecureToken_default_length);
  RUN_TEST(test_generateSecureToken_custom_length);
  RUN_TEST(test_generateSecureToken_uses_expected_charset);
  RUN_TEST(test_generateSecureToken_is_not_trivially_constant);
  RUN_TEST(test_generatePageToken_has_csrf_prefix);

  RUN_TEST(test_hashPassword_returns_nonempty_hash);
  RUN_TEST(test_hashPassword_is_deterministic_for_same_input);
  RUN_TEST(test_hashPassword_differs_with_different_salt);
  RUN_TEST(test_hashPassword_differs_with_different_password);
  RUN_TEST(test_verifyPassword_succeeds_for_correct_password);
  RUN_TEST(test_verifyPassword_fails_for_wrong_password);
  RUN_TEST(test_verifyPassword_fails_for_wrong_salt);
  RUN_TEST(test_verifyPassword_is_case_insensitive_on_hash);

  RUN_TEST(test_generateSalt_default_length_produces_32_hex_chars);
  RUN_TEST(test_generateSalt_custom_length);
  RUN_TEST(test_generateUserId_matches_uuid_v4_format);
  RUN_TEST(test_generateUserId_is_lowercase);
  RUN_TEST(test_bytesToHex_encodes_correctly);

  RUN_TEST(test_parseIPAddress_valid_address);
  RUN_TEST(test_parseIPAddress_empty_string_is_invalid);
  RUN_TEST(test_parseIPAddress_too_many_octets_is_invalid);
  RUN_TEST(test_parseIPAddress_too_few_octets_is_invalid);
  RUN_TEST(test_parseIPAddress_octet_out_of_range_is_invalid);
  RUN_TEST(test_parseIPAddress_non_numeric_octet_is_invalid);
  RUN_TEST(test_parseIPAddress_all_zeros_parses_but_reads_as_invalid);

  RUN_TEST(test_isIPInSubnet_matches_within_range);
  RUN_TEST(test_isIPInSubnet_rejects_outside_range);
  RUN_TEST(test_isIPInSubnet_rejects_invalid_prefix_length);
  RUN_TEST(test_isIPInSubnet_rejects_invalid_ip);

  RUN_TEST(test_isLocalNetworkIP_class_A_private_range);
  RUN_TEST(test_isLocalNetworkIP_class_B_private_range);
  RUN_TEST(test_isLocalNetworkIP_class_C_private_range);
  RUN_TEST(test_isLocalNetworkIP_link_local_range);
  RUN_TEST(test_isLocalNetworkIP_rejects_public_ip);
  RUN_TEST(test_isLocalNetworkIP_rejects_invalid_ip);
  RUN_TEST(test_isLocalNetworkIP_includes_loopback);
  RUN_TEST(test_isLoopbackIP_matches_127_range);
  RUN_TEST(test_isLoopbackIP_rejects_non_loopback);
  RUN_TEST(test_isLoopbackIP_rejects_invalid_ip);
}
