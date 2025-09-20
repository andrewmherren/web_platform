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
webPlatform.registerWebRoute("/public", handler, {AuthType::NONE});

// Login required
webPlatform.registerWebRoute("/admin", handler, {AuthType::SESSION});

// API access
webPlatform.registerApiRoute("/data", handler, {AuthType::TOKEN});

// Form with CSRF protection
webPlatform.registerApiRoute("/save", handler, 
    {AuthType::SESSION, AuthType::PAGE_TOKEN}, WebModule::WM_POST);

// Flexible access (either session or token)
webPlatform.registerApiRoute("/status", handler, 
    {AuthType::SESSION, AuthType::TOKEN});
```

### Default Credentials

WebPlatform creates a default admin account on first boot:

- **Username**: `admin`
- **Password**: `admin`

**Important**: Change these credentials in production deployments through the `/account` page or API endpoints!

## Route Management

WebPlatform provides a unified method for managing routes:

### Register a Standard Web Route
```cpp
webPlatform.registerWebRoute("/path", [](WebRequest& req, WebResponse& res) {
    res.setContent("Hello World", "text/html");
}, {AuthType::NONE});
```

### Register an API Route with OpenAPI Documentation
```cpp
// First, create a documentation factory class
class SystemApiDocs {
public:
  static const std::vector<String> SYSTEM_TAGS;
  
  static OpenAPIDocumentation createGetSystemStatus() {
    OpenAPIDocumentation doc = OpenAPIFactory::create(
        "Get system status",
        "Returns the current system status",
        "getSystemStatus",
        SYSTEM_TAGS
    );
    
    doc.responseExample = R"({
      "status": "ok",
      "uptime": 3600,
      "memory": 40000
    })";
    
    doc.responseSchema = R"({
      "type": "object",
      "properties": {
        "status": {"type": "string", "description": "System status"},
        "uptime": {"type": "number", "description": "Uptime in seconds"},
        "memory": {"type": "number", "description": "Free memory in bytes"}
      }
    })";
    
    return doc;
  }
};

// Define tags
const std::vector<String> SystemApiDocs::SYSTEM_TAGS = {"System"};

// Then use the factory in your route registration
webPlatform.registerApiRoute(
    "/status", 
    [](WebRequest& req, WebResponse& res) {
        res.setContent("{\"status\":\"ok\"}", "application/json");
    }, 
    {AuthType::TOKEN}, 
    WebModule::WM_GET,
    SystemApiDocs::createGetSystemStatus()
);
```

## OpenAPIFactory

The WebPlatform library provides an `OpenAPIFactory` class that simplifies creating documentation:

```cpp
// Create basic documentation
OpenAPIDocumentation doc = OpenAPIFactory::create(
    "Create user",                      // Summary
    "Creates a new user in the system", // Description
    "createUser",                       // Operation ID
    {"User Management"}                 // Tags
);

// Create standard success response
String schema = OpenAPIFactory::createSuccessResponse("User created successfully");

// Create standard error response
String errorSchema = OpenAPIFactory::createErrorResponse("Error description");

// Create list response
String listSchema = OpenAPIFactory::createListResponse("users");

// Create string request
String stringSchema = OpenAPIFactory::createStringRequest("API token to validate", 10);

// Generate operation ID using factory utility
String operationId = OpenAPIFactory::generateOperationId("create", "user");

### Replace an Existing Module Route
```cpp
// Simply register again with the same path to replace the handler
webPlatform.registerWebRoute("/module/path", [](WebRequest& req, WebResponse& res) {
    res.setContent("Custom implementation", "text/html");
}, {AuthType::SESSION});
```

### Disable a Route
```cpp
// Register with nullptr as the handler to disable a route
webPlatform.registerWebRoute("/path/to/disable", nullptr, {AuthType::NONE});
```

### OpenAPI Documentation Structure
The `OpenAPIDocumentation` object lets you document your API endpoints for better discoverability:

```cpp
// Basic documentation with just a summary
OpenAPIDocumentation docs(
    "Get device status"                     // Summary
);

// With description and operation ID
OpenAPIDocumentation docs(
    "Get device status",                    // Summary
    "Returns detailed device information",  // Description
    "getDeviceStatus"                       // Operation ID
);

// With tags for grouping in documentation tools
OpenAPIDocumentation docs(
    "Get device status",
    "Returns detailed device information",
    "getDeviceStatus",
    {"Device", "System"}                    // Tags
);

// Advanced documentation with examples
OpenAPIDocumentation docs("Create user");
docs.requestExample = "{\"username\":\"john\",\"email\":\"john@example.com\"}";
docs.responseExample = "{\"id\":\"123\",\"username\":\"john\"}";
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

## OpenAPI Integration

WebPlatform includes built-in support for OpenAPI 3.0 specification generation, making it easy to document your API endpoints.

### Accessing the OpenAPI Specification

The OpenAPI specification is automatically generated and available at:
```
/api/openapi.json
```

You can also filter the specification by authentication type:
```
/api/openapi.json?filter=token  # Only show token-authenticated routes
/api/openapi.json?filter=session  # Only show session-authenticated routes
```

### Benefits of Using OpenAPI Documentation

1. **Automatic Documentation**: Your API endpoints are automatically documented based on the information you provide
2. **Tool Integration**: The generated specification can be imported into tools like Postman, Swagger UI, or code generators
3. **Self-documenting APIs**: New developers can quickly understand your API without reading code
4. **Testing Support**: Tools can generate test cases based on your API documentation

### Example: API Documentation with OpenAPI

See the complete [OpenAPI Example](examples/openapi_example.cpp) for a working demonstration.

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
        webPlatform.registerWebRoute("/", [](WebRequest& req, WebResponse& res) {
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
        
        webPlatform.registerWebRoute("/about", [](WebRequest& req, WebResponse& res) {
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
        webPlatform.registerWebRoute("/info", [](WebRequest& req, WebResponse& res) {
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
        webPlatform.registerWebRoute("/", [](WebRequest& req, WebResponse& res) {
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
        webPlatform.registerApiRoute("/data", [](WebRequest& req, WebResponse& res) {
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
1. **Use PROGMEM Streaming**: Always use `setProgmemContent()` for embedded assets
2. **Minimize Dynamic HTML**: Pre-generate HTML strings where possible
3. **Efficient Route Structure**: Organize routes logically to minimize search time

### Asset Management
```cpp
// Correct: Use setProgmemContent for all embedded assets
res.setProgmemContent(STYLESHEET_CSS, "text/css");
res.setProgmemContent(SCRIPT_JS, "application/javascript");
res.setProgmemContent(PAGE_HTML, "text/html");

// Only use setContent for dynamic content
res.setContent(jsonResponse, "application/json");
res.setContent(dynamicHtml, "text/html");
```

### Development
1. **Clear Error Messages**: Provide useful error messages and status codes
2. **Consistent Route Structure**: Use consistent URL patterns
3. **Document API Endpoints**: Use OpenAPIDocumentation to properly document your APIs
4. **Progressive Enhancement**: Ensure basic functionality works without JavaScript
5. **Test on Both Platforms**: Verify on ESP32 device

### API Documentation
1. **Use Documentation Classes**: Create dedicated classes for API documentation using the pattern `ModuleNameDocs`
2. **Organize by Feature**: Group related API documentation methods in the same class
3. **Use Factory Methods**: Create static factory methods that return complete `OpenAPIDocumentation` objects
4. **Follow Naming Conventions**: Use method names like `createEndpointName()` for documentation factories
5. **Separate from Implementation**: Keep documentation separate from implementation code
