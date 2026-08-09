#ifndef AUTH_DECISION_H
#define AUTH_DECISION_H

#include <functional>
#include <interface/auth_types.h>
#include <string>

// Pure, WebPlatform-independent auth decision logic - the part of
// WebPlatform::authenticateRequest() actually worth unit testing.
//
// WebRequest/WebResponse aren't natively constructible (their real
// constructors need a live WebServerClass/httpd_req), so
// WebPlatform::authenticateRequest() (src/auth/web_platform_auth.cpp) stays
// a thin, untested adapter: pull raw strings off WebRequest, call
// evaluate() here, apply the Decision to WebResponse. Everything with
// actual branching logic - cookie/header parsing, auth-type precedence,
// which failure response to send - lives in evaluate() instead, where it's
// fully testable with plain strings and no WebPlatform/storage dependency.
namespace WebPlatformAuth {

struct DecisionInput {
  std::string clientIp;
  std::string cookieHeader;         // raw "Cookie" header value
  std::string authorizationHeader;  // raw "Authorization" header value
  std::string accessTokenParam;     // "access_token" query/form param
  std::string csrfTokenHeader;      // "X-CSRF-Token" header value
  std::string csrfTokenParam;       // "_csrf" query/form param
  std::string path;
};

enum class FailureResponse {
  None,
  Json401,       // API route, unauthenticated
  RedirectSetup, // SESSION required, no users exist yet
  RedirectLogin, // SESSION required, users exist
  Json403        // non-API route, non-SESSION auth failed
};

struct Decision {
  bool authenticated = false;
  AuthType authenticatedVia = AuthType::NONE;
  std::string sessionId;
  std::string token;
  std::string username;
  unsigned long authenticatedAt = 0;
  FailureResponse failureResponse = FailureResponse::None;
  std::string redirectUrl; // populated for RedirectLogin
};

// Callbacks so this logic never touches AuthStorage/LittleFS directly -
// production wires these to AuthStorage's static methods; tests inject
// fakes. Each lookup returns true and fills the out-params on success.
struct Dependencies {
  std::function<bool(const std::string &sessionId, std::string &username,
                     unsigned long &authenticatedAt)>
      lookupSession;
  std::function<bool(const std::string &token, std::string &username,
                     unsigned long &authenticatedAt)>
      lookupApiToken;
  std::function<bool(const std::string &csrfToken,
                     const std::string &clientIp)>
      validatePageToken;
  std::function<bool()> requiresInitialSetup;
};

Decision evaluate(const DecisionInput &input,
                  const AuthRequirements &requirements,
                  const Dependencies &deps);

} // namespace WebPlatformAuth

#endif // AUTH_DECISION_H
