# WebPlatform Library

A comprehensive web development platform for ESP32/ESP8266 that provides a unified framework for building web-enabled embedded applications.

## Documentation

- [User Guide](GUIDE.md) - Detailed usage instructions and examples
- [API Reference](API.md) - Complete API documentation
- [Authentication Guide](AUTH.md) - Detailed authentication system documentation
- [Examples](examples/README.md) - Example applications and modules

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
    // Set up navigation menu with authentication-aware items
    std::vector<NavigationItem> navItems = {
        NavigationItem("Home", "/"),
        NavigationItem("Settings", "/settings/"),
        NavigationItem("About", "/about"),
        Authenticated(NavigationItem("Account", "/account")),
        Authenticated(NavigationItem("Logout", "/logout")),
        Unauthenticated(NavigationItem("Login", "/login"))
    };
    IWebModule::setNavigationMenu(navItems);

    // Initialize WebPlatform (auto-detects HTTPS capability)
    webPlatform.begin("MyDevice");
    
    if (webPlatform.isConnected()) {
        // Register web modules
        // webPlatform.registerModule("/device", &deviceModule);
        
        // Add custom application routes
        webPlatform.registerWebRoute("/about", [](WebRequest& req, WebResponse& res) {
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
    std::vector<RouteVariant> getHttpRoutes() override {
        return {
            WebRoute("/", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getMainPage(), "text/html");
                }),
            ApiRoute("/data", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    res.setContent(getDataJSON(), "application/json");
                }, {AuthType::TOKEN})  // API requires token authentication
        };
    }
    
    std::vector<RouteVariant> getHttpsRoutes() override {
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

## Build Configuration

To use WebPlatform in your project, add the following to your `platformio.ini`:

### HTTP-only Mode (default)

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
  bblanchon/ArduinoJson@^6.20.0
  # Add your web modules here

[env:esp8266]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps = 
  bblanchon/ArduinoJson@^6.20.0
  https://github.com/rweaver/arduinolibs.git
  marvinroger/ESP8266TrueRandom@^1.0.0
  # Add your web modules here
```

### HTTPS Mode (ESP32 only)

```ini
[env:esp32-https]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
  bblanchon/ArduinoJson@^6.20.0
  # Add your web modules here
  
# HTTPS certificate embedding - certificate files must exist in src/ directory
board_build.embed_txtfiles = 
  src/server_cert.pem
  src/server_key.pem
```

### Generate SSL Certificates

```bash
# Generate private key
openssl genrsa -out src/server_key.pem 2048

# Generate certificate signing request
openssl req -new -key src/server_key.pem -out src/server_csr.pem

# Generate self-signed certificate (valid for 365 days)
openssl x509 -req -days 365 -in src/server_csr.pem -signkey src/server_key.pem -out src/server_cert.pem

# Clean up temporary file
rm src/server_csr.pem
```

## Key Features

For more information on WebPlatform features, see the [User Guide](GUIDE.md).

- **Dual-mode Operation**: WiFi configuration portal and application modes
- **Authentication System**: Session-based and token-based authentication
- **Route Management**: Register, override, and disable routes with OpenAPI documentation support
- **OpenAPI 3.0 Integration**: Automatic API documentation generation for registered routes
- **Template System**: HTML templates with automatic bookmark replacement
- **Storage System**: Flexible database drivers with query builder
- **HTTPS Support**: Automatic HTTPS when certificates available
- **Asset Streaming**: Memory-efficient serving of embedded assets using PROGMEM streaming

## Learn More

- [Authentication System](GUIDE.md#authentication-system)
- [Template System](GUIDE.md#template-system)
- [Storage System](GUIDE.md#storage-system)
- [HTTPS Configuration](GUIDE.md#https-configuration)
- [Navigation Menu](GUIDE.md#navigation-menu)
- [Examples](examples/README.md)
- [API Reference](API.md)

## Compatible Modules

The following modules are designed to work with WebPlatform:

- **[Maker API](https://github.com/andrewmherren/maker_api)** - Interactive API documentation and testing interface with OpenAPI 3.0 support
- More modules coming soon...

## OpenAPI 3.0 Support

WebPlatform includes built-in support for OpenAPI 3.0 specification generation. When you register API routes using `registerApiRoute()`, the platform automatically generates comprehensive API documentation that can be consumed by tools like:

- **Postman**: Import OpenAPI specs for easy API testing
- **Swagger UI**: Generate interactive API documentation
- **Code Generators**: Automatically generate client libraries

### Example with OpenAPI Documentation

```cpp
// Register an API route with detailed OpenAPI documentation
webPlatform.registerApiRoute("/device/status", 
    [](WebRequest& req, WebResponse& res) {
        res.setContent("{\"status\":\"online\",\"uptime\":12345}", "application/json");
    }, 
    {AuthType::TOKEN}, 
    WebModule::WM_GET,
    OpenAPIDocumentation(
        "Get device status",
        "Returns current device status including uptime and connection info",
        "getDeviceStatus", 
        {"Device Management"}
    )
);

// Access the OpenAPI specification at /api/openapi.json
String spec = webPlatform.getOpenAPISpec();
```

The Maker API module provides a complete web interface for exploring, testing, and managing these documented APIs directly from your device.
