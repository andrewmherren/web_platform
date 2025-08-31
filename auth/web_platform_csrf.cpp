#include "../web_platform.h"
#include "auth_constants.h"
#include "auth_storage.h"
#include <auth_types.h>
#include <functional>

/**
 * WebPlatform CSRF Token Handling
 *
 * This file implements CSRF token generation and injection for the WebPlatform
 * It provides methods to automatically inject CSRF tokens into HTML responses.
 *
 * CRITICAL: When an HTML page loads auth_utils.js, it will automatically
 * extract the CSRF token from either the meta tag or the cookie, and
 * include it in subsequent XHR/fetch requests as an X-CSRF-Token header.
 */

// Process HTML content for CSRF token injection
String WebPlatform::injectCsrfToken(const String &html,
                                    const String &clientIp) {
  // Skip processing if no token placeholder
  if (html.indexOf("{{csrfToken}}") < 0) {
    return html;
  }

  // Generate a new CSRF token
  String csrfToken = AuthStorage::createPageToken(clientIp);

  // Replace all instances of the token placeholder
  String processedHtml = html;
  processedHtml.replace("{{csrfToken}}", csrfToken);

  return processedHtml;
}

// Add CSRF token cookie to a response
void WebPlatform::addCsrfCookie(WebResponse &res, const String &token) {
  // Set HttpOnly cookie with page token (CSRF protection)
  String cookieHeader = "page_token=" + token + "; Path=/; Max-Age=" +
                        String(AuthConstants::PAGE_TOKEN_DURATION_MS / 1000) +
                        "; SameSite=Strict; HttpOnly";

  res.setHeader("Set-Cookie", cookieHeader);
}

// Process response for CSRF token injection
void WebPlatform::processCsrfForResponse(WebRequest &req, WebResponse &res) {
  // Only process HTML responses that haven't been sent
  if (res.isResponseSent() || res.getMimeType() != "text/html") {
    return;
  }

  // Get client IP for token generation
  String clientIp = req.getHeader("X-Forwarded-For");
  if (clientIp.isEmpty()) {
    clientIp = req.getClientIp();
  }

  // Get HTML content from response
  String html = res.getContent();

  // Check if we need to inject a CSRF token
  if (html.indexOf("{{csrfToken}}") >= 0) {
    // Generate a new CSRF token
    String csrfToken = AuthStorage::createPageToken(clientIp);

    // Replace the token placeholder
    html.replace("{{csrfToken}}", csrfToken);

    // Update the response content
    res.setContent(html, "text/html");

    // Add CSRF cookie
    addCsrfCookie(res, csrfToken);
  }
}