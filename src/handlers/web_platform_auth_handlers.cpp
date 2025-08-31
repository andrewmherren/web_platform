
#include "../../assets/account_page_html.h"
#include "../../assets/account_page_js.h"
#include "../../assets/login_page_html.h"
#include "../../include/auth/auth_constants.h"
#include "../../include/auth/auth_storage.h"
#include "../../include/web_platform.h"
#include "../../include/interface/auth_types.h"
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
  String clientIp = req.getHeader("X-Forwarded-For");
  if (clientIp.isEmpty()) {
    clientIp = req.getClientIp();
  }
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

    if (AuthStorage::validateCredentials(username, password)) {
      // Create session
      String sessionId = AuthStorage::createSession(username);

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
      loginHtml.replace("{{csrfToken}}", csrfToken);
      loginHtml.replace("{{redirectUrl}}", redirectUrl);
      loginHtml.replace("{{username}}", username);

      res.setContent(loginHtml);
      res.setStatus(401);
      return;
    }
  } else {
    // Show login form
    String loginHtml = String(LOGIN_PAGE_HTML);
    loginHtml.replace("{{csrfToken}}", csrfToken);
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
  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;

  // Get client IP for CSRF token
  String clientIp = req.getHeader("X-Forwarded-For");
  if (clientIp.isEmpty()) {
    clientIp = req.getClientIp();
  }
  String csrfToken = AuthStorage::createPageToken(clientIp);

  // Create API tokens
  std::vector<ApiToken> userTokens = AuthStorage::getUserTokens(username);

  // Generate token list HTML
  String tokensHtml = "";
  if (userTokens.empty()) {
    tokensHtml = "<p>No API tokens have been created yet.</p>";
  } else {
    tokensHtml = "<table class=\"token-table\">";
    tokensHtml += "<tr><th>Name</th><th>Created</th><th>Actions</th></tr>";
    for (const ApiToken &token : userTokens) {
      tokensHtml += "<tr>";
      tokensHtml += "<td>" + token.name + "</td>";
      // Format timestamp
      unsigned long ago =
          (millis() - token.createdAt) / 1000 / 60; // minutes ago
      String timeStr;
      if (ago < 60) {
        timeStr = String(ago) + " minutes ago";
      } else if (ago < 1440) {
        timeStr = String(ago / 60) + " hours ago";
      } else {
        timeStr = String(ago / 1440) + " days ago";
      }
      tokensHtml += "<td>" + timeStr + "</td>";
      tokensHtml += "<td><button class=\"btn btn-danger btn-sm\" "
                    "onclick=\"deleteToken('" +
                    token.token + "')\">Delete</button></td>";
      tokensHtml += "</tr>";
    }
    tokensHtml += "</table>";
  }

  String accountHtml = String(ACCOUNT_PAGE_HTML);
  accountHtml.replace("{{csrfToken}}", csrfToken);
  accountHtml.replace("{{username}}", username);
  accountHtml.replace("{{tokensHtml}}", tokensHtml);

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


void WebPlatform::updatePasswordApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_POST) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;
  String password = req.getParam("password");

  if (password.length() < 4) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password must be at "
                   "least 4 characters\"}");
    return;
  }

  bool success = AuthStorage::updateUserPassword(username, password);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"Password updated\"}");
  } else {
    res.setStatus(500);
    res.setContent(
        "{\"success\":false,\"message\":\"Failed to update password\"}");
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
  String tokenName = req.getParam("name");

  if (tokenName.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent(
        "{\"success\":false,\"message\":\"Token name is required\"}");
    return;
  }

  String token = AuthStorage::createApiToken(username, tokenName);

  res.setHeader("Content-Type", "application/json");
  res.setContent("{\"success\":true,\"token\":\"" + token + "\"}");
}

void WebPlatform::deleteTokenApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_POST) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  String username = auth.username;
  String token = req.getParam("token");

  if (token.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Token is required\"}");
    return;
  }

  // Verify token belongs to user
  ApiToken *apiToken = AuthStorage::findApiToken(token);
  if (apiToken == nullptr || apiToken->username != username) {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Token not found or "
                   "not authorized\"}");
    return;
  }

  bool success = AuthStorage::deleteApiToken(token);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"Token deleted\"}");
  } else {
    res.setStatus(500);
    res.setContent(
        "{\"success\":false,\"message\":\"Failed to delete token\"}");
  }
}