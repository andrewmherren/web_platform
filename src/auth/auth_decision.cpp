#include "auth/auth_decision.h"
#include "auth/auth_utils.h"

namespace WebPlatformAuth {

namespace {
bool startsWith(const std::string &s, const std::string &prefix) {
  return s.compare(0, prefix.size(), prefix) == 0;
}
} // namespace

Decision evaluate(const DecisionInput &input,
                  const AuthRequirements &requirements,
                  const Dependencies &deps) {
  Decision decision;

  if (!AuthUtils::requiresAuth(requirements)) {
    decision.authenticated = true;
    return decision;
  }

  for (AuthType authType : requirements) {
    if (authType == AuthType::NONE) {
      decision.authenticated = true;
      decision.authenticatedVia = AuthType::NONE;
    } else if (authType == AuthType::SESSION) {
      const std::string marker = "session=";
      size_t start = input.cookieHeader.find(marker);
      if (start != std::string::npos) {
        start += marker.size();
        size_t end = input.cookieHeader.find(';', start);
        std::string sessionId =
            (end == std::string::npos)
                ? input.cookieHeader.substr(start)
                : input.cookieHeader.substr(start, end - start);

        std::string username;
        unsigned long authenticatedAt = 0;
        if (deps.lookupSession &&
            deps.lookupSession(sessionId, username, authenticatedAt)) {
          decision.authenticated = true;
          decision.authenticatedVia = AuthType::SESSION;
          decision.sessionId = sessionId;
          decision.username = username;
          decision.authenticatedAt = authenticatedAt;
        }
      }
    } else if (authType == AuthType::TOKEN) {
      std::string token;
      const std::string bearerPrefix = "Bearer ";
      if (startsWith(input.authorizationHeader, bearerPrefix)) {
        token = input.authorizationHeader.substr(bearerPrefix.size());
      } else {
        token = input.accessTokenParam;
      }

      if (!token.empty() && deps.lookupApiToken) {
        std::string username;
        unsigned long authenticatedAt = 0;
        if (deps.lookupApiToken(token, username, authenticatedAt)) {
          decision.authenticated = true;
          decision.authenticatedVia = AuthType::TOKEN;
          decision.token = token;
          decision.username = username;
          decision.authenticatedAt = authenticatedAt;
        }
      }
    } else if (authType == AuthType::PAGE_TOKEN) {
      std::string csrfToken = !input.csrfTokenHeader.empty()
                                  ? input.csrfTokenHeader
                                  : input.csrfTokenParam;

      if (!csrfToken.empty() && deps.validatePageToken &&
          deps.validatePageToken(csrfToken, input.clientIp)) {
        decision.authenticated = true;
        decision.authenticatedVia = AuthType::PAGE_TOKEN;
      }
    } else if (authType == AuthType::LOCAL_ONLY) {
      AuthUtils::IPAddress clientAddr =
          AuthUtils::parseIPAddress(String(input.clientIp.c_str()));

      if (clientAddr.isValid() && AuthUtils::isLocalNetworkIP(clientAddr)) {
        decision.authenticated = true;
        decision.authenticatedVia = AuthType::LOCAL_ONLY;
      }
    }

    if (decision.authenticated) {
      break;
    }
  }

  if (!decision.authenticated) {
    if (startsWith(input.path, "/api/")) {
      decision.failureResponse = FailureResponse::Json401;
    } else if (AuthUtils::hasAuthType(requirements, AuthType::SESSION)) {
      if (deps.requiresInitialSetup && deps.requiresInitialSetup() &&
          !startsWith(input.path, "/setup")) {
        decision.failureResponse = FailureResponse::RedirectSetup;
      } else {
        decision.failureResponse = FailureResponse::RedirectLogin;
        decision.redirectUrl = "/login?redirect=" + input.path;
      }
    } else {
      decision.failureResponse = FailureResponse::Json403;
    }
  }

  return decision;
}

} // namespace WebPlatformAuth
