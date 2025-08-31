#ifndef AUTH_TYPES_H
#define AUTH_TYPES_H

#include <Arduino.h>
#include <vector>

/**
 * Authentication Types for Route Protection
 *
 * This enum defines the types of authentication that can be required
 * for accessing routes. Multiple auth types can be combined to allow
 * alternative authentication methods (OR logic).
 */
enum class AuthType {
  NONE = 0,       // No authentication required
  SESSION = 1,    // Session-based authentication (cookie)
  TOKEN = 2,      // Token-based authentication (header/param)
  LOCAL_ONLY = 4, // Local network access only
  PAGE_TOKEN = 8  // CSRF protection for pages
};

/**
 * Authentication Requirements Container
 *
 * Contains a list of AuthType values that represent allowed authentication
 * methods for a route. If multiple types are present, any one of them
 * can satisfy the requirement (OR logic).
 */
using AuthRequirements = std::vector<AuthType>;

/**
 * Authentication Context
 *
 * Contains information about the current authentication state
 * for a request, populated during authentication middleware processing.
 */
struct AuthContext {
  bool isAuthenticated = false;
  AuthType authenticatedVia = AuthType::NONE;
  String sessionId;
  String token;
  String username;
  unsigned long authenticatedAt = 0; // Timestamp of authentication

  AuthContext() = default;

  // Helper methods
  bool hasValidSession() const {
    return isAuthenticated && authenticatedVia == AuthType::SESSION &&
           !sessionId.isEmpty();
  }

  bool hasValidToken() const {
    return isAuthenticated && authenticatedVia == AuthType::TOKEN &&
           !token.isEmpty();
  }

  void clear() {
    isAuthenticated = false;
    authenticatedVia = AuthType::NONE;
    sessionId = "";
    token = "";
    username = "";
    authenticatedAt = 0;
  }
};

/**
 * Helper functions for working with AuthType
 */
namespace AuthUtils {
// Check if an AuthType is present in requirements
bool hasAuthType(const AuthRequirements &requirements, AuthType type);

// Check if any authentication is required
bool requiresAuth(const AuthRequirements &requirements);

// Convert AuthType to string for debugging
String authTypeToString(AuthType type);

} // namespace AuthUtils

#endif // AUTH_TYPES_H