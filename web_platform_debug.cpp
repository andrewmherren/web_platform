#include "auth/auth_storage.h"
#include "web_platform.h"
#include <auth_types.h>

/**
 * WebPlatform Debug Utilities (Simplified)
 *
 * This file provides simple debug functions to help diagnose authentication
 * and other issues with the WebPlatform.
 */

// Print simple debug info about CSRF tokens
void printPageTokensDebug() {
  Serial.println("\n=== CSRF Token Debugging ===\n");
  Serial.println("CSRF tokens are used to protect against cross-site request "
                 "forgery attacks.");
  Serial.println(
      "Each HTML page with {{csrfToken}} placeholder gets a unique token.");
  Serial.println(
      "API requests must include this token in the X-CSRF-Token header.");
  Serial.println("\nToken validation checks:\n- Token exists in storage\n- "
                 "Token is not expired\n- Client IP matches");
  Serial.println(
      "\nTo debug token issues:\n- Check browser console for client-side token "
      "extraction\n- Check server logs for validation messages\n");
  Serial.println("================================\n");
}

// Clean all expired tokens and print summary
void cleanPageTokensDebug() {
  Serial.println("\n=== Cleaning Expired Page Tokens ===");
  AuthStorage::cleanExpiredPageTokens();
  Serial.println("Cleaned expired CSRF tokens");
  Serial.println("==================================\n");
}

// WebPlatform method implementations for debug helpers
void WebPlatform::debugAuthState() const {
  Serial.println("\n=== WebPlatform Authentication State ===\n");
  Serial.println("Active Sessions: Use login/logout to manage sessions");
  Serial.println("\nRegistered Users: Default admin user is available");
  printPageTokensDebug();
  Serial.println("=========================================\n");
}

void WebPlatform::cleanExpiredTokens() {
  Serial.println("\n=== Cleaning Expired Tokens and Sessions ===\n");
  Serial.println("Cleaning expired sessions...");
  AuthStorage::cleanExpiredSessions();
  Serial.println("Cleaning expired API tokens...");
  AuthStorage::cleanExpiredTokens();
  cleanPageTokensDebug();
  Serial.println("\n===========================================\n");
}

// Debug function to log CSRF token validation details
void debugCsrfTokenValidation(const String &token, const String &clientIp,
                              bool isValid) {
  Serial.println("CSRF Token Validation Debug:");
  Serial.printf(
      "  Token: %s\n",
      token.substring(0, 6).c_str()); // Only show first 6 chars for security
  Serial.printf("  Client IP: %s\n", clientIp.c_str());
  Serial.printf("  Token found in storage: %s\n", isValid ? "yes" : "no");

  // We can't access private pageTokens directly, so just show validation result
  Serial.printf("  Validation result: %s\n", isValid ? "VALID" : "INVALID");

  if (isValid) {
    Serial.println("  Token is valid for this client IP");
  } else {
    Serial.println("  Token validation failed - could be expired, invalid, or "
                   "IP mismatch");
  }

  // Print all active tokens for debugging
  printPageTokensDebug();
}

// Helper function to check for CSRF token in request and validate it
bool validateRequestCsrfToken(WebRequest &req, const String &clientIp) {
  // Get CSRF token from headers or parameters
  String csrfToken = req.getHeader("X-CSRF-Token");
  if (csrfToken.isEmpty()) {
    csrfToken = req.getParam("_csrf");
  }

  if (csrfToken.isEmpty()) {
    Serial.printf("No CSRF token found in request: %s %s\n",
                  httpMethodToString(req.getMethod()).c_str(),
                  req.getPath().c_str());
    return false;
  }

  // Validate token
  bool isValid = AuthStorage::validatePageToken(csrfToken, clientIp);

  // Debug the validation result
  debugCsrfTokenValidation(csrfToken, clientIp, isValid);

  return isValid;
}