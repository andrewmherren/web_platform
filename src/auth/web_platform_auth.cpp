#include "auth/auth_decision.h"
#include "auth/auth_utils.h"
#include "storage/auth_storage.h"
#include "web_platform.h"
#include <functional>
#include <interface/auth_types.h>
#include <testing/arduino_string_compat.h>

// Initialize authentication system
void WebPlatform::initializeAuth() {
  // Initialize the auth storage
  AuthStorage::initialize();

  // Register authentication routes
  registerAuthRoutes();
}

// Check if a request is authenticated according to the route's requirements.
// Thin adapter: pulls raw strings off WebRequest, delegates the actual
// decision logic (cookie/header parsing, auth-type precedence, failure
// response selection) to WebPlatformAuth::evaluate() in auth_decision.cpp,
// which is unit-tested directly - WebRequest/WebResponse can't be
// constructed outside a live WebServerClass/httpd_req, so this adapter
// itself isn't (see auth_decision.h for the full rationale).
bool WebPlatform::authenticateRequest(WebRequest &req, WebResponse &res,
                                      const AuthRequirements &requirements) {
  WebPlatformAuth::DecisionInput input;
  input.clientIp = req.getClientIp().c_str();
  input.cookieHeader = req.getHeader("Cookie").c_str();
  input.authorizationHeader = req.getHeader("Authorization").c_str();
  input.accessTokenParam = req.getParam("access_token").c_str();
  input.csrfTokenHeader = req.getHeader("X-CSRF-Token").c_str();
  input.csrfTokenParam = req.getParam("_csrf").c_str();
  input.path = req.getPath().c_str();

  WebPlatformAuth::Dependencies deps;
  deps.lookupSession = [](const std::string &sessionId, std::string &username,
                          unsigned long &authenticatedAt) -> bool {
    String sid(sessionId.c_str());
    if (AuthStorage::validateSession(sid)) {
      AuthSession session = AuthStorage::findSession(sid);
      if (session.isValid()) {
        username = session.username.c_str();
        authenticatedAt = session.createdAt;
        return true;
      }
    }
    return false;
  };
  deps.lookupApiToken = [](const std::string &token, std::string &username,
                           unsigned long &authenticatedAt) -> bool {
    String t(token.c_str());
    if (AuthStorage::validateApiToken(t)) {
      AuthApiToken apiToken = AuthStorage::findApiToken(t);
      if (apiToken.isValid()) {
        username = apiToken.username.c_str();
        authenticatedAt = apiToken.createdAt;
        return true;
      }
    }
    return false;
  };
  deps.validatePageToken = [](const std::string &csrfToken,
                              const std::string &clientIp) -> bool {
    return AuthStorage::validatePageToken(String(csrfToken.c_str()),
                                          String(clientIp.c_str()));
  };
  deps.requiresInitialSetup = []() -> bool {
    return AuthStorage::requiresInitialSetup();
  };

  WebPlatformAuth::Decision decision =
      WebPlatformAuth::evaluate(input, requirements, deps);

  AuthContext authContext;
  authContext.clear();
  authContext.isAuthenticated = decision.authenticated;
  authContext.authenticatedVia = decision.authenticatedVia;
  authContext.sessionId = decision.sessionId.c_str();
  authContext.username = decision.username.c_str();
  authContext.authenticatedAt = decision.authenticatedAt;
  if (decision.authenticatedVia == AuthType::TOKEN) {
    authContext.token = decision.token.c_str();
  }
  req.setAuthContext(authContext);

  if (!decision.authenticated) {
    switch (decision.failureResponse) {
    case WebPlatformAuth::FailureResponse::Json401:
      res.setStatus(401);
      res.setHeader("Content-Type", "application/json");
      res.setContent("{\"error\":\"unauthorized\",\"message\":\"Authentication "
                     "required\",\"code\":401}");
      break;
    case WebPlatformAuth::FailureResponse::RedirectSetup:
      res.redirect("/setup");
      break;
    case WebPlatformAuth::FailureResponse::RedirectLogin:
      res.redirect(String(decision.redirectUrl.c_str()));
      break;
    case WebPlatformAuth::FailureResponse::Json403:
    default:
      res.setStatus(403);
      res.setHeader("Content-Type", "application/json");
      res.setContent("{\"error\":\"forbidden\",\"message\":\"Access "
                     "denied\",\"code\":403}");
      break;
    }
    return false;
  }

  return true;
}