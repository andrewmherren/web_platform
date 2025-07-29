# WiFi Manager for ESP8266/ESP32

A comprehensive WiFi management library for ESP8266/ESP32 microcontrollers that provides a configuration portal, persistent storage of credentials, mDNS hostname support, and a web interface for managing WiFi connections.## Features

- **Easy WiFi Configuration**: Hosts an access point with a captive portal that allows users to configure their WiFi network details without hardcoding.
- **Persistent Storage**: Stores WiFi credentials in EEPROM so they survive power cycles.
- **Configuration Portal**: Automatically starts a configuration portal if connection to the stored WiFi network fails.
- **Web Interface**: Provides a responsive web interface for managing WiFi connections.
- **REST API**: Includes RESTful API endpoints for programmatic management of WiFi settings.
- **Connection Management**: Handles connection, reconnection, and status monitoring.
- **Callback Support**: Provides callback when WiFi setup is complete for application initialization.
- **Fallback Mode**: Easily reset WiFi settings and return to configuration mode.
- **mDNS Hostname**: Provides easy device access via a human-readable hostname (e.g., http://mydevice.local).## Installation

1. Create a new PlatformIO project for your ESP8266 or ESP32 device.
2. Add this library to your project:
   - Clone this repository into your project's `lib` folder: 
     ```
     git clone https://github.com/yourusername/wifi_ap.git lib/wifi_ap
     ```
   - Or use git submodules if your project is a git repository:
     ```
     git submodule add https://github.com/yourusername/wifi_ap.git lib/wifi_ap
     ```

## Usage

### Basic Usage

```cpp
#include <Arduino.h>
#include <wifi_ap.h>

void onWiFiSetupComplete() {
  Serial.println("WiFi connected! Your code goes here.");
  
  // Get the web server reference if you want to add your own handlers
  ESP8266WebServer &server = wifiManager.getWebServer();
  
  // Add your own web server handlers
  server.on("/myendpoint", []() {
    server.send(200, "text/plain", "Hello from my endpoint!");
  });
  
  // Start the web server
  server.begin();
}void setup() {
  Serial.begin(115200);
  
  // Set callback to be called when WiFi setup is complete
  wifiManager.onSetupComplete(onWiFiSetupComplete);
  
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

### Advanced Usage

```cpp
#include <Arduino.h>
#include <wifi_ap.h>

void setup() {
  Serial.begin(115200);
  
  // Manual control - no callback
  wifiManager.begin("CustomAPName", true);
  
  // Later in your code, you can check the connection state
  if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
    // WiFi is connected, do something
    Serial.println("Connected to WiFi: " + WiFi.SSID());
    Serial.println("IP address: " + WiFi.localIP().toString());
  } 
  else if (wifiManager.getConnectionState() == WIFI_CONFIG_PORTAL) {
    // Device is in configuration portal mode
    Serial.println("Running in configuration portal mode");
    Serial.println("Connect to WiFi network: " + String(wifiManager.getAPName()));
    Serial.println("Then navigate to http://192.168.4.1");
  }
  
  // You can also force start the configuration portal
  // wifiManager.startConfigPortal();
  
  // Or reset the stored WiFi settings
  // wifiManager.resetSettings();
}

void loop() {
  wifiManager.handle();
}
```## Web Interface

The library provides a responsive web interface for managing WiFi connections:

- **WiFi Setup Portal**: `http://[device-ip]/wifi` or `http://192.168.4.1` when in AP mode
- **Connected Device Access**:
  - Via IP Address: `http://[device-ip]/`
  - Via mDNS hostname: `http://[baseName].local/` (e.g., `http://mydevice.local/`)
- **API Endpoints**:
  - `GET /api/wifi/status` - Returns current WiFi status
  - `GET /api/wifi/scan` - Returns list of available WiFi networks
  - `POST /api/wifi/connect` - Connects to a new WiFi network
  - `POST /api/wifi/disconnect` - Disconnects from current network
  - `POST /api/wifi/reset` - Resets WiFi settings

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

- WiFi libraries (ESP8266WiFi or WiFi.h for ESP32)
- Web server libraries (ESP8266WebServer or WebServer for ESP32)
- mDNS libraries (ESP8266mDNS or ESPmDNS for ESP32)
- DNSServer library
- EEPROM library
- ArduinoJson library (for API responses)

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