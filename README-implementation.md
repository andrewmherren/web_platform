# WebPlatform Implementation Details

This document provides technical details about the implementation of the WebPlatform module.

## File Structure

The module has been broken down into several files to improve maintainability:

- **web_platform.h**: Main header file with class definition
- **web_platform_core.cpp**: Core initialization and setup
- **web_platform_https.cpp**: HTTPS server and certificate handling
- **web_platform_portal.cpp**: Config portal implementation
- **web_platform_wifi.cpp**: WiFi credential management
- **web_platform_connected.cpp**: Connected mode implementation
- **assets/**: Web interface assets (HTML, CSS, JS)

## Phase 2 Implementation

Phase 2 focused on enhancing the WebPlatform with HTTPS support for the configuration portal. The key components implemented are:

### Certificate Detection

The certificate detection logic has been enhanced to work without requiring build flags:

```cpp
bool WebPlatform::areCertificatesAvailable() {
  // Check if embedded certificates exist and are valid
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;
  
  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    return false;
  }
  
  // Validate PEM format
  if (cert_len > 27 && key_len > 27) {
    String certStart((char *)cert_data, 27);
    String keyStart((char *)key_data, 27);
    
    if (certStart.indexOf("-----BEGIN CERTIFICATE-----") >= 0 &&
        keyStart.indexOf("-----BEGIN") >= 0) {
      return true;
    }
  }
  
  return false;
}
```

### HTTPS Server Configuration

For ESP32-S3, a full HTTPS server is configured:

```cpp
void WebPlatform::configureHttpsServer() {
  // Get certificate data
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;
  
  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    httpsEnabled = false;
    return;
  }
  
  // Configure HTTPS server
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.httpd.server_port = serverPort;
  config.httpd.max_uri_handlers = 50;
  config.httpd.task_priority = 5;
  config.httpd.stack_size = 8192;
  config.httpd.lru_purge_enable = true;
  
  // Set certificates
  config.cacert_pem = cert_data;
  config.cacert_len = cert_len;
  config.prvtkey_pem = key_data;
  config.prvtkey_len = key_len;
  
  // Start HTTPS server
  esp_err_t ret = httpd_ssl_start(&httpsServerHandle, &config);
  if (ret != ESP_OK) {
    httpsEnabled = false;
    return;
  }
  
  registerHttpsRoutes();
}
```

### HTTPS Route Handling

HTTPS routes are handled via a generic handler:

```cpp
esp_err_t WebPlatform::httpsGenericHandler(httpd_req_t *req) {
  String uri = String(req->uri);
  
  // Handle static assets
  StaticAsset asset = IWebModule::getStaticAsset(uri);
  if (asset.path.length() > 0) {
    // Serve static asset
    return ESP_OK;
  }
  
  // Handle dynamic routes based on current mode
  if (httpsInstance->currentMode == CONFIG_PORTAL) {
    return httpsInstance->handleHttpsConfigPortal(req);
  } else {
    return httpsInstance->handleHttpsConnected(req);
  }
}
```

### Enhanced Config Portal UI

The config portal now includes a security notice based on the connection type (HTTP vs HTTPS):

```cpp
String WebPlatform::handleConfigPortalRoot() {
  String html = FPSTR(CONFIG_PORTAL_HTML);
  
  // Replace placeholder with device name
  html.replace("{{DEVICE_NAME}}", deviceName);
  
  // Add appropriate security notice based on protocol
  String securityNotice;
  if (httpsEnabled) {
    securityNotice = R"(
    <div class="security-notice https">
        <h4><span class="security-icon-large">🔒</span> Secure Connection</h4>
        <p>This connection is secured with HTTPS encryption. Your WiFi password will be transmitted securely.</p>
    </div>)";
  } else {
    securityNotice = R"(
    <div class="security-notice">
        <h4><span class="security-icon-large">ℹ️</span> Connection Notice</h4>
        <p>This is a direct device connection. Only enter WiFi credentials on your trusted private network.</p>
    </div>)";
  }
  html.replace("{{SECURITY_NOTICE}}", securityNotice);
  
  return IWebModule::injectNavigationMenu(html);
}
```

### Asset Extraction

Web assets have been extracted to separate header files:

- **config_portal_html.h**: HTML templates for the configuration portal
- **config_portal_css.h**: CSS styles for a better user experience
- **config_portal_js.h**: Enhanced JavaScript for WiFi network scanning

## Implementation Notes

### Platform Support

- **ESP32-S3**: Full support with HTTPS for both config portal and connected mode
- **ESP32**: HTTP support only (no HTTPS implementation yet)
- **ESP8266**: HTTP support only (HTTPS not possible due to platform limitations)

### Memory Usage

HTTPS functionality adds approximately 15-20KB to the binary size and requires more RAM at runtime. The implementation carefully checks for sufficient resources and falls back to HTTP if HTTPS cannot be enabled.

### Code Organization

The code has been refactored from the original web_platform.cpp (which was over 1200 lines) into smaller, more manageable files:

1. **web_platform_core.cpp**: Core functionality (~250 lines)
2. **web_platform_https.cpp**: HTTPS-specific code (~350 lines)
3. **web_platform_portal.cpp**: Config portal implementation (~200 lines)
4. **web_platform_wifi.cpp**: WiFi management (~200 lines)
5. **web_platform_connected.cpp**: Connected mode (~250 lines)

This organization improves maintainability and makes it easier to enhance specific aspects of the platform.

### Migration Path

The original web_platform.cpp file should be removed, as its functionality has been distributed across the new files. The web_platform.h header remains as the single interface point for application code.

## Next Steps

For Phase 3, the focus will be on:

1. Enhanced route management for more efficient handling
2. Better memory usage and optimization
3. Expanded module integration capabilities
4. Improved error handling and user feedback

## Usage in main.cpp

The WebPlatform is now ready for use in main.cpp with the enhanced HTTPS capabilities:

```cpp
#include <Arduino.h>
#include <web_platform.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize foundation systems
  IWebModule::initializeDefaultTheme();
  
  // Initialize WebPlatform with device name
  webPlatform.begin("TickerTape");
  
  // In connected mode, register modules
  if (webPlatform.isConnected()) {
    usbPDController.begin();
    webPlatform.registerModule("/usb_pd", &usbPDController);
  }
}

void loop() {
  // Single handle call manages everything
  webPlatform.handle();
  
  // Module-specific handling when connected
  if (webPlatform.isConnected()) {
    usbPDController.handle();
  }
}
```