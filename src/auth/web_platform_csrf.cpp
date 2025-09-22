#include "auth/auth_constants.h"
#include "interface/auth_types.h"
#include "storage/auth_storage.h"
#include "web_platform.h"
#include <functional>

/**
 * WebPlatform CSRF Token Handling
 *
 * This file implements CSRF token generation and injection for the WebPlatform
 * It provides methods to automatically inject CSRF tokens into HTML responses.
 *
 */

// Add CSRF token cookie to a response
void WebPlatform::addCsrfCookie(WebResponse &res, const String &token) {
  // Set HttpOnly cookie with page token (CSRF protection)
  String cookieHeader = "page_token=" + token + "; Path=/; Max-Age=" +
                        String(AuthConstants::PAGE_TOKEN_DURATION_MS / 1000) +
                        "; SameSite=Strict; HttpOnly";

  res.setHeader("Set-Cookie", cookieHeader);
}