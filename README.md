# WiFi Manager for ESP8266/ESP32

A comprehensive WiFi management library for ESP8266/ESP32 microcontrollers that provides a configuration portal, persistent storage of credentials, mDNS hostname support, and web interface integration. Implements the `IWebModule` interface for seamless integration with web routing systems.## Features

- **Easy WiFi Configuration**: Hosts an access point with a captive portal that allows users to configure their WiFi network details without hardcoding.
- **Persistent Storage**: Stores WiFi credentials in EEPROM so they survive power cycles.
- **Configuration Portal**: Automatically starts a configuration portal if connection to the stored WiFi network fails.
- **Web Module Interface**: Implements `IWebModule` interface for integration with web routing systems.
- **Dual-Mode Operation**: Operates in config portal mode (no credentials) or normal mode (connected).
- **REST API**: Includes RESTful API endpoints for programmatic management of WiFi settings.
- **Connection Management**: Handles connection, reconnection, and status monitoring.
- **Callback Support**: Provides callback when WiFi setup is complete for application initialization.
- **Fallback Mode**: Easily reset WiFi settings and return to configuration mode.
- **mDNS Hostname**: Provides easy device access via a human-readable hostname (e.g., http://mydevice.local).
- **Global Instance**: Provides `extern` global instance for easy access across modules.## Installation

1. Create a new PlatformIO project for your ESP8266 or ESP32 device.
2. Add this library to your project's `lib` folder.
3. Add the `web_module_interface` dependency to your `platformio.ini`:
   ```ini
   lib_deps = 
     https://github.com/andrewmherren/web_module_interface.git
     # ... other dependencies
   ```## Usage

### Basic Usage (Standalone)

```cpp
#include <Arduino.h>
#include <wifi_ap.h>

void setup() {
  Serial.begin(115200);
  
  // Start WiFi manager with custom base name
  // This will create an AP named "MyDeviceSetup" during configuration
  // and set the mDNS hostname to "mydevice.local" when connected
  wifiManager.begin("MyDevice", true);
}

void loop() {
  // Must be called in loop to handle WiFi operations
  wifiManager.handle();
}
```

### Web Router Integration (Recommended)

```cpp
#include <Arduino.h>
#include <wifi_ap.h>
#include <web_router.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi Manager
  wifiManager.begin("MyDevice", true);
  
  // Check connection state for dual-mode operation
  if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
    // Mode B: Connected - integrate with web router
    webRouter.registerModule("/wifi", &wifiManager);
    webRouter.begin(80, 443);
    
    Serial.println("WiFi Settings: " + webRouter.getBaseUrl() + "/wifi/");
  } else {
    // Mode A: Config portal - WiFiManager handles its own server
    Serial.println("Connect to: " + String(wifiManager.getAPName()));
  }
}

void loop() {
  wifiManager.handle(); // Always handle WiFi operations
  
  // Only handle web router when connected
  if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
    webRouter.handle();
  }
}
```### IWebModule Interface

The WiFiManager implements the `IWebModule` interface and provides these routes:

```cpp
// Routes provided by getHttpRoutes() / getHttpsRoutes():
// GET  /           - WiFi management page (if web interface enabled)
// POST /save       - Save WiFi credentials (if web interface enabled)
// GET  /api/status - Get WiFi connection status
// GET  /api/scan   - Scan for available networks
// POST /api/connect - Connect to WiFi network
// POST /api/disconnect - Disconnect from current network  
// POST /api/reset  - Reset WiFi settings and restart
```

### Manual Control

```cpp
#include <Arduino.h>
#include <wifi_ap.h>

void setup() {
  Serial.begin(115200);
  
  wifiManager.begin("CustomAPName", true);
  
  // Check the connection state
  if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
    Serial.println("Connected to WiFi: " + WiFi.SSID());
    Serial.println("IP address: " + WiFi.localIP().toString());
    Serial.println("Hostname: " + wifiManager.getHostname());
  } 
  else if (wifiManager.getConnectionState() == WIFI_CONFIG_PORTAL) {
    Serial.println("Running in configuration portal mode");
    Serial.println("Connect to WiFi network: " + String(wifiManager.getAPName()));
    Serial.println("Navigate to http://192.168.4.1");
  }
  
  // Manual controls:
  // wifiManager.startConfigPortal();  // Force config portal
  // wifiManager.resetSettings();      // Clear stored credentials
}

void loop() {
  wifiManager.handle();
}
```## Web Interface

The library provides a responsive web interface and operates in two modes:

### Mode A: Configuration Portal (No WiFi Credentials)
- **Access Point**: Creates `[baseName]Setup` network (e.g., `MyDeviceSetup`)
- **Captive Portal**: `http://192.168.4.1/` - WiFi setup page
- **Internal Server**: WiFiManager runs its own server for captive portal

### Mode B: Connected Mode (WiFi Credentials Stored)
- **Normal Operation**: Connects to stored WiFi network
- **Web Router Integration**: Routes available when registered with web router
- **Device Access**:
  - Via IP Address: `http://[device-ip]/wifi/`
  - Via mDNS hostname: `http://[baseName].local/wifi/` (e.g., `http://mydevice.local/wifi/`)

### API Endpoints (Both Modes)
- `GET /api/status` - Returns current WiFi status
- `GET /api/scan` - Returns list of available WiFi networks  
- `POST /api/connect` - Connects to a new WiFi network
- `POST /api/disconnect` - Disconnects from current network
- `POST /api/reset` - Resets WiFi settings and restarts device

### mDNS Hostname Compatibility

The mDNS hostname feature allows access to the device using a human-readable name rather than an IP address:

- **iOS/macOS**: Native support for .local hostnames
- **Linux**: Most distributions work with Avahi installed
- **Android**: Supported on most modern devices
- **Windows**: Requires installation of Bonjour service (available with iTunes or as a standalone package)## Customization

You can customize the base name and whether to enable the web interface:

```cpp
// Custom base name, enable web interface
// This creates an AP named "MyCustomNameSetup" and hostname "mycustomname.local"
wifiManager.begin("MyCustomName", true);

// Default base name, disable web interface (API only)
// This creates an AP named "DeviceSetup" and hostname "device.local"
wifiManager.begin("Device", false);
```

### Getting Device Information

```cpp
// Get the base name used for the device
const char* baseName = wifiManager.getBaseName();  // e.g., "MyDevice"

// Get the full AP name used during setup
const char* apName = wifiManager.getAPName();  // e.g., "MyDeviceSetup"

// Get the full mDNS hostname
String hostname = wifiManager.getHostname();  // e.g., "mydevice.local"
```## Dependencies

- **web_module_interface** - Abstract interface for web module integration
- WiFi libraries (ESP8266WiFi or WiFi.h for ESP32)
- Web server libraries (ESP8266WebServer or WebServer for ESP32)
- mDNS libraries (ESP8266mDNS or ESPmDNS for ESP32)
- DNSServer library
- EEPROM library
- ArduinoJson library (for API responses)

## Architecture

This module implements a **dual-server architecture**:
- **Config Portal Mode**: Uses internal WebServer for captive portal
- **Normal Mode**: Provides routes via `IWebModule` interface for web router integration

This design allows for:
- Seamless captive portal experience without external dependencies
- Clean integration with larger web routing systems when connected
- Consistent API endpoints across both modes

## License

MIT License

Copyright (c) 2023 Your Name

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.