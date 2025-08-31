# WebPlatform Library

Unified web platform for ESP32/ESP8266 with integrated module interface, WiFi management, HTTPS support, and authentication system.

## Overview

WebPlatform is a comprehensive library that combines:
- **Web Module Interface**: Abstract interface for web-enabled modules with CSS theming and navigation systems
- **Web Platform**: Unified HTTP/HTTPS server with WiFi configuration and application serving
- **Authentication System**: Session-based and token-based authentication with CSRF protection
- **Asset Management**: Static asset serving and CSS/JS framework
- **WiFi Management**: Captive portal configuration and connection handling

## Architecture

### Core Components

1. **Interface Layer** (`include/interface/`, `src/interface/`)
   - Abstract IWebModule interface
   - Request/Response abstractions
   - Authentication types and requirements
   - UI styling and navigation systems

2. **Platform Layer** (`platform/`)
   - Core web server implementation
   - WiFi management and captive portal
   - Route registration and handling
   - HTTPS certificate detection

3. **Authentication System** (`auth/`)
   - User management and password hashing
   - Session and API token management
   - CSRF protection
   - Authentication middleware

4. **Asset Management** (`assets/`)
   - Built-in CSS framework
   - JavaScript utilities
   - HTML templates for common pages

## Status Update: Authentication Framework Added (Phases 1-2 Complete)
The WebPlatform implementation now includes a complete authentication framework (Phase 2). It provides a simplified, unified web server that handles WiFi configuration, application serving, and authentication through a single instance. The system now supports multiple authentication types (SESSION, TOKEN) with route-specific protection requirements.

## Key Features

### Core Architecture
- **Single Server Instance**: HTTP or HTTPS based on certificate availability
- **Mode-Based Operation**: Automatic switching between config portal and application modes
- **Restart-Based Transitions**: Simple and reliable mode switching via device restart
- **Module Registration**: IWebModule-based component registration for connected mode
- **Static Asset Management**: Consistent asset serving across both modes
- **Authentication Framework**: Route-specific auth requirements with session and token support

### Security Enhancements
- **HTTPS Config Portal**: Secure WiFi credential transmission when certificates available
- **Certificate Auto-Detection**: Runtime certificate detection without build flags
- **Graceful Fallback**: HTTP operation when certificates unavailable
- **Captive Portal**: Secure WiFi setup with DNS redirection
- **Session Authentication**: Cookie-based web authentication system
- **API Tokens**: Secure Bearer token authentication for programmatic access
- **Route Protection**: Fine-grained authentication requirements per route

### Integration Benefits
- **Simplified API**: Single initialization and handle calls
- **Reduced Memory**: Eliminates dual-server overhead
- **Consistent UX**: Same theming and navigation in both modes
- **Enhanced Security**: HTTPS for sensitive operations when possible

## Usage

### Basic Integration
```cpp
#include <web_platform.h>
#include <usb_pd_controller.h>  // Your modules

void setup() {
    // Single initialization call
    webPlatform.begin("MyDevice");
    
    // Register modules (only works in connected mode)
    if (webPlatform.isConnected()) {
        // Register module routes
        webPlatform.registerModule("/usb_pd", &usbPDController);
        
        // Override a module route with custom auth requirement
        webPlatform.overrideRoute("/usb_pd/config", configHandler, {AuthType::SESSION});
        
        // Disable a module route completely
        webPlatform.disableRoute("/usb_pd/firmware");
    }
}

void loop() {
    // Single handle call manages everything
    webPlatform.handle();
    
    // Handle your modules only when connected
    if (webPlatform.isConnected()) {
        usbPDController.handle();
    }
}
```

### Compared to Previous Architecture
**Before (Separate Components):**
```cpp
void setup() {
    wifiManager.begin("Device", true);
    
    if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
        webRouter.registerModule("/module", &module);
        webRouter.begin(80, 443);
    }
}

void loop() {
    wifiManager.handle();
    if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
        webRouter.handle();
        module.handle();
    }
}
```

**After (WebPlatform):**
```cpp
void setup() {
    webPlatform.begin("Device");
    if (webPlatform.isConnected()) {
        webPlatform.registerModule("/module", &module);
    }
}

void loop() {
    webPlatform.handle();
    if (webPlatform.isConnected()) {
        module.handle();
    }
}
```

## Operating Modes

### CONFIG_PORTAL Mode
**Activation**: No stored WiFi credentials or connection failure  
**Behavior**: 
- Creates WiFi access point (`DeviceNameSetup`)
- Serves captive portal for WiFi configuration
- Uses HTTPS if certificates available (security enhancement)
- Provides basic navigation and static assets
- Restarts device after successful credential save

**Access**: Connect to AP, browser redirects to configuration page

### CONNECTED Mode
**Activation**: Valid WiFi credentials exist and connection successful  
**Behavior**:
- Connects to stored WiFi network
- Serves registered application modules
- Provides WiFi management interface
- Uses HTTPS if certificates available
- Enables mDNS hostname resolution

**Access**: `http://device.local/` or device IP address

## API Reference

### Core Methods
```cpp
// Initialization
void begin(const char* deviceName = "Device");

// Module management
bool registerModule(const char* basePath, IWebModule* module);

// Route management
void registerRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                  const AuthRequirements &auth = {AuthType::NONE},
                  WebModule::Method method = WebModule::WM_GET);
void overrideRoute(const String &path, WebModule::UnifiedRouteHandler handler,
                  const AuthRequirements &auth = {AuthType::NONE},
                  WebModule::Method method = WebModule::WM_GET);
void disableRoute(const String &path, WebModule::Method method = WebModule::WM_GET);

// Request handling
void handle();

// State queries
bool isConnected() const;
WiFiConnectionState getConnectionState() const;
PlatformMode getCurrentMode() const;
bool isHttpsEnabled() const;
String getBaseUrl() const;

// Device information
const char* getDeviceName() const;
const char* getAPName() const;
String getHostname() const;

// WiFi management
void resetWiFiCredentials();
void startConfigPortal();
void onSetupComplete(WiFiSetupCompleteCallback callback);

// Route debugging
size_t getRouteCount() const;
void printUnifiedRoutes() const;
void validateRoutes() const;
```

### States and Enums
```cpp
enum PlatformMode {
    CONFIG_PORTAL,    // WiFi configuration portal
    CONNECTED        // Connected, serving application
};

enum WiFiConnectionState {
    WIFI_CONNECTING,       // Attempting connection
    WIFI_CONNECTED,        // Connected successfully
    WIFI_CONFIG_PORTAL,    // Running config portal
    WIFI_CONNECTION_FAILED // Connection failed
};
```

## Implementation Status

### Phase 1: Basic WebPlatform Structure ✅
- [x] Created `web_platform.h` with unified class design
- [x] Implemented basic server initialization and mode detection
- [x] Added WiFi credential management (EEPROM)
- [x] Basic HTTP server setup with mode-specific routing
- [x] Config portal and connected mode route registration
- [x] Module registration system for connected mode
- [x] Basic HTML pages for both modes

### Phase 2: Certificate Detection & HTTPS ✅
- [x] Runtime certificate detection without build flags
- [x] HTTPS server configuration for both modes
- [x] Secure config portal implementation
- [x] Certificate validation and fallback logic

### Phase 3: Unified Mode Management ✅
- [x] Single server instance for both modes
- [x] Restart-based mode switching
- [x] Static route sets for different modes
- [x] Certificate detection in both modes

### Phase 4: Static Route Sets ✅
- [x] Complete static asset integration
- [x] Module route registration and handling
- [x] Advanced captive portal features
- [x] Error handling and custom pages

### Phase 5: Simplified main.cpp Integration ✅
- [x] Single initialization call with `webPlatform.begin()`
- [x] Unified request handling with `webPlatform.handle()`
- [x] Conditional module registration based on connection state
- [x] Simplified LED status indication
- [x] Documentation updates for new integration pattern

### Phase 6: Certificate Detection Enhancement ✅
- [x] Enhance runtime certificate detection
- [x] Eliminate build flag requirements for HTTPS
- [x] Implement automatic protocol selection
- [x] Fallback logic for certificate failures

## Certificate Support

WebPlatform is designed to automatically detect and use HTTPS certificates when available, without requiring build-time configuration. This enables:

1. **Secure Config Portal**: WiFi credentials transmitted over HTTPS
2. **Secure Application**: All module interfaces served over HTTPS
3. **Automatic Fallback**: HTTP operation when certificates unavailable
4. **No Build Dependencies**: Runtime detection eliminates platformio.ini requirements

*Certificate detection implementation completed in Phase 2*

## Memory Considerations

### Advantages over Separate Components
- **Single Server**: Eliminates dual HTTP/HTTPS server instances
- **Unified Routing**: Single route table instead of separate registries
- **Shared Assets**: Static assets served from single location
- **Reduced Overhead**: Single initialization and handle loops

### Resource Requirements
- **Base Memory**: ~4KB RAM for WebPlatform instance
- **Per Module**: ~500 bytes additional per registered module
- **HTTPS Overhead**: ~2KB additional when certificates available
- **EEPROM Usage**: 512 bytes for WiFi credential storage

## Security Features

### Config Portal Security
- **HTTPS Transmission**: WiFi credentials sent over encrypted connection
- **Captive Portal**: Isolated network for configuration
- **No Credential Storage**: Temporary credential validation before save
- **Access Control**: Only configuration endpoints available in portal mode

### Application Security  
- **HTTPS by Default**: All registered modules served over HTTPS when available
- **Certificate Validation**: Runtime verification of certificate availability
- **Secure Headers**: Appropriate security headers for HTTPS responses
- **mDNS Integration**: Secure hostname resolution

## Integration with Existing Modules

WebPlatform maintains full compatibility with existing IWebModule implementations:

```cpp
// Existing module works without changes
class MyModule : public IWebModule {
    std::vector<WebRoute> getHttpRoutes() override {
        return {
            WebRoute("/", WebModule::WM_GET, 
                    [this](const String& body, const std::map<String, String>& params) {
                        return generatePage();
                    }, "text/html")
        };
    }
    // ... other IWebModule methods
};

// Registration identical to web_router
webPlatform.registerModule("/mymodule", &myModule);
```

All existing features remain available:
- Static asset management via `IWebModule::addStaticAsset()`
- Navigation menu system via `IWebModule::setNavigationMenu()`
- Custom error pages via `IWebModule::setErrorPage()`
- Route redirection via `IWebModule::addRedirect()`

## Future Enhancements

### Planned Features
- **WebSocket Support**: Real-time communication for modules
- **OTA Updates**: Secure firmware updates over HTTPS
- **User Authentication**: Login system for application access
- **API Rate Limiting**: Protection against abuse
- **Configuration Backup**: Export/import device settings

### Performance Optimizations
- **Route Caching**: Faster route resolution for frequently accessed paths
- **Asset Compression**: Gzip compression for large static assets
- **Connection Pooling**: Efficient handling of multiple simultaneous requests
- **Memory Pooling**: Reduced heap fragmentation for long-running operations

## Migration Guide

### From Separate Components
1. Replace `#include <web_router.h>` and `#include <wifi_ap.h>` with `#include <web_platform.h>`
2. Replace `wifiManager.begin()` + `webRouter.begin()` with `webPlatform.begin()`
3. Replace separate `.handle()` calls with single `webPlatform.handle()`
4. Update module registration from `webRouter.registerModule()` to `webPlatform.registerModule()`
5. Update state checks from `wifiManager.getConnectionState()` to `webPlatform.isConnected()`

### Backward Compatibility
During transition period, both old and new systems can coexist:
- Keep existing `lib/web_router/` and `lib/wifi_ap/` as reference
- Develop with `lib/web_platform/` for new features
- Migrate modules incrementally to WebPlatform
- Remove old components after full validation

This unified approach significantly simplifies the architecture while enhancing security and reducing resource usage.