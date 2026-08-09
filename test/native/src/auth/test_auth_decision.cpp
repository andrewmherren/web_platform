#include "auth/auth_decision.h"
#include <unity.h>

using WebPlatformAuth::Decision;
using WebPlatformAuth::DecisionInput;
using WebPlatformAuth::Dependencies;
using WebPlatformAuth::FailureResponse;
using WebPlatformAuth::evaluate;

namespace {

Dependencies alwaysFailDeps() {
  Dependencies deps;
  deps.lookupSession = [](const std::string &, std::string &,
                          unsigned long &) { return false; };
  deps.lookupApiToken = [](const std::string &, std::string &,
                           unsigned long &) { return false; };
  deps.validatePageToken = [](const std::string &, const std::string &) {
    return false;
  };
  deps.requiresInitialSetup = []() { return false; };
  return deps;
}

} // namespace

void test_no_auth_required_passes(void) {
  DecisionInput input;
  input.path = "/status";
  AuthRequirements requirements = {}; // empty -> requiresAuth() is false
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_TRUE(decision.authenticated);
}

void test_none_auth_type_always_passes(void) {
  DecisionInput input;
  AuthRequirements requirements = {AuthType::NONE};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_TRUE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.authenticatedVia == AuthType::NONE);
}

void test_session_success_via_cookie(void) {
  DecisionInput input;
  input.cookieHeader = "other=1; session=abc123; theme=dark";

  Dependencies deps = alwaysFailDeps();
  deps.lookupSession = [](const std::string &sessionId, std::string &username,
                          unsigned long &authenticatedAt) {
    TEST_ASSERT_EQUAL_STRING("abc123", sessionId.c_str());
    username = "alice";
    authenticatedAt = 1000;
    return true;
  };

  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, deps);

  TEST_ASSERT_TRUE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.authenticatedVia == AuthType::SESSION);
  TEST_ASSERT_EQUAL_STRING("abc123", decision.sessionId.c_str());
  TEST_ASSERT_EQUAL_STRING("alice", decision.username.c_str());
  TEST_ASSERT_EQUAL_UINT32(1000, decision.authenticatedAt);
}

void test_session_cookie_as_last_entry_no_trailing_semicolon(void) {
  DecisionInput input;
  input.cookieHeader = "theme=dark; session=lastone";

  Dependencies deps = alwaysFailDeps();
  deps.lookupSession = [](const std::string &sessionId, std::string &,
                          unsigned long &) {
    TEST_ASSERT_EQUAL_STRING("lastone", sessionId.c_str());
    return true;
  };

  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_TRUE(decision.authenticated);
}

void test_session_no_cookie_header_fails(void) {
  DecisionInput input;
  input.path = "/dashboard";
  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
}

void test_session_lookup_rejects_invalid_session(void) {
  DecisionInput input;
  input.cookieHeader = "session=expired";
  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
}

void test_token_success_via_bearer_header(void) {
  DecisionInput input;
  input.authorizationHeader = "Bearer mytoken123";

  Dependencies deps = alwaysFailDeps();
  deps.lookupApiToken = [](const std::string &token, std::string &username,
                           unsigned long &authenticatedAt) {
    TEST_ASSERT_EQUAL_STRING("mytoken123", token.c_str());
    username = "bob";
    authenticatedAt = 2000;
    return true;
  };

  AuthRequirements requirements = {AuthType::TOKEN};
  Decision decision = evaluate(input, requirements, deps);

  TEST_ASSERT_TRUE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.authenticatedVia == AuthType::TOKEN);
  TEST_ASSERT_EQUAL_STRING("mytoken123", decision.token.c_str());
  TEST_ASSERT_EQUAL_STRING("bob", decision.username.c_str());
}

void test_token_success_via_query_param_fallback(void) {
  DecisionInput input;
  input.accessTokenParam = "paramtoken";

  Dependencies deps = alwaysFailDeps();
  deps.lookupApiToken = [](const std::string &token, std::string &,
                           unsigned long &) {
    TEST_ASSERT_EQUAL_STRING("paramtoken", token.c_str());
    return true;
  };

  AuthRequirements requirements = {AuthType::TOKEN};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_TRUE(decision.authenticated);
}

void test_token_invalid_fails(void) {
  DecisionInput input;
  input.authorizationHeader = "Bearer badtoken";
  input.path = "/api/data";
  AuthRequirements requirements = {AuthType::TOKEN};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
}

void test_page_token_success_via_header(void) {
  DecisionInput input;
  input.csrfTokenHeader = "csrf_header_val";
  input.clientIp = "10.0.0.5";

  Dependencies deps = alwaysFailDeps();
  deps.validatePageToken = [](const std::string &token,
                              const std::string &clientIp) {
    TEST_ASSERT_EQUAL_STRING("csrf_header_val", token.c_str());
    TEST_ASSERT_EQUAL_STRING("10.0.0.5", clientIp.c_str());
    return true;
  };

  AuthRequirements requirements = {AuthType::PAGE_TOKEN};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_TRUE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.authenticatedVia == AuthType::PAGE_TOKEN);
}

void test_page_token_success_via_param_fallback(void) {
  DecisionInput input;
  input.csrfTokenParam = "csrf_param_val";

  Dependencies deps = alwaysFailDeps();
  deps.validatePageToken = [](const std::string &token, const std::string &) {
    TEST_ASSERT_EQUAL_STRING("csrf_param_val", token.c_str());
    return true;
  };

  AuthRequirements requirements = {AuthType::PAGE_TOKEN};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_TRUE(decision.authenticated);
}

void test_local_only_success_for_private_ip(void) {
  DecisionInput input;
  input.clientIp = "192.168.1.42";
  AuthRequirements requirements = {AuthType::LOCAL_ONLY};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_TRUE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.authenticatedVia == AuthType::LOCAL_ONLY);
}

void test_local_only_fails_for_public_ip(void) {
  DecisionInput input;
  input.clientIp = "8.8.8.8";
  AuthRequirements requirements = {AuthType::LOCAL_ONLY};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
}

void test_local_only_fails_for_invalid_ip(void) {
  DecisionInput input;
  input.clientIp = "not-an-ip";
  AuthRequirements requirements = {AuthType::LOCAL_ONLY};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
}

void test_multiple_requirements_or_semantics(void) {
  DecisionInput input;
  input.authorizationHeader = "Bearer sometoken";
  // No cookie, so SESSION fails; TOKEN should still succeed.
  Dependencies deps = alwaysFailDeps();
  deps.lookupApiToken = [](const std::string &, std::string &,
                           unsigned long &) { return true; };

  AuthRequirements requirements = {AuthType::SESSION, AuthType::TOKEN};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_TRUE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.authenticatedVia == AuthType::TOKEN);
}

void test_failure_on_api_path_returns_json_401(void) {
  DecisionInput input;
  input.path = "/api/widgets";
  AuthRequirements requirements = {AuthType::TOKEN};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.failureResponse == FailureResponse::Json401);
}

void test_failure_session_required_no_users_redirects_setup(void) {
  DecisionInput input;
  input.path = "/dashboard";
  Dependencies deps = alwaysFailDeps();
  deps.requiresInitialSetup = []() { return true; };

  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_FALSE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.failureResponse == FailureResponse::RedirectSetup);
}

void test_failure_session_required_already_on_setup_path_redirects_login(
    void) {
  DecisionInput input;
  input.path = "/setup";
  Dependencies deps = alwaysFailDeps();
  deps.requiresInitialSetup = []() { return true; };

  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, deps);
  TEST_ASSERT_FALSE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.failureResponse == FailureResponse::RedirectLogin);
  TEST_ASSERT_EQUAL_STRING("/login?redirect=/setup",
                           decision.redirectUrl.c_str());
}

void test_failure_session_required_users_exist_redirects_login(void) {
  DecisionInput input;
  input.path = "/dashboard";
  AuthRequirements requirements = {AuthType::SESSION};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.failureResponse == FailureResponse::RedirectLogin);
  TEST_ASSERT_EQUAL_STRING("/login?redirect=/dashboard",
                           decision.redirectUrl.c_str());
}

void test_failure_non_session_non_api_returns_json_403(void) {
  DecisionInput input;
  input.path = "/settings";
  AuthRequirements requirements = {AuthType::LOCAL_ONLY};
  Decision decision = evaluate(input, requirements, alwaysFailDeps());
  TEST_ASSERT_FALSE(decision.authenticated);
  TEST_ASSERT_TRUE(decision.failureResponse == FailureResponse::Json403);
}

void register_auth_decision_tests(void) {
  RUN_TEST(test_no_auth_required_passes);
  RUN_TEST(test_none_auth_type_always_passes);
  RUN_TEST(test_session_success_via_cookie);
  RUN_TEST(test_session_cookie_as_last_entry_no_trailing_semicolon);
  RUN_TEST(test_session_no_cookie_header_fails);
  RUN_TEST(test_session_lookup_rejects_invalid_session);
  RUN_TEST(test_token_success_via_bearer_header);
  RUN_TEST(test_token_success_via_query_param_fallback);
  RUN_TEST(test_token_invalid_fails);
  RUN_TEST(test_page_token_success_via_header);
  RUN_TEST(test_page_token_success_via_param_fallback);
  RUN_TEST(test_local_only_success_for_private_ip);
  RUN_TEST(test_local_only_fails_for_public_ip);
  RUN_TEST(test_local_only_fails_for_invalid_ip);
  RUN_TEST(test_multiple_requirements_or_semantics);
  RUN_TEST(test_failure_on_api_path_returns_json_401);
  RUN_TEST(test_failure_session_required_no_users_redirects_setup);
  RUN_TEST(test_failure_session_required_already_on_setup_path_redirects_login);
  RUN_TEST(test_failure_session_required_users_exist_redirects_login);
  RUN_TEST(test_failure_non_session_non_api_returns_json_403);
}
