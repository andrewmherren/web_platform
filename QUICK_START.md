# WebPlatform Quick Start

Get your ESP32 web application running in minutes!

## 1. Install Dependencies

Add to your `platformio.ini`:

```ini
lib_deps = 
  https://github.com/andrewmherren/web_platform.git
  bblanchon/ArduinoJson@^6.20.0
```

## 2. Basic Setup

```cpp
#include <web_platform.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize WebPlatform
  webPlatform.begin("MyDevice");
  
  if (webPlatform.isConnected()) {
    // Add your routes here
    webPlatform.registerWebRoute("/", [](WebRequest& req, WebResponse& res) {
      res.setContent("Hello WebPlatform!", "text/html");
    });
  }
}

void loop() {
  webPlatform.handle();
}
```

## 3. Access Your Device

1. **First Boot**: Connect to `MyDeviceSetup` WiFi network, configure WiFi
2. **After Setup**: Visit `http://mydevice.local` or device IP address
3. **Default Login**: Username: `admin`, Password: `admin`

## 4. Next Steps

- 📖 **[Complete Guide](GUIDE.md)** - Comprehensive documentation
- 🔧 **[Configuration](platformio.ini.example)** - Example platformio.ini
- 🚀 **[Examples Repository](examples/README.md)** - Complete project examples
- 🔐 **[Security Guide](SECURITY.md)** - Production deployment tips

## 5. Key Features

- ✅ **WiFi Management** - Automatic captive portal setup
- ✅ **Authentication** - Session + token-based auth with admin panel
- ✅ **HTTPS Support** - Automatic detection and certificate management
- ✅ **Module System** - Reusable web modules with isolated namespaces
- ✅ **Storage System** - Flexible database drivers (JSON + LittleFS)
- ✅ **OpenAPI** - Optional API documentation generation
- ✅ **Built-in UI** - Glass morphism theme with navigation system

## Help & Support

- 🐛 **Issues**: [GitHub Issues](https://github.com/andrewmherren/web_platform/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/andrewmherren/web_platform/discussions)
- 📧 **Contact**: your-email@example.com