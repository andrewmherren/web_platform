#include "models/data_models.h"
#include "storage/auth_storage.h"
#include "storage/storage_manager.h"
#include <unity.h>

namespace {

// AuthStorage's collection names are private constants (USERS_COLLECTION
// etc. in auth_storage.h) - mirrored here so tests can inject records
// directly into storage (e.g. an already-expired session) without a public
// API for it. Keep in sync with auth_storage.cpp if those ever change.
const char *kSessions = "sessions";
const char *kApiTokens = "api_tokens";
const char *kPageTokens = "page_tokens";

// AuthStorage::initialize() is a permanent one-shot per process (no reset
// hook - see PROJECT_PLAN.md). Tests rely on the *driver's* data being wiped
// every test instead: the global Unity setUp() calls
// StorageManager::clearAllDrivers(), so AuthStorage's default ("") driver
// resolves to a fresh JsonDatabaseDriver (backed by the also-reset
// Preferences fake) each time, even though AuthStorage's own initialized/
// driverName statics are set once and never touched again.
IDatabaseDriver &rawDriver() { return StorageManager::driver(""); }

} // namespace

// --- User management ---

void test_create_user_rejects_empty_fields(void) {
  TEST_ASSERT_EQUAL_STRING("", AuthStorage::createUser("", "pw").c_str());
  TEST_ASSERT_EQUAL_STRING("", AuthStorage::createUser("alice", "").c_str());
}

void test_create_user_succeeds_and_normalizes_username(void) {
  String id = AuthStorage::createUser("Alice", "pw12345");
  TEST_ASSERT_TRUE(id.length() > 0);

  AuthUser user = AuthStorage::findUserById(id);
  TEST_ASSERT_EQUAL_STRING("alice", user.username.c_str());
}

void test_create_user_rejects_case_insensitive_duplicate(void) {
  TEST_ASSERT_TRUE(AuthStorage::createUser("alice", "pw12345").length() > 0);
  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::createUser("ALICE", "different").c_str());
}

void test_first_user_becomes_admin(void) {
  String id = AuthStorage::createUser("alice", "pw12345");
  AuthUser user = AuthStorage::findUserById(id);
  TEST_ASSERT_TRUE(user.isAdmin);
}

void test_second_user_is_not_admin(void) {
  AuthStorage::createUser("alice", "pw12345");
  String id2 = AuthStorage::createUser("bob", "pw12345");
  AuthUser user2 = AuthStorage::findUserById(id2);
  TEST_ASSERT_FALSE(user2.isAdmin);
}

void test_find_user_by_id_unknown_returns_invalid(void) {
  AuthUser user = AuthStorage::findUserById("does-not-exist");
  TEST_ASSERT_FALSE(user.isValid());
}

void test_find_user_by_username_is_case_insensitive(void) {
  AuthStorage::createUser("Alice", "pw12345");
  AuthUser user = AuthStorage::findUserByUsername("aLiCe");
  TEST_ASSERT_TRUE(user.isValid());
  TEST_ASSERT_EQUAL_STRING("alice", user.username.c_str());
}

void test_update_user_password_changes_credentials(void) {
  String id = AuthStorage::createUser("alice", "oldpassword");
  TEST_ASSERT_TRUE(AuthStorage::updateUserPassword(id, "newpassword"));

  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::validateCredentials("alice", "oldpassword").c_str());
  TEST_ASSERT_EQUAL_STRING(
      id.c_str(),
      AuthStorage::validateCredentials("alice", "newpassword").c_str());
}

void test_update_user_password_fails_for_unknown_user(void) {
  TEST_ASSERT_FALSE(
      AuthStorage::updateUserPassword("does-not-exist", "newpassword"));
}

void test_delete_user_removes_user_and_cascades(void) {
  String id = AuthStorage::createUser("alice", "pw12345");
  String sessionId = AuthStorage::createSession(id);
  String token = AuthStorage::createApiToken(id, "cli");
  TEST_ASSERT_TRUE(sessionId.length() > 0);
  TEST_ASSERT_TRUE(token.length() > 0);

  TEST_ASSERT_TRUE(AuthStorage::deleteUser(id));

  TEST_ASSERT_FALSE(AuthStorage::findUserById(id).isValid());
  TEST_ASSERT_FALSE(AuthStorage::findSession(sessionId).isValid());
  TEST_ASSERT_EQUAL_STRING("", AuthStorage::validateApiToken(token).c_str());
}

void test_validate_credentials_succeeds_for_correct_password(void) {
  String id = AuthStorage::createUser("alice", "pw12345");
  TEST_ASSERT_EQUAL_STRING(
      id.c_str(),
      AuthStorage::validateCredentials("alice", "pw12345").c_str());
}

void test_validate_credentials_fails_for_wrong_password(void) {
  AuthStorage::createUser("alice", "pw12345");
  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::validateCredentials("alice", "wrong").c_str());
}

void test_validate_credentials_fails_for_unknown_user(void) {
  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::validateCredentials("nobody", "pw12345").c_str());
}

void test_get_all_users_returns_every_created_user(void) {
  AuthStorage::createUser("alice", "pw12345");
  AuthStorage::createUser("bob", "pw12345");
  std::vector<AuthUser> users = AuthStorage::getAllUsers();
  TEST_ASSERT_EQUAL(2, users.size());
}

// --- Session management ---

void test_create_session_fails_for_unknown_user(void) {
  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::createSession("does-not-exist").c_str());
}

void test_create_session_succeeds_with_prefix(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String sessionId = AuthStorage::createSession(userId);
  TEST_ASSERT_TRUE(sessionId.startsWith("sess_"));
}

void test_find_session_unknown_returns_invalid(void) {
  TEST_ASSERT_FALSE(AuthStorage::findSession("nope").isValid());
}

void test_validate_session_succeeds_and_returns_user_id(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String sessionId = AuthStorage::createSession(userId);
  TEST_ASSERT_EQUAL_STRING(userId.c_str(),
                           AuthStorage::validateSession(sessionId).c_str());
}

void test_validate_session_rejects_and_cleans_up_expired(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  AuthUser user = AuthStorage::findUserById(userId);

  AuthSession expired("sess_expired", userId, user.username);
  expired.expiresAt = 1; // long in the past
  rawDriver().store(kSessions, expired.id, expired.toJson());

  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::validateSession("sess_expired").c_str());
  // validateSession() should have deleted it as a side effect.
  TEST_ASSERT_FALSE(AuthStorage::findSession("sess_expired").isValid());
}

void test_delete_session_removes_it(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String sessionId = AuthStorage::createSession(userId);
  TEST_ASSERT_TRUE(AuthStorage::deleteSession(sessionId));
  TEST_ASSERT_FALSE(AuthStorage::findSession(sessionId).isValid());
}

void test_clean_expired_sessions_removes_only_expired(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String validSessionId = AuthStorage::createSession(userId);

  AuthUser user = AuthStorage::findUserById(userId);
  AuthSession expired("sess_expired2", userId, user.username);
  expired.expiresAt = 1;
  rawDriver().store(kSessions, expired.id, expired.toJson());

  int cleaned = AuthStorage::cleanExpiredSessions();
  TEST_ASSERT_EQUAL(1, cleaned);
  TEST_ASSERT_TRUE(AuthStorage::findSession(validSessionId).isValid());
  TEST_ASSERT_FALSE(AuthStorage::findSession("sess_expired2").isValid());
}

// --- API token management ---

void test_create_api_token_fails_for_unknown_user(void) {
  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::createApiToken("does-not-exist", "cli").c_str());
}

void test_create_api_token_succeeds_with_prefix(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String token = AuthStorage::createApiToken(userId, "cli");
  TEST_ASSERT_TRUE(token.startsWith("tok_"));
}

void test_validate_api_token_roundtrip(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String token = AuthStorage::createApiToken(userId, "cli");
  TEST_ASSERT_EQUAL_STRING(userId.c_str(),
                           AuthStorage::validateApiToken(token).c_str());
}

void test_validate_api_token_rejects_and_cleans_up_expired(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  AuthUser user = AuthStorage::findUserById(userId);

  AuthApiToken expired("tok_expired", userId, user.username, "cli", 1);
  expired.expiresAt = 1; // already in the past
  rawDriver().store(kApiTokens, expired.id, expired.toJson());

  TEST_ASSERT_EQUAL_STRING(
      "", AuthStorage::validateApiToken("tok_expired").c_str());
  TEST_ASSERT_FALSE(AuthStorage::findApiToken("tok_expired").isValid());
}

void test_delete_api_token_removes_it(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String token = AuthStorage::createApiToken(userId, "cli");
  TEST_ASSERT_TRUE(AuthStorage::deleteApiToken(token));
  TEST_ASSERT_EQUAL_STRING("", AuthStorage::validateApiToken(token).c_str());
}

void test_get_user_api_tokens_scoped_to_user(void) {
  String userId1 = AuthStorage::createUser("alice", "pw12345");
  String userId2 = AuthStorage::createUser("bob", "pw12345");
  AuthStorage::createApiToken(userId1, "cli-1");
  AuthStorage::createApiToken(userId1, "cli-2");
  AuthStorage::createApiToken(userId2, "cli-3");

  std::vector<AuthApiToken> tokens = AuthStorage::getUserApiTokens(userId1);
  TEST_ASSERT_EQUAL(2, tokens.size());
}

void test_clean_expired_api_tokens_removes_only_expired(void) {
  String userId = AuthStorage::createUser("alice", "pw12345");
  String validToken = AuthStorage::createApiToken(userId, "cli");

  AuthUser user = AuthStorage::findUserById(userId);
  AuthApiToken expired("tok_expired2", userId, user.username, "cli", 1);
  expired.expiresAt = 1;
  rawDriver().store(kApiTokens, expired.id, expired.toJson());

  int cleaned = AuthStorage::cleanExpiredApiTokens();
  TEST_ASSERT_EQUAL(1, cleaned);
  TEST_ASSERT_TRUE(AuthStorage::findApiToken(validToken).isValid());
}

// --- Page token management (CSRF) ---

void test_create_and_validate_page_token_same_ip_succeeds(void) {
  String token = AuthStorage::createPageToken("10.0.0.5");
  TEST_ASSERT_TRUE(token.length() > 0);
  TEST_ASSERT_TRUE(AuthStorage::validatePageToken(token, "10.0.0.5"));
}

void test_validate_page_token_rejects_ip_mismatch(void) {
  String token = AuthStorage::createPageToken("10.0.0.5");
  TEST_ASSERT_FALSE(AuthStorage::validatePageToken(token, "10.0.0.6"));
}

void test_validate_page_token_rejects_unknown_token(void) {
  TEST_ASSERT_FALSE(AuthStorage::validatePageToken("nope", "10.0.0.5"));
}

void test_validate_page_token_rejects_and_cleans_up_expired(void) {
  AuthPageToken expired("csrf_expired", "10.0.0.5");
  expired.expiresAt = 1;
  rawDriver().store(kPageTokens, expired.id, expired.toJson());

  TEST_ASSERT_FALSE(
      AuthStorage::validatePageToken("csrf_expired", "10.0.0.5"));
  // Confirm it's actually gone, not just reported invalid this one time.
  TEST_ASSERT_EQUAL_STRING("", rawDriver().retrieve(kPageTokens, expired.id).c_str());
}

void test_clean_expired_page_tokens_removes_only_expired(void) {
  String valid = AuthStorage::createPageToken("10.0.0.5");

  AuthPageToken expired("csrf_expired2", "10.0.0.5");
  expired.expiresAt = 1;
  rawDriver().store(kPageTokens, expired.id, expired.toJson());

  int cleaned = AuthStorage::cleanExpiredPageTokens();
  TEST_ASSERT_EQUAL(1, cleaned);
  TEST_ASSERT_TRUE(AuthStorage::validatePageToken(valid, "10.0.0.5"));
}

// --- Setup state ---

void test_requires_initial_setup_true_when_no_users(void) {
  TEST_ASSERT_TRUE(AuthStorage::requiresInitialSetup());
  TEST_ASSERT_FALSE(AuthStorage::hasUsers());
}

void test_requires_initial_setup_false_once_user_exists(void) {
  AuthStorage::createUser("alice", "pw12345");
  TEST_ASSERT_FALSE(AuthStorage::requiresInitialSetup());
  TEST_ASSERT_TRUE(AuthStorage::hasUsers());
}

void register_auth_storage_tests(void) {
  RUN_TEST(test_create_user_rejects_empty_fields);
  RUN_TEST(test_create_user_succeeds_and_normalizes_username);
  RUN_TEST(test_create_user_rejects_case_insensitive_duplicate);
  RUN_TEST(test_first_user_becomes_admin);
  RUN_TEST(test_second_user_is_not_admin);
  RUN_TEST(test_find_user_by_id_unknown_returns_invalid);
  RUN_TEST(test_find_user_by_username_is_case_insensitive);
  RUN_TEST(test_update_user_password_changes_credentials);
  RUN_TEST(test_update_user_password_fails_for_unknown_user);
  RUN_TEST(test_delete_user_removes_user_and_cascades);
  RUN_TEST(test_validate_credentials_succeeds_for_correct_password);
  RUN_TEST(test_validate_credentials_fails_for_wrong_password);
  RUN_TEST(test_validate_credentials_fails_for_unknown_user);
  RUN_TEST(test_get_all_users_returns_every_created_user);

  RUN_TEST(test_create_session_fails_for_unknown_user);
  RUN_TEST(test_create_session_succeeds_with_prefix);
  RUN_TEST(test_find_session_unknown_returns_invalid);
  RUN_TEST(test_validate_session_succeeds_and_returns_user_id);
  RUN_TEST(test_validate_session_rejects_and_cleans_up_expired);
  RUN_TEST(test_delete_session_removes_it);
  RUN_TEST(test_clean_expired_sessions_removes_only_expired);

  RUN_TEST(test_create_api_token_fails_for_unknown_user);
  RUN_TEST(test_create_api_token_succeeds_with_prefix);
  RUN_TEST(test_validate_api_token_roundtrip);
  RUN_TEST(test_validate_api_token_rejects_and_cleans_up_expired);
  RUN_TEST(test_delete_api_token_removes_it);
  RUN_TEST(test_get_user_api_tokens_scoped_to_user);
  RUN_TEST(test_clean_expired_api_tokens_removes_only_expired);

  RUN_TEST(test_create_and_validate_page_token_same_ip_succeeds);
  RUN_TEST(test_validate_page_token_rejects_ip_mismatch);
  RUN_TEST(test_validate_page_token_rejects_unknown_token);
  RUN_TEST(test_validate_page_token_rejects_and_cleans_up_expired);
  RUN_TEST(test_clean_expired_page_tokens_removes_only_expired);

  RUN_TEST(test_requires_initial_setup_true_when_no_users);
  RUN_TEST(test_requires_initial_setup_false_once_user_exists);
}
