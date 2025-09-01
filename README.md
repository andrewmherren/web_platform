# WebPlatform Library

A comprehensive web development platform for ESP32/ESP8266 that provides a unified framework for building web-enabled embedded applications.

## Overview

WebPlatform combines multiple components into a single, easy-to-use library:

- **Web Module Interface**: Abstract base class for creating reusable web modules
- **Unified HTTP/HTTPS Server**: Single server instance with automatic HTTPS detection
- **WiFi Management**: Captive portal for configuration and automatic connection handling
- **Authentication System**: Session-based and token-based authentication with CSRF protection
- **Route Management**: Advanced route handling with override and disable capabilities
- **Asset Management**: Static asset serving with CSS/JS framework

## Quick Start

### For Application Developers

```cpp
#include <web_platform.h>
//#include <some_module.h> //optional module includes

void setup() {
    // Set up navigation menu
    std::vector<NavigationItem> navItems = {
        NavigationItem("Home", "/"),
        NavigationItem("Settings", "/settings/"),
        NavigationItem("About", "/about")
    };
    IWebModule::setNavigationMenu(navItems);

    // Initialize WebPlatform
    webPlatform.begin("MyDevice");
    
    if (webPlatform.isConnected()) {
        // Register your modules
        //webPlatform.registerModule("/route_prefix/", &someModule);
        
        // Add custom routes
        webPlatform.registerRoute("/about", [](WebRequest& req, WebResponse& res) {
            res.setContent("<h1>About My Device</h1>", "text/html");
        });
        
        // Override module routes if needed
        webPlatform.overrideRoute("/", customHomeHandler, {AuthType::SESSION});
    }
}

void loop() {
    webPlatform.handle();
    
    // if (webPlatform.isConnected()) {
    //     // call any registered modules handle method
    //     someModule.handle();
    // }
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

## Certificate Support

WebPlatform automatically detects HTTPS certificates at runtime without requiring build-time configuration:

1. **Runtime Detection**: Scans for embedded certificates during initialization
2. **Automatic HTTPS**: Enables secure communication when certificates available  
3. **HTTP Fallback**: Graceful degradation when certificates not present
4. **No Build Flags**: Eliminates platformio.ini certificate configuration requirements

## Development Workflow

1. **Create Your Module**: Implement `IWebModule` interface
2. **Define Routes**: Specify HTTP routes with appropriate authentication
3. **Set Up Application**: Initialize WebPlatform and register modules  
4. **Customize**: Add navigation, error pages, redirects as needed
5. **Test**: Use built-in WiFi configuration and authentication features
6. **Deploy**: Single binary with automatic HTTPS detection

This comprehensive platform enables rapid development of sophisticated embedded web applications while maintaining security and performance standards.