#include "web_platform.h"
#include "route_entry.h"
#include <auth_types.h>

// Authentication implementation for WebPlatform

// Session storage - simple in-memory implementation for Phase 2
// In a real implementation, consider persistent storage and security features
struct SessionInfo {
    String sessionId;
    String username;
    unsigned long createdAt;
    unsigned long lastActivity;
    
    SessionInfo() : createdAt(0), lastActivity(0) {}
    
    SessionInfo(const String& id, const String& user) 
        : sessionId(id), username(user), 
          createdAt(millis()), lastActivity(millis()) {}
    
    bool isValid() const {
        // Sessions expire after 1 hour of inactivity (3,600,000 ms)
        return (millis() - lastActivity) < 3600000;
    }
    
    void updateActivity() {
        lastActivity = millis();
    }
};

// Global session storage
static std::vector<SessionInfo> activeSessions;

// API token storage - simple in-memory implementation for Phase 2
struct TokenInfo {
    String token;
    String username;
    String description;
    unsigned long createdAt;
    
    TokenInfo() : createdAt(0) {}
    
    TokenInfo(const String& t, const String& user, const String& desc) 
        : token(t), username(user), description(desc), createdAt(millis()) {}
    
    bool isValid() const {
        return true; // Tokens don't expire in this simple implementation
    }
};

// Global token storage
static std::vector<TokenInfo> apiTokens;

// Generate a simple session ID (in a real implementation, use secure random generation)
String generateSessionId() {
    String id = "";
    for (int i = 0; i < 32; i++) {
        id += "0123456789abcdef"[random(16)];
    }
    return id;
}

// Generate a simple API token (in a real implementation, use secure random generation)
String generateApiToken() {
    String token = "";
    for (int i = 0; i < 40; i++) {
        token += "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[random(62)];
    }
    return token;
}

// Create a new session
String createSession(const String& username) {
    String sessionId = generateSessionId();
    activeSessions.push_back(SessionInfo(sessionId, username));
    return sessionId;
}

// Get session info by session ID
SessionInfo* getSession(const String& sessionId) {
    for (auto& session : activeSessions) {
        if (session.sessionId == sessionId && session.isValid()) {
            session.updateActivity();
            return &session;
        }
    }
    return nullptr;
}

// Invalidate a session
void invalidateSession(const String& sessionId) {
    for (auto it = activeSessions.begin(); it != activeSessions.end(); ++it) {
        if (it->sessionId == sessionId) {
            activeSessions.erase(it);
            return;
        }
    }
}

// Create a new API token
String createApiToken(const String& username, const String& description) {
    String token = generateApiToken();
    apiTokens.push_back(TokenInfo(token, username, description));
    return token;
}

// Validate an API token
TokenInfo* validateApiToken(const String& token) {
    for (auto& tokenInfo : apiTokens) {
        if (tokenInfo.token == token && tokenInfo.isValid()) {
            return &tokenInfo;
        }
    }
    return nullptr;
}

// Check if a request is authenticated according to the route's requirements
bool WebPlatform::authenticateRequest(WebRequest& req, WebResponse& res, const AuthRequirements& requirements) {
    // If no auth is required, or only NONE is specified, allow access
    if (!AuthUtils::requiresAuth(requirements)) {
        return true;
    }
    
    // Initialize auth context
    AuthContext authContext;
    authContext.clear();
    
    // Check for session authentication if required
    if (AuthUtils::hasAuthType(requirements, AuthType::SESSION)) {
        // Get session cookie
        String sessionId = req.getHeader("Cookie");
        if (sessionId.indexOf("session=") >= 0) {
            // Extract session ID from cookie
            int start = sessionId.indexOf("session=") + 8;
            int end = sessionId.indexOf(";", start);
            if (end < 0) end = sessionId.length();
            sessionId = sessionId.substring(start, end);
            
            // Validate session
            SessionInfo* session = getSession(sessionId);
            if (session != nullptr) {
                // Session is valid
                authContext.isAuthenticated = true;
                authContext.authenticatedVia = AuthType::SESSION;
                authContext.sessionId = sessionId;
                authContext.username = session->username;
                authContext.authenticatedAt = session->createdAt;
                
                // Set auth context and allow access
                req.setAuthContext(authContext);
                return true;
            }
        }
    }
    
    // Check for token authentication if required
    if (AuthUtils::hasAuthType(requirements, AuthType::TOKEN)) {
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
        
        if (token.length() > 0) {
            // Validate token
            TokenInfo* tokenInfo = validateApiToken(token);
            if (tokenInfo != nullptr) {
                // Token is valid
                authContext.isAuthenticated = true;
                authContext.authenticatedVia = AuthType::TOKEN;
                authContext.token = token;
                authContext.username = tokenInfo->username;
                authContext.authenticatedAt = tokenInfo->createdAt;
                
                // Set auth context and allow access
                req.setAuthContext(authContext);
                return true;
            }
        }
    }
    
    // If we get here, authentication failed
    // Send appropriate response based on preferred auth method
    
    if (AuthUtils::hasAuthType(requirements, AuthType::SESSION)) {
        // Redirect to login page for session auth
        res.redirect("/login?redirect=" + req.getPath());
        return false;
    } else if (AuthUtils::hasAuthType(requirements, AuthType::TOKEN)) {
        // Return 401 Unauthorized for API token auth
        res.setStatus(401);
        res.setHeader("WWW-Authenticate", "Bearer");
        res.setContent("{\"error\":\"unauthorized\",\"message\":\"Valid authentication token required\"}", "application/json");
        return false;
    } else {
        // Generic 403 Forbidden for other auth types
        res.setStatus(403);
        res.setContent("{\"error\":\"forbidden\",\"message\":\"Access denied\"}", "application/json");
        return false;
    }
}

// Register authentication-related routes
void WebPlatform::registerAuthRoutes() {
    // Login page
    registerRoute("/login", [this](WebRequest& req, WebResponse& res) {
        String redirectUrl = req.getParam("redirect");
        if (redirectUrl.isEmpty()) {
            redirectUrl = "/";
        }
        
        if (req.getMethod() == "POST") {
            // Process login form
            String username = req.getParam("username");
            String password = req.getParam("password");
            
            // Simple hardcoded authentication for Phase 2
            // In a real implementation, use secure password storage/validation
            if (username == "admin" && password == "password") {
                // Create session
                String sessionId = createSession(username);
                
                // Set session cookie
                res.setHeader("Set-Cookie", "session=" + sessionId + "; Path=/; Max-Age=3600; SameSite=Strict");
                
                // Redirect to requested page
                res.redirect(redirectUrl);
                return;
            } else {
                // Invalid credentials, show login form with error
                String loginHtml = R"(
                    <!DOCTYPE html>
                    <html>
                    <head>
                        <title>Login - TickerTape</title>
                        <link rel="stylesheet" href="/styles.css">
                        <meta name="viewport" content="width=device-width, initial-scale=1">
                    </head>
                    <body>
                        <div class="container">
                            <h1>Login</h1>
                            <div class="card error">Invalid username or password</div>
                            <form method="post" action="/login?redirect=)" + redirectUrl + R"(">
                                <div class="form-group">
                                    <label for="username">Username:</label>
                                    <input type="text" id="username" name="username" class="form-control" value=")" + username + R"(" required>
                                </div>
                                <div class="form-group">
                                    <label for="password">Password:</label>
                                    <input type="password" id="password" name="password" class="form-control" required>
                                </div>
                                <button type="submit" class="btn btn-primary">Login</button>
                            </form>
                        </div>
                    </body>
                    </html>
                )";
                
                res.setContent(loginHtml);
                res.setStatus(401);
                return;
            }
        } else {
            // Show login form
            String loginHtml = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>Login - TickerTape</title>
                    <link rel="stylesheet" href="/styles.css">
                    <meta name="viewport" content="width=device-width, initial-scale=1">
                </head>
                <body>
                    <div class="container">
                        <h1>Login</h1>
                        <form method="post" action="/login?redirect=)" + redirectUrl + R"(">
                            <div class="form-group">
                                <label for="username">Username:</label>
                                <input type="text" id="username" name="username" class="form-control" required>
                            </div>
                            <div class="form-group">
                                <label for="password">Password:</label>
                                <input type="password" id="password" name="password" class="form-control" required>
                            </div>
                            <button type="submit" class="btn btn-primary">Login</button>
                        </form>
                    </div>
                </body>
                </html>
            )";
            
            res.setContent(loginHtml);
        }
    });
    
    // Logout endpoint
    registerRoute("/logout", [](WebRequest& req, WebResponse& res) {
        // Get session cookie
        String sessionId = req.getHeader("Cookie");
        if (sessionId.indexOf("session=") >= 0) {
            // Extract session ID from cookie
            int start = sessionId.indexOf("session=") + 8;
            int end = sessionId.indexOf(";", start);
            if (end < 0) end = sessionId.length();
            sessionId = sessionId.substring(start, end);
            
            // Invalidate session
            invalidateSession(sessionId);
        }
        
        // Clear session cookie
        res.setHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
        
        // Redirect to home page
        res.redirect("/");
    }, {AuthType::NONE});
    
    // API Token management
    registerRoute("/api/tokens", [](WebRequest& req, WebResponse& res) {
        const AuthContext& auth = req.getAuthContext();
        
        if (req.getMethod() == "GET") {
            // List user's tokens
            String jsonResponse = "[";
            bool first = true;
            
            for (const auto& token : apiTokens) {
                if (token.username == auth.username) {
                    if (!first) jsonResponse += ",";
                    first = false;
                    
                    jsonResponse += "{";
                    jsonResponse += "\"token\":\"" + token.token.substring(0, 8) + "...\",";
                    jsonResponse += "\"description\":\"" + token.description + "\",";
                    jsonResponse += "\"created\":" + String(token.createdAt);
                    jsonResponse += "}";
                }
            }
            
            jsonResponse += "]";
            res.setContent(jsonResponse, "application/json");
        } else if (req.getMethod() == "POST") {
            // Create new token
            String description = req.getParam("description");
            if (description.isEmpty()) {
                description = "API Token created at " + String(millis());
            }
            
            String token = createApiToken(auth.username, description);
            
            String jsonResponse = "{";
            jsonResponse += "\"token\":\"" + token + "\",";
            jsonResponse += "\"description\":\"" + description + "\",";
            jsonResponse += "\"message\":\"Store this token securely. It won't be shown again.\"";
            jsonResponse += "}";
            
            res.setContent(jsonResponse, "application/json");
        }
    }, {AuthType::SESSION});
}