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
    webPlatform.setNavigationMenu(navItems);
    
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

When creating API endpoints that require authentication, combine proper authentication requirements with clear documentation using the memory-efficient macro system. Consider whether routes should be included in the Maker API for external consumption:

```cpp
// Wrap documentation class to disappear when OpenAPI disabled
#if OPENAPI_ENABLED
class DeviceApiDocs {
public:
  static const std::vector<String> DEVICE_TAGS;
  static const std::vector<String> MAKER_DEVICE_TAGS;  // Tags for maker-friendly routes
  
  static OpenAPIDocumentation createGetDeviceStatus() {
    OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Get device status",
      "Returns the current status of the device with authentication details",
      "getDeviceStatus", 
      MAKER_DEVICE_TAGS  // Include "maker" tag for public API
    );
    
    doc.responseExample = R"({
      "status": "online",
      "uptime": 3600,
      "auth_method": "token"
    })";
    
    return doc;
  }
  
  static OpenAPIDocumentation createGetAdminStatus() {
    OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Get admin status",
      "Returns detailed admin status (internal use only)",
      "getAdminStatus", 
      DEVICE_TAGS  // No "maker" tag - internal only
    );
    
    doc.responseExample = R"({
      "status": "online",
      "uptime": 3600,
      "auth_method": "session",
      "internal_metrics": {...}
    })";
    
    return doc;
  }
};

// Define tags
const std::vector<String> DeviceApiDocs::DEVICE_TAGS = {"Device"};
const std::vector<String> DeviceApiDocs::MAKER_DEVICE_TAGS = {"Device", "maker"};  // Public API
#endif // OPENAPI_ENABLED

// Public API route - appears in both Full API and Maker API
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

// Internal API route - appears only in Full API (no maker tag)
webPlatform.registerApiRoute("/admin/status", 
  [this](WebRequest& req, WebResponse& res) {
    // Include sensitive internal metrics
    String json = getDetailedAdminStatus();  // Implementation with internal details
    res.setContent(json, "application/json");
  }, 
  {AuthType::SESSION},  // Session-only for admin interface
  WebModule::WM_GET,
  API_DOC_BLOCK(DeviceApiDocs::createGetAdminStatus())
);
```

### API Documentation Strategy for Authentication

When designing authenticated APIs, consider the dual OpenAPI system:

1. **Public/Maker APIs** (`/maker/openapi.json`):
   - Tag with "maker" or configured public tags
   - Use token authentication for API access
   - Provide clean, focused documentation
   - Hide internal implementation details

2. **Internal APIs** (full `/openapi.json`):
   - No maker tags - internal development only
   - Can use session authentication for admin interfaces
   - Include detailed implementation information
   - Document admin-only functionality

```cpp
// Example: Public sensor reading API
webPlatform.registerApiRoute("/sensor/reading", handler, {AuthType::TOKEN}, WebModule::WM_GET,
    API_DOC("Get sensor reading", "Returns current sensor value", "getSensorReading", 
            {"maker", "sensor"}));  // Public API

// Example: Internal sensor calibration API  
webPlatform.registerApiRoute("/admin/sensor/calibrate", handler, {AuthType::SESSION}, WebModule::WM_POST,
    API_DOC("Calibrate sensor", "Internal sensor calibration", "calibrateSensor", 
            {"admin", "sensor"}));  // Internal only
```

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

// Get device status (public API - available in Maker API spec)
async function getDeviceStatus() {
    try {
        const status = await apiRequest('/api/device/status');
        console.log('Device status:', status);
    } catch (error) {
        console.error('Error:', error);
    }
}

// Send control command (public API - available in Maker API spec)
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

// Admin function (internal API - NOT in Maker API spec)
async function performFactoryReset() {
    try {
        const result = await apiRequest('/api/admin/factory-reset', {
            method: 'POST'
        });
        console.log('Factory reset result:', result);
    } catch (error) {
        console.error('Error:', error);
    }
}

// Usage examples
getDeviceStatus();    // Public API
sendCommand('restart'); // Public API
// performFactoryReset(); // Admin API - typically not exposed to external developers
```

### API Discovery Using OpenAPI Specifications

```javascript
// Discover available public APIs using Maker API spec
async function discoverPublicAPIs() {
    try {
        const spec = await fetch('/maker/openapi.json');
        const openapi = await spec.json();
        
        console.log('Available public APIs:', Object.keys(openapi.paths));
        console.log('API Server:', openapi.servers[0].url);
        console.log('API Title:', openapi.info.title);
        
        // Extract available endpoints
        const endpoints = [];
        for (const [path, methods] of Object.entries(openapi.paths)) {
            for (const [method, details] of Object.entries(methods)) {
                endpoints.push({
                    path: path,
                    method: method.toUpperCase(),
                    summary: details.summary,
                    description: details.description
                });
            }
        }
        
        return endpoints;
    } catch (error) {
        console.error('Failed to discover APIs:', error);
        return [];
    }
}

// Use API discovery
discoverPublicAPIs().then(endpoints => {
    console.log('Public API Endpoints:', endpoints);
});
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
    
    // Check if user has admin privileges
    bool isAdmin = auth.isAdmin; // Use the built-in isAdmin flag
    
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

WebPlatform provides built-in RESTful API endpoints for user and token management. These endpoints appear in different OpenAPI specifications based on their intended audience:

### User Management (Internal APIs - Full OpenAPI Only)
```bash
# List all users (admin only) - NOT in Maker API
GET /api/users

# Create new user (admin only) - NOT in Maker API
POST /api/users
{"username": "newuser", "password": "password123"}

# Get specific user by ID - NOT in Maker API
GET /api/users/{userId}

# Update user password - NOT in Maker API
PUT /api/users/{userId}
{"password": "newpassword"}

# Delete user (admin only) - NOT in Maker API
DELETE /api/users/{userId}
```

### Current User (Convenience Endpoints - Internal APIs Only)
```bash
# Get current user info - NOT in Maker API
GET /api/user

# Update current user password - NOT in Maker API
PUT /api/user
{"password": "newpassword"}
```

### Token Management (Mixed - Some in Maker API)
```bash
# Get user's API tokens - NOT in Maker API (admin function)
GET /api/users/{userId}/tokens

# Create new token for user - NOT in Maker API (admin function)
POST /api/users/{userId}/tokens
{"name": "My API Token"}

# Delete specific token - Could be in Maker API if tagged
DELETE /api/tokens/{tokenId}
```

### System Information (Public APIs - Available in Maker API)
```bash
# Get system status - Available in Maker API
GET /api/system/status

# Get network status - Available in Maker API
GET /api/system/network

# Get available modules - Available in Maker API
GET /api/system/modules
```

### OpenAPI Specifications
```bash
# Full API specification (all endpoints)
GET /openapi.json

# Maker API specification (public endpoints only)
GET /maker/openapi.json
```

**Note**: The built-in user management endpoints are considered internal administrative functions and do not appear in the Maker API specification by default. Public-facing devices should implement their own maker-friendly user management APIs if needed for external consumption.

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

## Initial Setup Flow

WebPlatform implements a secure initial setup flow on first boot:

1. When first accessing the device, users are redirected to a setup page
2. Users create their own administrator account with a custom username and password
3. This account becomes the primary administrator for the device
4. No default credentials exist - security is ensured from first boot

The setup page guides users through creating a secure administrator account with proper validation for username and password requirements.

## Security Considerations

- Self-signed certificates will show browser warnings - use proper CA-signed certificates for production
- API tokens are shown only once during creation - save them securely
- Sessions expire automatically for security
- CSRF protection is built-in for form submissions
- All user data is stored using UUID primary keys for better security and scalability

The authentication system provides comprehensive security features while remaining flexible enough for various use cases. Application developers can choose the level of security appropriate for their needs, while module developers can suggest reasonable defaults that can be overridden when necessary.

The dual OpenAPI system allows for clear separation between internal administrative APIs (full specification) and public/maker-friendly APIs (Maker specification), ensuring that sensitive management functions remain internal while providing clean, focused documentation for external integrations.
