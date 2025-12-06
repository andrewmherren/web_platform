# WebPlatform Library

A comprehensive web development platform for ESP32 that provides a unified framework for building web-enabled embedded applications.

## 🚀 Quick Start

**New to WebPlatform?** Get your ESP32 web application running in minutes:

👉 **[Quick Start Guide](QUICK_START.md)** - 5-minute setup with code examples  
⚙️ **[Configuration Template](platformio.ini.example)** - Copy-paste PlatformIO config  
📋 **[Working Examples](examples/README.md)** - Complete code samples

## Documentation

- [User Guide](GUIDE.md) - Detailed usage instructions and examples
- [API Reference](API.md) - Complete API documentation
- [Authentication Guide](AUTH.md) - Detailed authentication system documentation
- [Examples](examples/README.md) - Example applications and modules

## Overview

WebPlatform is a complete web development framework that transforms ESP32 devices into sophisticated web-enabled applications. It combines multiple components into a single, easy-to-use library:

- **Modular Architecture**: Build reusable web modules using the IWebModule interface
- **Unified HTTP/HTTPS Server**: Single server instance with automatic HTTPS detection
- **WiFi Management**: Captive portal for configuration and automatic connection handling
- **Advanced Authentication**: Session-based and token-based authentication with CSRF protection
- **Dual Storage System**: JSON driver for fast access + LittleFS driver for large data
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
    webPlatform.setNavigationMenu(navItems);

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

**💡 Tip:** Use the **[Web Module Example](https://github.com/andrewmherren/web_module_example)** template as a starting point for new modules!

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
            ApiRoute("/api/data", WebModule::WM_GET, 
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

See the **[Web Module Example](https://github.com/andrewmherren/web_module_example)** repository for a complete, production-ready template with testing infrastructure, documentation, and best practices.

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
```

### HTTPS Mode

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
- **Dual Storage Drivers**: JSON (fast, small data) and LittleFS (scalable, large data) with unified query interface
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

## Related Projects

- **[web_platform_interface](https://github.com/andrewmherren/web_platform_interface)**: Core interface and testing library for WebPlatform ecosystem
- **[maker_api](https://github.com/andrewmherren/maker_api)**: Interactive API explorer module
- **[usb_pd_controller](https://github.com/andrewmherren/usb_pd_controller)**: USB-C Power Delivery control module
- **[ota_update](https://github.com/andrewmherren/ota_update)**: Over-the-air firmware update module
- **[web_module_example](https://github.com/andrewmherren/web_module_example)**: Boilerplate template for creating custom WebPlatform modules

## Dual OpenAPI 3.0 Support

WebPlatform includes built-in support for dual OpenAPI 3.0 specification generation:

### Full OpenAPI Specification
- Complete API documentation for all registered routes
- Intended for internal development and system administration
- Available at `/openapi.json` when enabled

### Maker API Specification
- Filtered specification containing only routes tagged for public/maker consumption
- Routes tagged with "maker" or configured tags
- Ideal for external documentation and client SDK generation
- Available at `/maker/openapi.json` when enabled

### Build Configuration

Both systems are **disabled by default** to conserve memory:

```ini
# Enable full OpenAPI documentation
build_flags = -DWEB_PLATFORM_OPENAPI=1

# Enable Maker API documentation (filtered subset)
build_flags = -DWEB_PLATFORM_MAKERAPI=1

# Enable both (recommended for development)
build_flags = 
  -DWEB_PLATFORM_OPENAPI=1
  -DWEB_PLATFORM_MAKERAPI=1
```

### Using OpenAPI with Third-Party Tools

**Full OpenAPI Specification** (`/openapi.json`):
- **Internal Development**: Complete API reference for system developers
- **Administrative Tools**: Full access to all system endpoints
- **Swagger UI**: Comprehensive interactive documentation

**Maker API Specification** (`/maker/openapi.json`):
- **Public Documentation**: Clean, focused API docs for end users
- **Postman**: Generate focused collections for public APIs
- **Client SDKs**: Generate libraries containing only public endpoints
- **Integration Guides**: Simplified API reference for makers

### Example with Dual OpenAPI Documentation

```cpp
// Route appears in both Full API and Maker API (has "maker" tag)
webPlatform.registerApiRoute("/device/status", 
    [](WebRequest& req, WebResponse& res) {
        res.setContent("{\"status\":\"online\",\"uptime\":12345}", "application/json");
    }, 
    {AuthType::TOKEN}, 
    WebModule::WM_GET,
    API_DOC("Get device status", 
            "Returns current device status including uptime and connection info",
            "getDeviceStatus", 
            {"maker", "device"}));

// Route appears only in Full API (no maker tag)
webPlatform.registerApiRoute("/admin/config", 
    [](WebRequest& req, WebResponse& res) {
        res.setContent("{\"config\":\"internal\"}", "application/json");
    }, 
    {AuthType::SESSION}, 
    WebModule::WM_GET,
    API_DOC("Get admin config", 
            "Internal system configuration",
            "getAdminConfig", 
            {"admin"}));
```

## Requirements

- ESP32 development board
- PlatformIO development environment
- Arduino framework for ESP32
- ArduinoJson library (automatically installed)

## Hardware Compatibility

- ESP32 (specific variant compatibility should be verified for your use case)
- All testing preformed on ESP32-S3

## Performance Considerations

Performance varies based on:
- ESP32 variant and clock speed
- Available RAM and flash memory
- Network conditions
- Application complexity
- Enabled features (HTTPS, OpenAPI documentation)

## Troubleshooting

### Common Issues

**WiFi Connection Issues**
- Ensure correct credentials in captive portal
- Check WiFi signal strength
- Verify network supports ESP32 (2.4GHz only)

**Memory Issues**
- Reduce concurrent connections
- Disable unused features (OpenAPI, HTTPS)
- Use LittleFS driver for large data instead of JSON driver

**HTTPS Certificate Issues**
- Verify certificate files are in `src/` directory
- Check certificate format (PEM)
- Ensure `board_build.embed_txtfiles` is correctly configured

### Debug Mode

Enable debug output in your `platformio.ini`:

```ini
build_flags = 
  -DCORE_DEBUG_LEVEL=3
  -DWEB_PLATFORM_DEBUG=1
```

## Related Projects

- **[web_platform_interface](https://github.com/andrewmherren/web_platform_interface)**: Core interface and testing library for WebPlatform ecosystem
- **[maker_api](https://github.com/andrewmherren/maker_api)**: Interactive API explorer module
- **[usb_pd_controller](https://github.com/andrewmherren/usb_pd_controller)**: USB-C Power Delivery control module
- **[ota_update](https://github.com/andrewmherren/ota_update)**: Over-the-air firmware update module
- **[web_module_example](https://github.com/andrewmherren/web_module_example)**: Boilerplate template for creating custom WebPlatform modules

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**Made with ❤️ for the ESP32 community**

*WebPlatform - Transforming ESP32 devices into sophisticated web-enabled applications*
