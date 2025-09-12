# WebPlatform User Guide

This guide provides detailed usage instructions, examples, and best practices for using the WebPlatform library.

## Table of Contents
- [Core Concepts](#core-concepts)
- [Platform Modes](#platform-modes)
- [Authentication System](#authentication-system)
- [Route Management](#route-management)
- [Template System](#template-system)
- [Storage System](#storage-system)
- [HTTPS Configuration](#https-configuration)
- [Navigation Menu](#navigation-menu)
- [Examples](#examples)
- [Best Practices](#best-practices)

## Core Concepts

WebPlatform combines multiple components into a single, easy-to-use library:

### Unified Architecture
- **Single Server Instance**: HTTP or HTTPS automatically selected based on certificate availability
- **Mode-Based Operation**: Automatic switching between WiFi configuration and application modes
- **Module System**: Register multiple web modules with isolated route namespaces
- **Static Assets**: Built-in CSS framework and JavaScript utilities

### Component Overview
- **WiFi Management**: Configuration portal with captive portal
- **Authentication System**: Session-based and token-based authentication
- **Route Management**: Register, override, and disable routes
- **Template System**: HTML templates with automatic bookmark replacement
- **Storage System**: Flexible database drivers with query builder

## Platform Modes

### CONFIG_PORTAL Mode
Activated when no WiFi credentials are stored or connection fails.

- Creates WiFi access point (`[DeviceName]Setup`)
- Serves captive portal for WiFi configuration  
- Uses HTTPS if certificates available
- Automatically transitions to CONNECTED mode after setup

### CONNECTED Mode  
Activated when device successfully connects to WiFi.

- Serves registered application modules
- Provides full authentication system
- Enables advanced features like API tokens
- Supports mDNS hostname resolution (`device.local`)

## Authentication System

WebPlatform provides a comprehensive authentication system with multiple authentication methods.

### Authentication Types
- `AuthType::NONE` - Public access (no authentication required)
- `AuthType::SESSION` - Cookie-based session authentication (web UI)
- `AuthType::TOKEN` - Bearer token authentication (API access)
- `AuthType::PAGE_TOKEN` - CSRF protection for form submissions
- `AuthType::LOCAL_ONLY` - Restrict access to local network only

### Route Protection Examples

```cpp
// Public route
webPlatform.registerRoute("/public", handler, {AuthType::NONE});

// Login required
webPlatform.registerRoute("/admin", handler, {AuthType::SESSION});

// API access
webPlatform.registerRoute("/api/data", handler, {AuthType::TOKEN});

// Form with CSRF protection
webPlatform.registerRoute("/api/save", handler, 
    {AuthType::SESSION, AuthType::PAGE_TOKEN}, WebModule::WM_POST);

// Flexible access (either session or token)
webPlatform.registerRoute("/api/status", handler, 
    {AuthType::SESSION, AuthType::TOKEN});
```

### Default Credentials

WebPlatform creates a default admin account on first boot:

- **Username**: `admin`
- **Password**: `admin`

**Important**: Change these credentials in production deployments through the `/account` page or API endpoints!

## Route Management

WebPlatform provides several methods for managing routes:

### Register a Route
```cpp
webPlatform.registerRoute("/path", [](WebRequest& req, WebResponse& res) {
    res.setContent("Hello World", "text/html");
}, {AuthType::NONE});
```

### Override a Module Route
```cpp
webPlatform.overrideRoute("/module/path", [](WebRequest& req, WebResponse& res) {
    res.setContent("Custom implementation", "text/html");
}, {AuthType::SESSION});
```

### Disable a Route
```cpp
webPlatform.disableRoute("/path/to/disable");
```

## Template System

WebPlatform includes an automatic template processing system that replaces bookmarks in HTML content.

### Supported Bookmarks
- `{{NAV_MENU}}` - Injects the navigation menu
- `{{csrfToken}}` - Generates and injects a CSRF token for form protection
- `{{SECURITY_NOTICE}}` - Displays HTTPS status notice for sensitive forms
- `{{username}}` - Shows the authenticated user's username
- `{{DEVICE_NAME}}` - Shows the device name set during initialization

### Example Usage
```cpp
String html = R"(
  <!DOCTYPE html>
  <html><head><title>Example</title></head>
  <body>
    <div class="container">
      {{NAV_MENU}}
      <h1>Welcome, {{username}}!</h1>
      <p>Device: {{DEVICE_NAME}}</p>
    </div>
  </body></html>
)";

res.setContent(html, "text/html");  // Bookmarks processed automatically!
```

### Opt-Out of Template Processing
```cpp
// Disable template processing for a specific response
res.setHeader("X-Skip-Template-Processing", "true");
res.setContent(html, "text/html");
```

## Storage System

WebPlatform includes a flexible storage system inspired by Laravel's database architecture:

### Basic Usage
```cpp
// Store user data
StorageManager::query("users")
  ->store("user1", userObject.toJson());

// Query with conditions
String userData = StorageManager::query("users")
  ->where("username", "admin")
  ->get();

// Use different storage drivers
StorageManager::driver("cloud")
  ->query("audit_logs")
  ->store("log1", logData);
```

### Data Models
Built-in models with automatic JSON serialization:
- `AuthUser`: User accounts with UUID primary keys
- `AuthSession`: Session management
- `AuthApiToken`: API token authentication
- `ConfigItem`: Configuration storage

## HTTPS Configuration

WebPlatform provides both HTTP and HTTPS support with automatic certificate detection:

### Certificate Files
1. Generate or obtain SSL certificate files
2. Place certificate files in the `src` directory:
   - `server_cert.pem` - Server certificate
   - `server_key.pem` - Private key
3. Configure `platformio.ini` to embed certificate files (see [Build Configuration](#build-configuration))

### Generating SSL Certificates

```bash
# Generate private key
openssl genrsa -out src/server_key.pem 2048

# Generate certificate signing request (follow prompts for certificate details)
openssl req -new -key src/server_key.pem -out src/server_csr.pem

# Generate self-signed certificate (valid for 365 days)
openssl x509 -req -days 365 -in src/server_csr.pem -signkey src/server_key.pem -out src/server_cert.pem

# Clean up temporary CSR file (optional)
rm src/server_csr.pem
```

## Navigation Menu

Create responsive navigation menus that adapt to user authentication state:

```cpp
std::vector<NavigationItem> navItems = {
    NavigationItem("Dashboard", "/"),
    NavigationItem("Settings", "/config/"),
    NavigationItem("API Docs", "/docs", "_blank"),  // Opens in new tab
    Authenticated(NavigationItem("Account", "/account")),
    Authenticated(NavigationItem("Logout", "/logout")),
    Unauthenticated(NavigationItem("Login", "/login"))
};
IWebModule::setNavigationMenu(navItems);
```

**Authentication-aware Navigation Items**:
- `Authenticated(NavigationItem(...))` - Only shows when user has valid session
- `Unauthenticated(NavigationItem(...))` - Only shows when user is not logged in
- Regular `NavigationItem(...)` - Always visible regardless of auth state

## Examples

### Basic Application
```cpp
#include <web_platform.h>

void setup() {
    Serial.begin(115200);
    
    // Set up navigation menu
    std::vector<NavigationItem> navItems = {
        NavigationItem("Home", "/"),
        NavigationItem("Settings", "/settings"),
        NavigationItem("About", "/about")
    };
    IWebModule::setNavigationMenu(navItems);
    
    // Initialize WebPlatform
    webPlatform.begin("MyDevice");
    
    if (webPlatform.isConnected()) {
        // Add custom routes
        webPlatform.registerRoute("/", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>Home</title></head>
                <body>
                <div class="container">
                    {{NAV_MENU}}
                    <h1>Welcome to My Device</h1>
                    <p>This is a basic WebPlatform application.</p>
                </div>
                </body></html>
            )";
            res.setContent(html, "text/html");
        });
        
        webPlatform.registerRoute("/about", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>About</title></head>
                <body>
                <div class="container">
                    {{NAV_MENU}}
                    <h1>About</h1>
                    <p>This device runs on WebPlatform.</p>
                </div>
                </body></html>
            )";
            res.setContent(html, "text/html");
        });
    }
}

void loop() {
    webPlatform.handle();
}
```

### Authenticated Application
```cpp
#include <web_platform.h>

void setup() {
    Serial.begin(115200);
    
    // Set up authentication-aware navigation
    std::vector<NavigationItem> navItems = {
        NavigationItem("Dashboard", "/"),
        NavigationItem("Public Info", "/info"),
        Authenticated(NavigationItem("Account", "/account")),
        Authenticated(NavigationItem("Logout", "/logout")),
        Unauthenticated(NavigationItem("Login", "/login"))
    };
    IWebModule::setNavigationMenu(navItems);
    
    // Initialize WebPlatform
    webPlatform.begin("SecureDevice");
    
    if (webPlatform.isConnected()) {
        // Public route
        webPlatform.registerRoute("/info", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>Public Info</title></head>
                <body>
                <div class="container">
                    {{NAV_MENU}}
                    <h1>Public Information</h1>
                    <p>This page is accessible to everyone.</p>
                </div>
                </body></html>
            )";
            res.setContent(html, "text/html");
        }, {AuthType::NONE});
        
        // Protected dashboard
        webPlatform.registerRoute("/", [](WebRequest& req, WebResponse& res) {
            const AuthContext& auth = req.getAuthContext();
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>Dashboard</title></head>
                <body>
                <div class="container">
                    {{NAV_MENU}}
                    <h1>Welcome, )" + auth.username + R"(!</h1>
                    <p>This is your secure dashboard.</p>
                </div>
                </body></html>
            )";
            res.setContent(html, "text/html");
        }, {AuthType::SESSION});
        
        // API endpoint
        webPlatform.registerRoute("/api/data", [](WebRequest& req, WebResponse& res) {
            String json = "{\"status\":\"ok\",\"data\":123}";
            res.setContent(json, "application/json");
        }, {AuthType::TOKEN});
    }
}

void loop() {
    webPlatform.handle();
}
```

## Best Practices

### Security
1. **Change Default Credentials**: Always change `admin`/`admin` in production
2. **Use HTTPS**: Enable HTTPS with proper certificates when possible
3. **Protect Sensitive Routes**: Use `{AuthType::SESSION}` for admin functions
4. **CSRF Protection**: Use `{AuthType::PAGE_TOKEN}` for state-changing forms
5. **API Security**: Use `{AuthType::TOKEN}` for programmatic access

### Performance
1. **Minimize Dynamic HTML**: Pre-generate HTML strings where possible
2. **Use Static Assets**: Serve CSS/JS from embedded assets
3. **Efficient Route Structure**: Organize routes logically to minimize search time

### Development
1. **Clear Error Messages**: Provide useful error messages and status codes
2. **Consistent Route Structure**: Use consistent URL patterns
3. **Progressive Enhancement**: Ensure basic functionality works without JavaScript
4. **Test on Both Platforms**: Verify on both ESP32 and ESP8266