# WebPlatform Library

A comprehensive web development platform for ESP32/ESP8266 that provides a unified framework for building web-enabled embedded applications.

## Overview

WebPlatform is a complete web development framework that transforms ESP32/ESP8266 devices into sophisticated web-enabled applications. It combines multiple components into a single, easy-to-use library:

- **Modular Architecture**: Build reusable web modules using the IWebModule interface
- **Unified HTTP/HTTPS Server**: Single server instance with automatic HTTPS detection
- **WiFi Management**: Captive portal for configuration and automatic connection handling
- **Advanced Authentication**: Session-based and token-based authentication with CSRF protection
- **Storage System**: Flexible database drivers with Laravel-inspired query builder
- **Route Management**: Advanced route handling with override and disable capabilities
- **Asset Management**: Static asset serving with built-in CSS/JS framework

## Target Audiences

### Application Developers
Build complete embedded web applications by leveraging the WebPlatform and optional web modules. The platform handles WiFi setup, authentication, HTTPS, and provides a solid foundation for your device's web interface.

### Module Developers
Create reusable web modules using the IWebModule interface that can be shared across projects. Modules can define their own routes, authentication requirements, and UI components while integrating seamlessly with any WebPlatform application.

## Quick Start

### For Application Developers

```cpp
#include <web_platform.h>
// #include <some_module.h>  // Include your web modules

void setup() {
    Serial.begin(115200);
    
    // Set up navigation menu
    std::vector<NavigationItem> navItems = {
        NavigationItem("Home", "/"),
        NavigationItem("Settings", "/settings/"),
        NavigationItem("About", "/about")
    };
    IWebModule::setNavigationMenu(navItems);

    // Initialize WebPlatform (auto-detects HTTPS capability)
    webPlatform.begin("MyDevice");
    
    if (webPlatform.isConnected()) {
        // Register web modules
        // webPlatform.registerModule("/device", &deviceModule);
        
        // Add custom application routes
        webPlatform.registerRoute("/about", [](WebRequest& req, WebResponse& res) {
            String html = "<h1>About My Device</h1><p>Built with WebPlatform</p>";
            res.setContent(html, "text/html");
        }, {AuthType::SESSION});  // Protect with login
        
        // Override module routes if needed
        webPlatform.overrideRoute("/", customHomeHandler, {AuthType::SESSION});
    }
}

void loop() {
    webPlatform.handle();
    
    if (webPlatform.isConnected()) {
        // Handle registered modules
        // deviceModule.handle();
    }
}
```

### For Module Developers

```cpp
#include <web_platform.h>

class MyModule : public IWebModule {
public:
    std::vector<WebRoute> getHttpRoutes() override {
        return {
            WebRoute("/", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getMainPage(), "text/html");
                }),
            WebRoute("/api/data", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getDataJSON(), "application/json");
                }, {AuthType::TOKEN})  // API requires token authentication
        };
    }
    
    std::vector<WebRoute> getHttpsRoutes() override {
        return getHttpRoutes(); // Same routes for HTTP and HTTPS
    }
    
    String getModuleName() const override { 
        return "My Custom Module"; 
    }
    
private:
    String getMainPage() { /* return HTML content */ }
    String getDataJSON() { /* return JSON data */ }
};
```

## Key Features

### Unified Architecture
- **Single Server Instance**: HTTP or HTTPS automatically selected based on certificate availability
- **Mode-Based Operation**: Automatic switching between WiFi configuration and application modes
- **Module System**: Register multiple web modules with isolated route namespaces
- **Static Assets**: Built-in CSS framework and JavaScript utilities

### Security & Authentication
- **HTTPS Auto-Detection**: Uses HTTPS when certificates are available, falls back to HTTP
- **Authentication Types**: Support for session-based and token-based authentication
- **CSRF Protection**: Built-in protection for form submissions
- **Route-Level Security**: Fine-grained authentication requirements per route

### Advanced Route Management
- **Route Overrides**: Replace module routes with custom implementations
- **Route Disabling**: Completely disable specific module routes
- **Authentication Requirements**: Specify different auth types per route
- **Flexible Handlers**: Both legacy and modern request/response handlers supported

## Operating Modes

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

### Authentication Types
- `AuthType::NONE` - Public access (no authentication)
- `AuthType::SESSION` - Web-based login (cookies)
- `AuthType::TOKEN` - API access (Bearer tokens)
- `AuthType::PAGE_TOKEN` - CSRF protection for forms
- `AuthType::LOCAL_ONLY` - Restrict to local network access

### Route Protection
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

## API Reference

### WebPlatform Core Methods

```cpp
// Initialization
void begin(const char* deviceName = "Device", bool forceHttpsOnly = false);

// Module management
bool registerModule(const char* basePath, IWebModule* module);

// Route management
void registerRoute(const String& path, WebModule::UnifiedRouteHandler handler,
                  const AuthRequirements& auth = {AuthType::NONE},
                  WebModule::Method method = WebModule::WM_GET);
                  
void overrideRoute(const String& path, WebModule::UnifiedRouteHandler handler,
                  const AuthRequirements& auth = {AuthType::NONE},
                  WebModule::Method method = WebModule::WM_GET);
                  
void disableRoute(const String& path, WebModule::Method method = WebModule::WM_GET);

// Request handling
void handle();

// State queries  
bool isConnected() const;
PlatformMode getCurrentMode() const;
bool isHttpsEnabled() const;
String getBaseUrl() const;

// WiFi management
void resetWiFiCredentials();
String getHostname() const;
```

### IWebModule Interface

```cpp
class IWebModule {
public:
    // Required implementations
    virtual std::vector<WebRoute> getHttpRoutes() = 0;
    virtual std::vector<WebRoute> getHttpsRoutes() = 0;
    virtual String getModuleName() const = 0;
    
    // Optional implementations
    virtual String getModuleVersion() const { return "1.0.0"; }
    virtual String getModuleDescription() const { return "Web-enabled module"; }
    
    // Static utility methods
    static void setNavigationMenu(const std::vector<NavigationItem>& items);
    static String injectNavigationMenu(const String& htmlContent);
    static void setErrorPage(int statusCode, const String& html);
    static void addRedirect(const String& fromPath, const String& toPath);
};
```

### WebRoute Structure

```cpp
struct WebRoute {
    String path;                                    // Route path
    WebModule::Method method;                       // HTTP method  
    WebModule::UnifiedRouteHandler unifiedHandler;  // Request handler
    String contentType;                             // Response content type
    String description;                             // Optional description
    AuthRequirements authRequirements;             // Authentication requirements
    
    // Constructor with auth requirements
    WebRoute(const String& path, WebModule::Method method,
             WebModule::UnifiedRouteHandler handler,
             const AuthRequirements& auth = {AuthType::NONE});
};
```

## Advanced Features

### Custom Error Pages
```cpp
IWebModule::setErrorPage(404, R"(
    <html><body>
        <h1>Page Not Found</h1>
        <p>The requested page could not be found.</p>
    </body></html>
)");
```

### URL Redirects
```cpp
IWebModule::addRedirect("/old-path", "/new-path");
IWebModule::addRedirect("/settings", "/config/");
```

### Template System with Bookmarks
WebPlatform includes an automatic template processing system that replaces bookmarks in HTML content. This happens automatically for HTML responses and requires no manual processing.

```cpp
// Define a handler that uses bookmarks
webPlatform.registerRoute("/example", [](WebRequest& req, WebResponse& res) {
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
}, {AuthType::SESSION});
```

Supported bookmarks:
- `{{NAV_MENU}}` - Injects the navigation menu (replaces the old comment-based system)
- `{{csrfToken}}` - Generates and injects a CSRF token for form protection
- `{{SECURITY_NOTICE}}` - Displays HTTPS status notice for sensitive forms
- `{{username}}` - Shows the authenticated user's username
- `{{DEVICE_NAME}}` - Shows the device name set during initialization

### Opt-Out of Template Processing
```cpp
// Disable template processing for a specific response
res.setHeader("X-Skip-Template-Processing", "true");
res.setContent(html, "text/html");
```

### Navigation Menu
```cpp
std::vector<NavigationItem> navItems = {
    NavigationItem("Dashboard", "/"),
    NavigationItem("Settings", "/config/"),
    NavigationItem("API Docs", "/docs", "_blank")  // Opens in new tab
};
IWebModule::setNavigationMenu(navItems);
```

### Accessing Authentication Context
```cpp
void protectedHandler(WebRequest& req, WebResponse& res) {
    const AuthContext& auth = req.getAuthContext();
    
    if (auth.authenticatedVia == AuthType::SESSION) {
        // User logged in via web interface
        String username = auth.username;
    } else if (auth.authenticatedVia == AuthType::TOKEN) {
        // API access via token
        String token = auth.token;
    }
}
```

## Memory and Performance

### Resource Requirements
- **Base Memory**: ~4KB RAM for WebPlatform instance
- **Per Module**: ~500 bytes additional per registered module
- **HTTPS Overhead**: ~2KB additional when certificates enabled
- **Storage**: 512 bytes EEPROM for WiFi credentials

### Performance Benefits
- **Single Server**: Eliminates dual HTTP/HTTPS server overhead
- **Unified Routing**: Single route table with efficient lookup
- **Asset Caching**: Built-in caching headers for static assets
- **Memory Management**: Careful string handling and minimal heap fragmentation

## Storage System

WebPlatform includes a flexible storage system inspired by Laravel's database architecture:

### Multiple Storage Drivers
- **JsonDatabaseDriver**: Default driver using ESP32 Preferences or ESP8266 EEPROM
- **Extensible Architecture**: Support for additional drivers (LittleFS, cloud databases)
- **Query Builder**: Laravel-inspired fluent API for data operations

### Usage Example
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

## Certificate Support

WebPlatform provides both HTTP and HTTPS support with automatic certificate detection:

### How HTTPS Works

WebPlatform uses ESP-IDF's native HTTPS server implementation (`esp_https_server.h`) which provides:
- Secure TLS/SSL connections
- Efficient memory usage  
- Hardware-accelerated cryptography
- Automatic HTTP to HTTPS redirection

### Certificate Configuration

**HTTPS is not automatically detected** - it requires manual configuration in both `platformio.ini` and certificate files.

#### To Enable HTTPS:
1. Generate or obtain SSL certificate files (see instructions below)
2. Place certificate files in the `src` directory:
   - `server_cert.pem` - Server certificate
   - `server_key.pem` - Private key
3. Ensure the following lines are **uncommented** in `platformio.ini` for ESP32 builds:
   ```ini
   board_build.embed_txtfiles =
     src/server_cert.pem
     src/server_key.pem
   ```

#### To Disable HTTPS (HTTP-only mode):
1. **Comment out or remove** the `board_build.embed_txtfiles` lines in `platformio.ini`:
   ```ini
   ; board_build.embed_txtfiles =
   ;   src/server_cert.pem
   ;   src/server_key.pem
   ```
2. Certificate files in `src` directory are not required when HTTPS is disabled

**Important**: If the `board_build.embed_txtfiles` lines are present in `platformio.ini` but the certificate files don't exist, the build will fail.

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

**After generating certificates**, ensure the `board_build.embed_txtfiles` lines are uncommented in `platformio.ini` as described above.

### Build Notes
- If you get build errors related to missing certificate files, check that the `board_build.embed_txtfiles` lines in `platformio.ini` match your setup (commented out for HTTP-only, uncommented with existing certificate files for HTTPS)
- For HTTP-only builds, ensure the certificate embedding lines are commented out or removed from `platformio.ini`
- ESP8266 builds use HTTP only and do not require certificate configuration

## Development Workflow

### For Module Developers
1. **Implement IWebModule**: Create routes and handlers for your module
2. **Define Authentication**: Specify appropriate security requirements
3. **Build Assets**: Create HTML/CSS/JS for your module's interface
4. **Test Integration**: Verify compatibility with WebPlatform
5. **Package Module**: Prepare for distribution as a library

### For Application Developers
1. **Initialize Platform**: Set up WebPlatform with device configuration
2. **Register Modules**: Add web modules for specific functionality
3. **Customize Routes**: Override or extend module behavior
4. **Configure Security**: Set up authentication and user management
5. **Deploy**: Single binary with automatic HTTPS detection

## Platform Dependencies

Add to your `platformio.ini`:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
  bblanchon/ArduinoJson@^6.20.0
  # Add web modules as needed
  
# Optional HTTPS certificate embedding
# board_build.embed_txtfiles = src/server_cert.pem, src/server_key.pem

[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps = 
  ${env:esp32-s3-devkitc-1.lib_deps}
  https://github.com/rweaver/arduinolibs.git
  marvinroger/ESP8266TrueRandom@^1.0.0
```

This comprehensive platform enables rapid development of sophisticated embedded web applications while maintaining security and performance standards. Whether you're building a simple device interface or a complex IoT system, WebPlatform provides the foundation you need.
