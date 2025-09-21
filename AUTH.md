# WebPlatform Authentication System

This guide explains how to implement and use WebPlatform's built-in authentication system for securing embedded web applications.

## Overview

WebPlatform provides a comprehensive authentication system with multiple authentication methods that can be applied at the route level. This enables flexible security implementations from simple login systems to enterprise-grade API access controls with token-based authentication.

## Authentication Types

The system supports several authentication methods that can be combined:

- `AuthType::NONE` - Public access (no authentication required)
- `AuthType::SESSION` - Cookie-based session authentication (web UI)
- `AuthType::TOKEN` - Bearer token authentication (API access)
- `AuthType::PAGE_TOKEN` - CSRF protection for form submissions
- `AuthType::LOCAL_ONLY` - Restrict access to local network only

## For Application Developers

### Basic Login System

Protect your entire application behind a login screen:

```cpp
#include <web_platform.h>
// #include <device_module.h>  // Your web modules

void setup() {
    Serial.begin(115200);
    
    // Set up navigation with authentication-aware items
    std::vector<NavigationItem> navItems = {
        NavigationItem("Dashboard", "/"),
        NavigationItem("Device Control", "/device/"),
        NavigationItem("Settings", "/settings"),
        Authenticated(NavigationItem("Account", "/account")),
        Authenticated(NavigationItem("Logout", "/logout")),
        Unauthenticated(NavigationItem("Login", "/login"))
    };
    IWebModule::setNavigationMenu(navItems);
    
    webPlatform.begin("SecureDevice");
    
    if (webPlatform.isConnected()) {
        // Register your modules (they inherit authentication from WebPlatform)
        // webPlatform.registerModule("/device", &deviceModule);
        
        // Create protected dashboard
        webPlatform.registerWebRoute("/", [](WebRequest& req, WebResponse& res) {
            const AuthContext& auth = req.getAuthContext();
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>Dashboard</title></head><body>
                <h1>Welcome, )" + auth.username + R"(!</h1>
                <p>You are logged in via )" + 
                (auth.authenticatedVia == AuthType::SESSION ? "web interface" : "API token") + R"(</p>
                <ul>
                    <li><a href="/device/">Device Control</a></li>
                    <li><a href="/settings">Settings</a></li>
                    <li><a href="/account">Manage Account</a></li>
                </ul>
                </body></html>
            )";
            res.setContent(html, "text/html");
        }, {AuthType::SESSION, AuthType::TOKEN});  // Allow both web and API access
        
        // Replace module routes to add authentication if needed
        // webPlatform.registerWebRoute("/device/", protectedHandler, {AuthType::SESSION});
    }
}
```

### API Token Management

Allow users to create API tokens for programmatic access:

```cpp
void setup() {
    // ... basic setup ...
    
    if (webPlatform.isConnected()) {
        // API management page (web interface)
        webPlatform.registerWebRoute("/api-tokens", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head>
                    <title>API Token Management</title>
                    <script>
                        async function createToken() {
                            const description = document.getElementById('description').value;
                            const csrf = document.querySelector('meta[name="csrf-token"]').content;
                            
                            // Use WebPlatform's RESTful API
                            const response = await fetch('/api/user/tokens', {
                                method: 'POST',
                                headers: {
                                    'Content-Type': 'application/json',
                                    'X-CSRF-Token': csrf
                                },
                                body: JSON.stringify({name: description})
                            });
                            
                            const result = await response.json();
                            if (result.success) {
                                document.getElementById('result').innerHTML = 
                                    '<div class="success">Token created: ' + result.token + 
                                    '<br><strong>Save this now - it won\'t be shown again!</strong></div>';
                            }
                        }
                    </script>
                </head><body>
                    <h1>API Token Management</h1>
                    <div>
                        <input type="text" id="description" placeholder="Token description">
                        <button onclick="createToken()">Create Token</button>
                    </div>
                    <div id="result"></div>
                </body></html>
            )";
            res.setContent(html, "text/html");
        }, {AuthType::SESSION});
        
        // API endpoints that accept both session and token auth
        webPlatform.registerApiRoute("/device/status", [](WebRequest& req, WebResponse& res) {
            // This route can be accessed via web interface OR API token
            String status = "{\"status\":\"online\",\"uptime\":" + String(millis()) + "}";
            res.setContent(status, "application/json");
        }, {AuthType::SESSION, AuthType::TOKEN});
        
        webPlatform.registerApiRoute("/device/control", [](WebRequest& req, WebResponse& res) {
            if (req.getMethod() != WebModule::WM_POST) {
                res.setStatus(405);
                res.setContent("{\"error\":\"Method not allowed\"}", "application/json");
                return;
            }
            
            String command = req.getParam("command");
            DEBUG_PRINTLN("Received command: " + command);
            
            // Execute your device command logic here
            bool success = (command == "restart" || command == "status");
            
            String result = success ? 
                "{\"success\":true,\"command\":\"" + command + "\"}" : 
                "{\"success\":false,\"error\":\"Unknown command\"}";
            res.setContent(result, "application/json");
        }, {AuthType::SESSION, AuthType::TOKEN}, WebModule::WM_POST);
    }
}
```

### CSRF Protection with AuthUtils

WebPlatform provides built-in JavaScript utilities for handling authentication and CSRF tokens:

```javascript
// Using the AuthUtils helper for fetch requests with CSRF protection
async function updateConfig(settings) {
  try {
    const result = await AuthUtils.fetch('/api/device/update-config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(settings)
    });
    
    // AuthUtils.fetch automatically handles CSRF tokens
    const data = await result.json();
    UIUtils.showAlert('Success', 'Configuration updated', 'success');
    return data;
  } catch (error) {
    UIUtils.showAlert('Error', error.message, 'error');
    console.error('Failed to update config:', error);
  }
}

// For form submissions, simply use the form's action and method
document.getElementById('config-form').addEventListener('submit', async function(e) {
  e.preventDefault();
  
  // AuthUtils.submitForm handles CSRF tokens automatically
  const result = await AuthUtils.submitForm(this);
  
  if (result.success) {
    UIUtils.showAlert('Success', 'Form submitted successfully', 'success');
  } else {
    UIUtils.showAlert('Error', result.error, 'error');
  }
});
```

### Custom Login Page

Override the default login page with your own branding:

```cpp
void setup() {
    // ... basic setup ...
    
    if (webPlatform.isConnected()) {
        // Replace the default login page
        webPlatform.registerWebRoute("/login", [](WebRequest& req, WebResponse& res) {
            if (req.getMethod() == WebModule::WM_POST) {
                // Handle login submission
                String username = req.getParam("username");
                String password = req.getParam("password");
                
                // Custom authentication logic here
                if (authenticateUser(username, password)) {
                    // Redirect to requested page
                    String redirect = req.getParam("redirect", "/");
                    res.redirect(redirect);
                    return;
                }
                
                // Show error
                res.setContent(getLoginPage("Invalid credentials"), "text/html");
                res.setStatus(401);
            } else {
                // Show login form
                res.setContent(getLoginPage(), "text/html");
            }
        }, {AuthType::NONE});
    }
}

String getLoginPage(const String& error = "") {
    String html = R"(
        <!DOCTYPE html>
        <html><head>
            <title>Login - My Device</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 40px; }
                .login-form { max-width: 300px; margin: 0 auto; }
                .error { color: red; margin-bottom: 10px; }
                input { width: 100%; padding: 8px; margin: 5px 0; }
                button { width: 100%; padding: 10px; background: #007cba; color: white; border: none; }
            </style>
        </head><body>
            <div class="login-form">
                <h1>Device Login</h1>
    )";
    
    if (error.length() > 0) {
        html += "<div class='error'>" + error + "</div>";
    }
    
    html += R"(
                <form method="post">
                    <input type="text" name="username" placeholder="Username" required>
                    <input type="password" name="password" placeholder="Password" required>
                    <button type="submit">Login</button>
                </form>
            </div>
        </body></html>
    )";
    
    return html;
}
```

## For Module Developers

### Protecting Module Routes

As a module developer, you can specify authentication requirements for your routes:

```cpp
class MyDeviceModule : public IWebModule {
public:
    std::vector<RouteVariant> getHttpRoutes() override {
        return {
            // Public information page
            WebRoute("/info", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getDeviceInfo(), "text/html");
                }, {AuthType::NONE}),
            
            // Main control interface - no auth specified (application can override)
            WebRoute("/", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getControlPage(), "text/html");
                }),
            
            // Configuration API - suggest session auth but allow override
            ApiRoute("/config", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getConfigJSON(), "application/json");
                }, {AuthType::SESSION}),
            
            // Dangerous operations - require authentication
            ApiRoute("/factory-reset", WebModule::WM_POST, 
                [this](WebRequest& req, WebResponse& res) {
                    bool success = performFactoryReset();
                    res.setContent(success ? "{\"success\":true}" : "{\"error\":\"Failed\"}", 
                                 "application/json");
                }, {AuthType::SESSION})
        };
    }
    
    std::vector<RouteVariant> getHttpsRoutes() override {
        return getHttpRoutes(); // Same routes for HTTPS
    }
    
    String getModuleName() const override { return "Device Controller"; }
    
private:
    String getDeviceInfo() { /* return public device info */ }
    String getControlPage() { /* return main control interface */ }
    String getConfigJSON() { /* return configuration data */ }
    bool performFactoryReset() { /* perform reset operation */ }
};
```

### CSRF Protection for Forms

WebPlatform automatically injects CSRF tokens into all HTML pages. Use them in your forms:

```cpp
WebRoute("/config", WebModule::WM_GET, 
    [this](WebRequest& req, WebResponse& res) {
        String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>Device Configuration</title>
            </head><body>
                <form method="post" action="/device/api/update-config">
                    <input type="text" name="setting1" placeholder="Setting 1">
                    <input type="text" name="setting2" placeholder="Setting 2">
                    <button type="submit">Save Configuration</button>
                </form>
                
                <script>
                    // CSRF token is automatically available from injected meta tag
                    document.querySelector('form').addEventListener('submit', function(e) {
                        const csrf = document.querySelector('meta[name="csrf-token"]').content;
                        const input = document.createElement('input');
                        input.type = 'hidden';
                        input.name = '_csrf';
                        input.value = csrf;
                        this.appendChild(input);
                    });
                </script>
            </body></html>
        )";
        res.setContent(html, "text/html");
    }),

ApiRoute("/update-config", WebModule::WM_POST, 
    [this](WebRequest& req, WebResponse& res) {
        // Update configuration
        bool success = updateConfig(req.getBody());
        res.setContent(success ? "{\"success\":true}" : "{\"error\":\"Update failed\"}", 
                     "application/json");
    }, {AuthType::SESSION, AuthType::PAGE_TOKEN})  // Requires both session and CSRF
```

### Combining Authentication and API Documentation

When creating API endpoints that require authentication, combine proper authentication requirements with clear documentation using the memory-efficient macro system:

```cpp
// Wrap documentation class to disappear when OpenAPI disabled
#if OPENAPI_ENABLED
class DeviceApiDocs {
public:
  static const std::vector<String> DEVICE_TAGS;
  
  static OpenAPIDocumentation createGetDeviceStatus() {
    OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Get device status",
      "Returns the current status of the device with authentication details",
      "getDeviceStatus", 
      DEVICE_TAGS
    );
    
    doc.responseExample = R"({
      "status": "online",
      "uptime": 3600,
      "auth_method": "session"
    })";
    
    return doc;
  }
};

// Define tags
const std::vector<String> DeviceApiDocs::DEVICE_TAGS = {"Device"};
#endif // OPENAPI_ENABLED

// Use in API route with authentication and memory-efficient documentation
webPlatform.registerApiRoute("/status", 
  [this](WebRequest& req, WebResponse& res) {
    const AuthContext& auth = req.getAuthContext();
    String authMethod = (auth.authenticatedVia == AuthType::SESSION) ? "session" : "token";
    
    String json = "{\"status\":\"online\",\"uptime\":" + String(millis()/1000) + 
                 ",\"auth_method\":\"" + authMethod + "\"}";
    res.setContent(json, "application/json");
  }, 
  {AuthType::SESSION, AuthType::TOKEN}, 
  WebModule::WM_GET,
  API_DOC_BLOCK(DeviceApiDocs::createGetDeviceStatus())
);

## API Usage Examples

### Using API Tokens

Once tokens are created, they can be used for programmatic access:

```bash
# Using curl with Bearer token
curl -H "Authorization: Bearer tok_abc123def456" \
     https://device.local/api/device/status

# Using curl with URL parameter (alternative)
curl "https://device.local/api/device/status?access_token=tok_abc123def456"

# POST request with token
curl -X POST \
     -H "Authorization: Bearer tok_abc123def456" \
     -H "Content-Type: application/json" \
     -d '{"command":"restart"}' \
     https://device.local/api/device/control
```

### JavaScript API Integration

```javascript
// Store the API token
const API_TOKEN = 'tok_abc123def456';
const BASE_URL = 'https://device.local';

// Helper function for authenticated requests
async function apiRequest(endpoint, options = {}) {
    const config = {
        headers: {
            'Authorization': `Bearer ${API_TOKEN}`,
            'Content-Type': 'application/json',
            ...options.headers
        },
        ...options
    };
    
    const response = await fetch(`${BASE_URL}${endpoint}`, config);
    return await response.json();
}

// Get device status
async function getDeviceStatus() {
    try {
        const status = await apiRequest('/api/device/status');
        console.log('Device status:', status);
    } catch (error) {
        console.error('Error:', error);
    }
}

// Send control command
async function sendCommand(command) {
    try {
        const result = await apiRequest('/api/device/control', {
            method: 'POST',
            body: JSON.stringify({command: command})
        });
        console.log('Command result:', result);
    } catch (error) {
        console.error('Error:', error);
    }
}

// Usage examples
getDeviceStatus();
sendCommand('restart');
```

## Authentication Context

In your route handlers, you can access authentication information:

```cpp
void protectedHandler(WebRequest& req, WebResponse& res) {
    const AuthContext& auth = req.getAuthContext();
    
    // Check how the user authenticated
    if (auth.authenticatedVia == AuthType::SESSION) {
        // Web user logged in with username/password
        String username = auth.username;
        DEBUG_PRINTLN("Web user: " + username);
        
    } else if (auth.authenticatedVia == AuthType::TOKEN) {
        // API access with token
        String tokenId = auth.token;
        DEBUG_PRINTLN("API access with token: " + tokenId);
    }
    
    // Check if user has admin privileges (custom logic)
    bool isAdmin = (auth.username == "admin");
    
    if (isAdmin) {
        res.setContent(getAdminPage(), "text/html");
    } else {
        res.setContent(getRegularUserPage(), "text/html");
    }
}
```

## Best Practices

### Security Guidelines

1. **Use HTTPS**: Enable HTTPS whenever possible for encrypted communication
2. **Protect Sensitive Operations**: Always require authentication for configuration changes
3. **CSRF Protection**: CSRF tokens are automatically available - use `PAGE_TOKEN` for form submissions
4. **Token Security**: Show API tokens only once during creation
5. **Network Restrictions**: Consider `LOCAL_ONLY` for administrative functions

### Route Organization

```cpp
// Good: Clear separation of public and protected routes
WebRoute("/info", handler, {AuthType::NONE})           // Public information
WebRoute("/", handler, {AuthType::NONE})               // Let application decide
ApiRoute("/status", handler, {AuthType::TOKEN})    // API access only
WebRoute("/config", handler, {AuthType::SESSION})      // Web interface only
ApiRoute("/config", handler, {AuthType::SESSION, AuthType::TOKEN})  // Both

// Good: Descriptive route paths
ApiRoute("/factory-reset", resetHandler, {AuthType::SESSION})
ApiRoute("/firmware-update", updateHandler, {AuthType::SESSION})
```

### Error Handling

```cpp
void apiHandler(WebRequest& req, WebResponse& res) {
    const AuthContext& auth = req.getAuthContext();
    
    if (!auth.isAuthenticated) {
        res.setStatus(401);
        res.setContent("{\"error\":\"Authentication required\"}", "application/json");
        return;
    }
    
    // Proceed with authenticated request
    res.setContent(processRequest(req), "application/json");
}
```

## RESTful API Endpoints

WebPlatform provides built-in RESTful API endpoints for user and token management:

### User Management
```bash
# List all users (admin only)
GET /api/users

# Create new user (admin only)
POST /api/users
{"username": "newuser", "password": "password123"}

# Get specific user by ID
GET /api/users/{userId}

# Update user password
PUT /api/users/{userId}
{"password": "newpassword"}

# Delete user (admin only)
DELETE /api/users/{userId}
```

### Current User (Convenience Endpoints)
```bash
# Get current user info
GET /api/user

# Update current user password
PUT /api/user
{"password": "newpassword"}
```

### Token Management
```bash
# Get user's API tokens
GET /api/users/{userId}/tokens

# Create new token for user
POST /api/users/{userId}/tokens
{"name": "My API Token"}

# Delete specific token
DELETE /api/tokens/{tokenId}
```

## Storage Integration

WebPlatform's authentication system uses the built-in storage system with UUID-based user identification:

```cpp
// Authentication storage uses the storage manager
#include "storage/auth_storage.h"

// Create user with automatic UUID assignment
String userId = AuthStorage::createUser("username", "password");

// Find user by ID or username
AuthUser user = AuthStorage::findUserById(userId);
AuthUser user = AuthStorage::findUserByUsername("admin");

// Token management with user IDs
String token = AuthStorage::createApiToken(userId, "Token Name");
std::vector<AuthApiToken> tokens = AuthStorage::getUserApiTokens(userId);
```

## Default Credentials

WebPlatform creates a default admin account on first boot:

- **Username**: `admin`
- **Password**: `admin`

**Important**: Change these credentials in production deployments through the `/account` page or API endpoints!

## Security Considerations

- Self-signed certificates will show browser warnings - use proper CA-signed certificates for production
- API tokens are shown only once during creation - save them securely
- Sessions expire automatically for security
- CSRF protection is built-in for form submissions
- All user data is stored using UUID primary keys for better security and scalability

The authentication system provides comprehensive security features while remaining flexible enough for various use cases. Application developers can choose the level of security appropriate for their needs, while module developers can suggest reasonable defaults that can be overridden when necessary.