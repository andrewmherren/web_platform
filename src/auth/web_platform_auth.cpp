#include "../../include/web_platform.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/auth/auth_utils.h"
#include "../../include/interface/auth_types.h"
#include <functional>

// Initialize authentication system
void WebPlatform::initializeAuth() {
  // Initialize the auth storage
  AuthStorage::initialize();

  // Register authentication routes
  registerAuthRoutes();
}

// Check if a request is authenticated according to the route's requirements
bool WebPlatform::authenticateRequest(WebRequest &req, WebResponse &res,
                                      const AuthRequirements &requirements) {
  // If no auth is required, allow access
  if (!AuthUtils::requiresAuth(requirements)) {
    return true;
  }

  // Initialize auth context
  AuthContext authContext;
  authContext.clear();

  // Get client IP for validation - prioritize req.getClientIp() for consistency
  String clientIp = req.getClientIp();

  // Track if any auth method succeeded
  bool authSuccess = false; // Check each allowed auth type
  for (AuthType authType : requirements) {
    if (authType == AuthType::NONE) {
      // NONE always passes
      authSuccess = true;
      authContext.isAuthenticated = true;
      authContext.authenticatedVia = AuthType::NONE;
    } else if (authType == AuthType::SESSION) {
      // Get session cookie
      String sessionCookie = req.getHeader("Cookie");
      if (sessionCookie.indexOf("session=") >= 0) {
        // Extract session ID from cookie
        int start = sessionCookie.indexOf("session=") + 8;
        int end = sessionCookie.indexOf(";", start);
        if (end < 0)
          end = sessionCookie.length();
        String sessionId =
            sessionCookie.substring(start, end); // Validate session
        if (AuthStorage::validateSession(sessionId)) {AuthSession session = AuthStorage::findSession(sessionId);
          if (session.isValid()) {
            authSuccess = true;
            authContext.isAuthenticated = true;
            authContext.authenticatedVia = AuthType::SESSION;
            authContext.sessionId = sessionId;
            authContext.username = session.username;
            authContext.authenticatedAt = session.createdAt;
          }
        }
      }
    } else if (authType == AuthType::TOKEN) {
      // Check Authorization header first
      String authHeader = req.getHeader("Authorization");
      String token;
      if (authHeader.startsWith("Bearer ")) {
        // Extract token from Authorization header
        token = authHeader.substring(7);
      } else {
        // Check for token in query parameters
        token = req.getParam("access_token");
      }

      if (!token.isEmpty() && AuthStorage::validateApiToken(token)) {AuthApiToken apiToken = AuthStorage::findApiToken(token);
        if (apiToken.isValid()) {
          authSuccess = true;
          authContext.isAuthenticated = true;
          authContext.authenticatedVia = AuthType::TOKEN;
          authContext.token = token;
          authContext.username = apiToken.username;
          authContext.authenticatedAt = apiToken.createdAt;
        }
      }
    } else if (authType == AuthType::PAGE_TOKEN) {
      // Check for CSRF token in X-CSRF-Token header or form field
      String csrfToken = req.getHeader("X-CSRF-Token");
      if (csrfToken.isEmpty()) {
        csrfToken = req.getParam("_csrf");
      }

      if (!csrfToken.isEmpty()) {
        // Use validation and debug the process
        bool isValid = AuthStorage::validatePageToken(csrfToken, clientIp);

        if (isValid) {
          authSuccess = true;
          authContext.isAuthenticated = true;
          authContext.authenticatedVia = AuthType::PAGE_TOKEN;
        } else {
          Serial.printf(
              "PAGE_TOKEN validation failed for %s %s - Token invalid\n",
              wmMethodToString(req.getMethod()).c_str(),
              req.getPath().c_str());
        }
      } else {
        // Log the missing token for debugging
        Serial.printf("PAGE_TOKEN auth failed for %s %s - No token found\n",
                      wmMethodToString(req.getMethod()).c_str(),
                      req.getPath().c_str());

        // Print available headers for debugging
        Serial.println("Available request headers:");
        // List common headers for debugging
        const char *commonHeaders[] = {"Host", "User-Agent", "X-CSRF-Token",
                                       "Content-Type", "Accept"};
        for (const char *headerName : commonHeaders) {
          String value = req.getHeader(headerName);
          if (!value.isEmpty()) {
            Serial.printf("  %s: %s\n", headerName,
                          String(headerName).equals("Cookie") ? "[hidden]"
                                                              : value.c_str());
          }
        }
      }
    } else if (authType == AuthType::LOCAL_ONLY) {
      // Check if client IP is from local network
      AuthUtils::IPAddress clientAddr = AuthUtils::parseIPAddress(clientIp);
      
      if (clientAddr.isValid()) {
        bool isLocalNetwork = AuthUtils::isLocalNetworkIP(clientAddr);
        
        if (isLocalNetwork) {
          authSuccess = true;
          authContext.isAuthenticated = true;
          authContext.authenticatedVia = AuthType::LOCAL_ONLY;
        } else {
          Serial.printf("LOCAL_ONLY auth failed for %s - IP %s is not in local network\n",
                       req.getPath().c_str(), clientIp.c_str());
        }
      } else {
        Serial.printf("LOCAL_ONLY auth failed for %s - Invalid IP address: %s\n",
                     req.getPath().c_str(), clientIp.c_str());
      }
    }

    // If any auth method succeeded, we can stop checking
    if (authSuccess) {
      break;
    }
  }

  // Set auth context regardless of outcome
  req.setAuthContext(authContext);

  // If authentication failed, handle according to route type
  if (!authSuccess) {
    if (req.getPath().startsWith("/api/")) {
      // API routes return 401 JSON
      res.setStatus(401);
      res.setHeader("Content-Type", "application/json");
      res.setContent("{\"error\":\"unauthorized\",\"message\":\"Authentication "
                     "required\",\"code\":401}");
    } else if (AuthUtils::hasAuthType(requirements, AuthType::SESSION)) {
      // UI routes redirect to login
      res.redirect("/login?redirect=" + req.getPath());
    } else {
      // Other routes get 403 Forbidden
      res.setStatus(403);
      res.setHeader("Content-Type", "application/json");
      res.setContent("{\"error\":\"forbidden\",\"message\":\"Access "
                     "denied\",\"code\":403}");
    }
    return false;
  }

  return true;
}