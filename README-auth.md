# WebPlatform Authentication Guide

This document explains how to implement and use the authentication system in WebPlatform applications.

## Overview

WebPlatform provides a comprehensive authentication system that supports multiple authentication methods and can be applied at the route level. This allows for flexible security implementations ranging from simple login systems to complex API access controls.

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
#include <my_device_module.h>

void setup() {
    Serial.begin(115200);
    
    // Set up navigation with logout option
    std::vector<NavigationItem> navItems = {
        NavigationItem("Dashboard", "/"),
        NavigationItem("Device Control", "/device/"),
        NavigationItem("Settings", "/settings"),
        NavigationItem("Logout", "/logout")
    };
    IWebModule::setNavigationMenu(navItems);
    
    webPlatform.begin("SecureDevice");
    
    if (webPlatform.isConnected()) {
        // Register your modules
        webPlatform.registerModule("/device", &myDeviceModule);
        
        // Protect the main dashboard
        webPlatform.registerRoute("/", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <html><head><title>Dashboard</title></head><body>
                <h1>Welcome, )" + req.getAuthContext().username + R"(!</h1>
                <p>You are successfully logged in.</p>
                <ul>
                    <li><a href="/device/">Device Control</a></li>
                    <li><a href="/settings">Settings</a></li>
                    <li><a href="/account">Manage Account</a></li>
                </ul>
                </body></html>
            )";
            res.setContent(html, "text/html");
        }, {AuthType::SESSION});
        
        // Override module routes to add authentication
        webPlatform.overrideRoute("/device/", deviceDashboardHandler, {AuthType::SESSION});
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
        webPlatform.registerRoute("/api-tokens", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head>
                    <title>API Token Management</title>
                    <meta name="csrf-token" content="{{csrfToken}}">
                    <script>
                        async function createToken() {
                            const description = document.getElementById('description').value;
                            const csrf = document.querySelector('meta[name="csrf-token"]').content;
                            
                            const response = await fetch('/api/tokens', {
                                method: 'POST',
                                headers: {
                                    'Content-Type': 'application/json',
                                    'X-CSRF-Token': csrf
                                },
                                body: JSON.stringify({description: description})
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
        webPlatform.registerRoute("/api/device/status", [](WebRequest& req, WebResponse& res) {
            // This route can be accessed via web interface OR API token
            res.setContent(myDeviceModule.getStatusJSON(), "application/json");
        }, {AuthType::SESSION, AuthType::TOKEN});
        
        webPlatform.registerRoute("/api/device/control", [](WebRequest& req, WebResponse& res) {
            if (req.getMethod() != WebModule::WM_POST) {
                res.setStatus(405);
                return;
            }
            
            String command = req.getParam("command");
            bool success = myDeviceModule.executeCommand(command);
            
            String result = success ? 
                "{\"success\":true}" : 
                "{\"success\":false,\"error\":\"Command failed\"}";
            res.setContent(result, "application/json");
        }, {AuthType::SESSION, AuthType::TOKEN}, WebModule::WM_POST);
    }
}
```

### Custom Login Page

Override the default login page with your own branding:

```cpp
void setup() {
    // ... basic setup ...
    
    if (webPlatform.isConnected()) {
        // Override the default login page
        webPlatform.overrideRoute("/login", [](WebRequest& req, WebResponse& res) {
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
    std::vector<WebRoute> getHttpRoutes() override {
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
            WebRoute("/api/config", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getConfigJSON(), "application/json");
                }, {AuthType::SESSION}),
            
            // Dangerous operations - require authentication
            WebRoute("/api/factory-reset", WebModule::WM_POST, 
                [this](WebRequest& req, WebResponse& res) {
                    bool success = performFactoryReset();
                    res.setContent(success ? "{\"success\":true}" : "{\"error\":\"Failed\"}", 
                                 "application/json");
                }, {AuthType::SESSION})
        };
    }
    
    std::vector<WebRoute> getHttpsRoutes() override {
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

When creating forms in your modules, include CSRF protection:

```cpp
WebRoute("/config", WebModule::WM_GET, 
    [this](WebRequest& req, WebResponse& res) {
        String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>Device Configuration</title>
                <meta name="csrf-token" content="{{csrfToken}}">
            </head><body>
                <form method="post" action="/device/api/update-config">
                    <input type="text" name="setting1" placeholder="Setting 1">
                    <input type="text" name="setting2" placeholder="Setting 2">
                    <button type="submit">Save Configuration</button>
                </form>
                
                <script>
                    // Add CSRF token to form submissions
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

WebRoute("/api/update-config", WebModule::WM_POST, 
    [this](WebRequest& req, WebResponse& res) {
        // Update configuration
        bool success = updateConfig(req.getBody());
        res.setContent(success ? "{\"success\":true}" : "{\"error\":\"Update failed\"}", 
                     "application/json");
    }, {AuthType::SESSION, AuthType::PAGE_TOKEN})  // Requires both session and CSRF
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
        Serial.println("Web user: " + username);
        
    } else if (auth.authenticatedVia == AuthType::TOKEN) {
        // API access with token
        String tokenId = auth.token;
        Serial.println("API access with token: " + tokenId);
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
3. **CSRF Protection**: Use `PAGE_TOKEN` for all form submissions
4. **Token Security**: Show API tokens only once during creation
5. **Network Restrictions**: Consider `LOCAL_ONLY` for administrative functions

### Route Organization

```cpp
// Good: Clear separation of public and protected routes
WebRoute("/info", handler, {AuthType::NONE})           // Public information
WebRoute("/", handler, {AuthType::NONE})               // Let application decide
WebRoute("/api/status", handler, {AuthType::TOKEN})    // API access only
WebRoute("/config", handler, {AuthType::SESSION})      // Web interface only
WebRoute("/api/config", handler, {AuthType::SESSION, AuthType::TOKEN})  // Both

// Good: Descriptive route paths
WebRoute("/api/factory-reset", resetHandler, {AuthType::SESSION})
WebRoute("/api/firmware-update", updateHandler, {AuthType::SESSION})
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

## Default Credentials

WebPlatform creates a default admin account on first boot:

- **Username**: `admin`
- **Password**: `admin`

**Important**: Change these credentials in production deployments!

The authentication system provides comprehensive security features while remaining flexible enough for various use cases. Application developers can choose the level of security appropriate for their needs, while module developers can suggest reasonable defaults that can be overridden when necessary.