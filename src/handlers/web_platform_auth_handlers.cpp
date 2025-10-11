#include "../../assets/account_page_html.h"
#include "../../assets/account_page_js.h"
#include "../../assets/login_page_html.h"
#include "auth/auth_constants.h"
#include "storage/auth_storage.h"
#include "utilities/json_response_builder.h"
#include "web_platform.h"
#include <functional>
#include <interface/auth_types.h>

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

  // Show login form - use progmem content directly
  res.setProgmemContent(LOGIN_PAGE_HTML, "text/html");

  // Set HttpOnly cookie with page token (CSRF protection)
  res.setHeader("Set-Cookie",
                "page_token=" + csrfToken + "; Path=/; Max-Age=" +
                    String(AuthConstants::PAGE_TOKEN_DURATION_MS / 1000) +
                    "; SameSite=Strict; HttpOnly");
}

void WebPlatform::loginApiHandler(WebRequest &req, WebResponse &res) {
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
    res.setProgmemContent(LOGIN_PAGE_ERROR_HTML, "text/html");
    res.setStatus(401);
    return;
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

  res.setProgmemContent(ACCOUNT_PAGE_HTML, "text/html");

  // Set HttpOnly cookie with page token (CSRF protection)
  res.setHeader("Set-Cookie",
                "page_token=" + csrfToken + "; Path=/; Max-Age=" +
                    String(AuthConstants::PAGE_TOKEN_DURATION_MS / 1000) +
                    "; SameSite=Strict; HttpOnly");
}

void WebPlatform::accountPageJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(ACCOUNT_PAGE_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::deleteTokenApiHandler(WebRequest &req, WebResponse &res) {
  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;

  // Extract token ID from URL path (e.g., /api/tokens/{id})
  String tokenId = req.getRouteParameter("id");

  if (tokenId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "Token ID is required", 400);
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
    JsonResponseBuilder::createErrorResponse(res, "Token not found", 404);
    return;
  }

  // Verify token belongs to user
  if (targetToken.username != username) {
    JsonResponseBuilder::createErrorResponse(
        res, "Not authorized to delete this token", 403);
    return;
  }

  bool success = AuthStorage::deleteApiToken(targetToken.token);

  if (success) {
    JsonResponseBuilder::createSuccessResponse(res, "Token deleted");
  } else {
    JsonResponseBuilder::createErrorResponse(res, "Failed to delete token",
                                             500);
  }
}