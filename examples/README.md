# WebPlatform Examples

This directory contains comprehensive examples demonstrating how to use the WebPlatform library for different types of embedded web applications.

## Examples Overview

### 1. Basic Application (`applications/basic_application.cpp`)
**Audience**: Application developers getting started with WebPlatform  
**Demonstrates**:
- Simple WebPlatform initialization and setup
- Custom route registration and navigation menu configuration  
- Static asset serving and WiFi configuration handling
- System information display and basic API endpoints
- Error handling and URL redirects

### 2. Authenticated Application (`applications/authenticated_application.cpp`)
**Audience**: Application developers building secure devices  
**Demonstrates**:
- Session-based authentication for web users
- API token authentication for programmatic access
- CSRF protection for forms and route-level security configuration
- User account management and secure API endpoints
- NTP time synchronization and authentication-aware navigation

### 3. Custom Module (`custom_module/environmental_sensor_module.h/.cpp`)
**Audience**: Module developers creating reusable components  
**Demonstrates**:
- Complete IWebModule interface implementation with proper C++ structure
- Mixed authentication requirements (local, session, token-based)
- RESTful API design and interactive web interface with real-time updates
- Configuration management with CSRF protection
- Professional class design and module lifecycle management

See the [Custom Module README](custom_module/README.md) for detailed documentation, API examples, and real-world adaptation guide.

## Quick Start Guide

### For Application Developers

If you want to build a complete embedded web application:

1. **Start with Basic Application**: Copy `basic_application.cpp` as your foundation
2. **Add Authentication**: Review `authenticated_application.cpp` for security features
3. **Integrate Modules**: Use existing modules or create custom ones following the module example

```cpp
// Minimal application setup
#include <web_platform.h>

void setup() {
    webPlatform.begin("MyDevice");
    
    if (webPlatform.isConnected()) {
        webPlatform.registerWebRoute("/", [](WebRequest& req, WebResponse& res) {
            res.setContent("<h1>Hello World!</h1>", "text/html");
        });
    }
}

void loop() {
    webPlatform.handle();
}
```

### For Module Developers

If you want to create reusable web components:

1. **Study the Custom Module**: Review the [Environmental Sensor Module](custom_module/README.md) for best practices
2. **Implement IWebModule**: Create your class inheriting from IWebModule with proper header/source separation
3. **Define Routes**: Specify HTTP routes with appropriate authentication levels
4. **Test Integration**: Ensure your module works in different applications

```cpp
// environmental_sensor_module.h
class MyModule : public IWebModule {
private:
    // Private data members
    float sensorValue;
    
public:
    // Lifecycle methods
    void begin();
    void handle();
    
    // Public API
    float getSensorValue() const;
    
    // IWebModule interface
    std::vector<RouteVariant> getHttpRoutes() override;
    std::vector<RouteVariant> getHttpsRoutes() override;
    String getModuleName() const override;
    
private:
    // Route handlers
    void mainPageHandler(WebRequest& req, WebResponse& res);
    void apiHandler(WebRequest& req, WebResponse& res);
};
```

See the [Custom Module documentation](custom_module/README.md) for complete implementation examples, API usage, and real-world adaptation tips.

## Example Features Comparison

| Feature | Basic App | Auth App | Custom Module |
|---------|-----------|----------|---------------|
| WiFi Management | ✅ | ✅ | ✅ |
| Custom Routes | ✅ | ✅ | ✅ |
| Navigation Menu | ✅ | ✅ | ✅ |
| Static Assets | ✅ | ✅ | ✅ |
| User Authentication | ❌ | ✅ | ✅* |
| API Tokens | ❌ | ✅ | ✅* |
| CSRF Protection | ❌ | ✅ | ✅ |
| Module Architecture | ❌ | ❌ | ✅ |
| RESTful APIs | Basic | ✅ | ✅ |
| Real-time Updates | ❌ | ❌ | ✅ |
| NTP Time Sync | ❌ | ✅ | ❌ |
| Error Pages | Basic | ❌ | ❌ |
| URL Redirects | ✅ | ❌ | ❌ |

*\* Authentication support through route requirements, but can be used without auth*

## Running the Examples

### PlatformIO Setup

1. Copy the desired example to your `src/main.cpp`:
   ```bash
   # For basic application
   cp lib/web_platform/examples/applications/basic_application.cpp src/main.cpp
   
   # For authenticated application  
   cp lib/web_platform/examples/applications/authenticated_application.cpp src/main.cpp
   
   # For custom module (copy module files and main)
   mkdir -p lib/environmental_sensor/
   cp lib/web_platform/examples/custom_module/lib/custom_module/environmental_sensor_module.* lib/environmental_sensor/
   cp lib/web_platform/examples/custom_module/custom_module_main.cpp src/main.cpp
   ```

2. Ensure your `platformio.ini` includes the web_platform dependency:
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   lib_deps = 
       bblanchon/ArduinoJson@^6.20.0
       # Add other sensor libraries as needed
   ```

3. Build and upload:
   ```bash
   pio run --target upload
   ```

### First Run

1. **WiFi Configuration**: 
   - Device creates an access point `[DeviceName]Setup`
   - Connect to the AP and configure WiFi credentials
   - Device restarts and connects to your WiFi network

2. **Access the Application**:
   - Find device IP address in Serial Monitor
   - Or use mDNS: `http://devicename.local/`
   - HTTPS automatically used if certificates available

3. **Authentication (Auth App and Custom Module)**:
   - Default credentials: `admin` / `admin`
   - Change credentials through account page
   - Create API tokens for programmatic access

## Customization Tips

### Styling and Theming
All examples use the built-in CSS framework (`/assets/style.css`). Key classes:
- `.container` - Main content wrapper
- `.card` - Content cards with glass morphism effect  
- `.status-grid` - Grid layout for status indicators
- `.btn .btn-primary .btn-secondary` - Styled buttons
- `.form-group .form-control` - Form styling

### Adding Custom Routes
```cpp
webPlatform.registerWebRoute("/my-page", [](WebRequest& req, WebResponse& res) {
    String html = "<!DOCTYPE html><html>...";
    res.setContent(html, "text/html");
});
```

### API Endpoints
```cpp
webPlatform.registerApiRoute("/data", [](WebRequest& req, WebResponse& res) {
    String json = "{\"status\":\"ok\",\"data\":123}";
    res.setContent(json, "application/json");
}, {AuthType::TOKEN});  // Require API token
```

### Navigation Menu
```cpp
std::vector<NavigationItem> navItems = {
    NavigationItem("Home", "/"),
    NavigationItem("Settings", "/config"),
    NavigationItem("Help", "/help", "_blank")  // Opens in new tab
};
IWebModule::setNavigationMenu(navItems);
```

## Security Considerations

### Authentication Best Practices
1. **Change Default Credentials**: Always change `admin`/`admin` in production
2. **Use HTTPS**: Enable HTTPS with proper certificates when possible
3. **Protect Sensitive Routes**: Use `{AuthType::SESSION}` for admin functions
4. **CSRF Protection**: Use `{AuthType::PAGE_TOKEN}` for state-changing forms
5. **API Security**: Use `{AuthType::TOKEN}` for programmatic access

### Network Security
1. **Local Access**: Consider `{AuthType::LOCAL_ONLY}` for sensitive functions
2. **Firewall**: Configure network firewalls appropriately
3. **Regular Updates**: Keep firmware updated with latest WebPlatform version

## Troubleshooting

### Common Issues

**WiFi Configuration Not Working**:
- Ensure device creates AP correctly
- Check Serial Monitor for error messages
- Verify device isn't already connected to a network

**Authentication Failures**:
- Verify credentials are correct (case-sensitive)
- Clear browser cookies/cache
- Check Serial Monitor for auth errors

**Module Registration Fails**:
- Ensure module is registered only when `webPlatform.isConnected()` returns true
- Verify module implements all required IWebModule methods
- Check for route path conflicts

**HTTPS Not Working**:
- Verify certificates are properly embedded
- Check certificate format and validity
- Monitor Serial output for HTTPS initialization messages

### Debug Tips

1. **Enable Serial Logging**: Use `Serial.println()` liberally during development
2. **Check Route Registration**: Use `webPlatform.printUnifiedRoutes()` to verify routes
3. **Monitor Authentication**: Check auth context in route handlers
4. **Test APIs**: Use curl or Postman to test API endpoints independently

## Contributing

When contributing new examples:

1. **Follow Patterns**: Use consistent coding style and structure
2. **Add Comments**: Explain complex concepts and design decisions  
3. **Test Thoroughly**: Verify examples work on both ESP32 and ESP8266
4. **Document Features**: Update this README with new capabilities
5. **Consider Security**: Demonstrate proper authentication usage

## Additional Resources

- [WebPlatform Library Documentation](../README.md) - Main library documentation
- [Custom Module Development Guide](custom_module/README.md) - Detailed module creation guide
- [Module Development Best Practices](../README.md#for-module-developers) - General module guidelines
- [Authentication System Guide](../README.md#authentication-system) - Security implementation details
