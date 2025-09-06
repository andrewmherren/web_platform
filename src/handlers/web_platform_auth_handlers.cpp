#include "../../assets/account_page_html.h"
#include "../../assets/account_page_js.h"
#include "../../assets/login_page_html.h"
#include "../../include/auth/auth_constants.h"
#include "../../include/interface/auth_types.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"
#include <functional>

void WebPlatform::loginPageHandler(WebRequest &req, WebResponse &res) {
  String redirectUrl = req.getParam("redirect");
  if (redirectUrl.isEmpty()) {
    redirectUrl = "/";
  }

  // Check if already logged in
  const AuthContext &auth = req.getAuthContext();
  if (auth.hasValidSession()) {
    res.redirect(redirectUrl);
    return;
  }

  // Create a CSRF token for the form
  String clientIp = req.getClientIp();
  String csrfToken = AuthStorage::createPageToken(clientIp);

  if (req.getMethod() == WebModule::WM_POST) {
    // Verify CSRF token
    String formToken = req.getParam("_csrf");

    if (formToken.isEmpty() ||
        !AuthStorage::validatePageToken(formToken, clientIp)) {
      res.setStatus(403);
      res.setContent("CSRF token validation failed. Please try again.");
      return;
    }

    // Process login form
    String username = req.getParam("username");
    String password = req.getParam("password");
    String userId = AuthStorage::validateCredentials(username, password);
    if (!userId.isEmpty()) {
      // Create session
      String sessionId = AuthStorage::createSession(userId);

      // Set session cookie - HTTP only for security
      res.setHeader("Set-Cookie",
                    "session=" + sessionId + "; Path=/; Max-Age=" +
                        String(AuthConstants::SESSION_DURATION_MS / 1000) +
                        "; SameSite=Strict; HttpOnly");

      // Redirect to requested page
      res.redirect(redirectUrl);
      return;
    } else {
      // Invalid credentials, show login form with error
      String loginHtml = String(LOGIN_PAGE_ERROR_HTML);
      loginHtml.replace("{{redirectUrl}}", redirectUrl);

      res.setContent(loginHtml);
      res.setStatus(401);
      return;
    }
  } else {
    // Show login form
    String loginHtml = String(LOGIN_PAGE_HTML);
    loginHtml.replace("{{redirectUrl}}", redirectUrl);

    res.setContent(loginHtml);

    // Set HttpOnly cookie with page token (CSRF protection)
    res.setHeader("Set-Cookie",
                  "page_token=" + csrfToken + "; Path=/; Max-Age=" +
                      String(AuthConstants::PAGE_TOKEN_DURATION_MS / 1000) +
                      "; SameSite=Strict; HttpOnly");
  }
}

void WebPlatform::logoutPageHandler(WebRequest &req, WebResponse &res) {
  // Get session cookie
  String sessionCookie = req.getHeader("Cookie");
  if (sessionCookie.indexOf("session=") >= 0) {
    // Extract session ID from cookie
    int start = sessionCookie.indexOf("session=") + 8;
    int end = sessionCookie.indexOf(";", start);
    if (end < 0)
      end = sessionCookie.length();
    String sessionId = sessionCookie.substring(start, end);

    // Delete session from storage
    AuthStorage::deleteSession(sessionId);
  }

  // Clear session cookie by setting an expired date
  res.setHeader("Set-Cookie",
                "session=; Path=/; Max-Age=0; SameSite=Strict; HttpOnly");

  // Redirect to login page
  res.redirect("/login");
}

void WebPlatform::accountPageHandler(WebRequest &req, WebResponse &res) {
  String csrfToken = AuthStorage::createPageToken(req.getClientIp());

  String accountHtml = String(ACCOUNT_PAGE_HTML);

  res.setContent(accountHtml);

  // Set HttpOnly cookie with page token (CSRF protection)
  res.setHeader("Set-Cookie",
                "page_token=" + csrfToken + "; Path=/; Max-Age=" +
                    String(AuthConstants::PAGE_TOKEN_DURATION_MS / 1000) +
                    "; SameSite=Strict; HttpOnly");
}

void WebPlatform::accountPageJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setContent(FPSTR(ACCOUNT_PAGE_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::updateUserApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_PUT) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;
  String password = req.getJsonParam("password");

  // Currently only password updates are supported
  if (password.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password is required\"}");
    return;
  }

  if (password.length() < 4) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password must be at "
                   "least 4 characters\"}");
    return;
  }

  // Get user by username to obtain user ID
  AuthUser user = AuthStorage::findUserByUsername(username);
  bool success = AuthStorage::updateUserPassword(user.id, password);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"User updated\"}");
  } else {
    res.setStatus(500);
    res.setContent("{\"success\":false,\"message\":\"Failed to update user\"}");
  }
}

void WebPlatform::createTokenApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_POST) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;

  // Try to get token name from JSON body first
  String tokenName = req.getJsonParam("name");

  // Fallback to form parameter for backward compatibility
  if (tokenName.isEmpty()) {
    tokenName = req.getParam("name");
  }

  if (tokenName.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent(
        "{\"success\":false,\"message\":\"Token name is required\"}");
    return;
  }

  // Get user by username to obtain user ID
  AuthUser user = AuthStorage::findUserByUsername(username);
  String token = AuthStorage::createApiToken(user.id, tokenName);

  res.setHeader("Content-Type", "application/json");
  res.setContent("{\"success\":true,\"token\":\"" + token + "\"}");
}

void WebPlatform::deleteTokenApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_DELETE) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;

  // Extract token ID from URL path (e.g., /api/tokens/{id})
  String tokenId = req.getRouteParameter("id");

  if (tokenId.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Token ID is required\"}");
    return;
  }

  // Need to update AuthStorage to support findApiTokenById
  // For now, we'll get all user tokens and find the one with matching ID
  AuthUser user = AuthStorage::findUserByUsername(username);
  std::vector<AuthApiToken> userTokens = AuthStorage::getUserApiTokens(user.id);

  AuthApiToken targetToken;
  for (const AuthApiToken &token : userTokens) {
    if (token.id == tokenId) {
      targetToken = token;
      break;
    }
  }

  if (!targetToken.isValid()) {
    res.setStatus(404);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Token not found\"}");
    return;
  }

  // Verify token belongs to user
  if (targetToken.username != username) {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Not authorized to delete "
                   "this token\"}");
    return;
  }

  bool success = AuthStorage::deleteApiToken(targetToken.token);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"Token deleted\"}");
  } else {
    res.setStatus(500);
    res.setContent(
        "{\"success\":false,\"message\":\"Failed to delete token\"}");
  }
}