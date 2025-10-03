/**
 * Basic WebPlatform Application Example
 *
 * This example shows how to create a simple embedded web application using
 * WebPlatform. It demonstrates:
 * - Basic WebPlatform initialization with debug logging
 * - Navigation menu setup using PROGMEM-friendly patterns
 * - Custom route registration with current authentication patterns
 * - Static asset serving with proper content types
 * - WiFi configuration handling
 * - OpenAPI documentation patterns (conditional compilation)
 * - Build flag usage (WEB_PLATFORM_DEBUG and WEB_PLATFORM_OPENAPI)
 */

#include <Arduino.h>
#include <web_platform.h>

// Optional: Include your custom modules
// #include <sensor_module.h>
// SensorModule sensorModule;

void setupApplicationRoutes() {
  Serial.println("Setting up custom application routes...");
  // Register custom home page using current patterns
  webPlatform.registerWebRoute(
      "/",
      [](WebRequest &req, WebResponse &res) {
        String html = R"(
<!DOCTYPE html>
<html><head>
    <title>My Device - Home</title>
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head><body>
    <div class="container">
        {{NAV_MENU}}
        <h1>Welcome to My Device</h1>
        <p>This is a simple embedded web application built with WebPlatform.</p>
        
        <div class="status-grid">
            <div class="status-card success">
                <h3>Device Status</h3>
                <p class="large-text">Online</p>
            </div>
            <div class="status-card info">
                <h3>Uptime</h3>
                <p class="large-text">)" +
                      String(millis() / 1000) + R"( seconds</p>
            </div>
            <div class="status-card info">
                <h3>Free Memory</h3>
                <p class="large-text">)" +
                      String(ESP.getFreeHeap()) + R"( bytes</p>
            </div>
        </div>
        
        <div class="card">
            <h2>Quick Actions</h2>
            <button class="btn btn-primary" onclick="window.location='/about'">About This Device</button>
            <button class="btn btn-secondary" onclick="window.location='/settings'">Device Settings</button>
        </div>
    </div>
</body></html>
      )";
        res.setContent(html, "text/html");
      },
      {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Register about page
  webPlatform.registerWebRoute(
      "/about",
      [](WebRequest &req, WebResponse &res) {
        String html = R"(
<!DOCTYPE html>
<html><head>
    <title>About My Device</title>
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head><body>
    <div class="container">
        {{NAV_MENU}}
        <h1>About My Device</h1>
        
        <div class="card">
            <h2>Device Information</h2>
            <table class="info-table">
                <tr><td>Device Name:</td><td>)" +
                      String(webPlatform.getDeviceName()) + R"(</td></tr>
                <tr><td>Firmware Version:</td><td>1.0.0</td></tr>
                <tr><td>WiFi SSID:</td><td>)" +
                      WiFi.SSID() + R"(</td></tr>
                <tr><td>IP Address:</td><td>)" +
                      WiFi.localIP().toString() + R"(</td></tr>
                <tr><td>Hostname:</td><td>)" +
                      webPlatform.getHostname() + R"(</td></tr>
                <tr><td>HTTPS Enabled:</td><td>)" +
                      String(webPlatform.isHttpsEnabled() ? "Yes" : "No") +
                      R"(</td></tr>
            </table>
        </div>
        
        <div class="card">
            <h2>System Stats</h2>
            <table class="info-table">
                <tr><td>Uptime:</td><td>)" +
                      String(millis() / 1000) + R"( seconds</td></tr>
                <tr><td>Free Heap:</td><td>)" +
                      String(ESP.getFreeHeap()) + R"( bytes</td></tr>
                <tr><td>CPU Frequency:</td><td>)" +
                      String(ESP.getCpuFreqMHz()) + R"( MHz</td></tr>
            </table>
        </div>
        
        <a href="/" class="btn btn-primary">Back to Home</a>
    </div>
</body></html>
      )";
        res.setContent(html, "text/html");
      },
      {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Register settings page
  webPlatform.registerWebRoute(
      "/settings",
      [](WebRequest &req, WebResponse &res) {
        String html = R"(
<!DOCTYPE html>
<html><head>
    <title>Device Settings</title>
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
    <script src="/assets/web-platform-utils.js"></script>
</head><body>
    <div class="container">
        {{NAV_MENU}}
        <h1>Device Settings</h1>
        
        <div class="card">
            <h2>WiFi Configuration</h2>
            <p>Current Network: <strong>)" +
                      WiFi.SSID() + R"(</strong></p>
            <p>Signal Strength: <strong>)" +
                      String(WiFi.RSSI()) + R"( dBm</strong></p>
            <button class="btn btn-secondary" onclick="window.location='/wifi'">WiFi Settings</button>
        </div>
        
        <div class="card">
            <h2>System Actions</h2>
            <button class="btn btn-warning" onclick='confirmRestart()'>Restart Device</button>
            <button class="btn btn-danger" onclick='confirmFactoryReset()'>Factory Reset</button>
        </div>
    </div>
    
    <script>
        async function confirmRestart() {
            if (confirm('Are you sure you want to restart the device?')) {
                try {
                    const response = await AuthUtils.fetch("/api/restart", {method: "POST"});
                    const result = await response.json();
                    UIUtils.showAlert('Success', 'Device is restarting...', 'success');
                } catch (error) {
                    UIUtils.showAlert('Error', 'Failed to restart device: ' + error.message, 'error');
                }
            }
        }
        
        async function confirmFactoryReset() {
            if (confirm("Are you sure you want to factory reset? This will erase all settings!")) {
                if (confirm("This action cannot be undone. Continue?")) {
                    try {
                        const response = await AuthUtils.fetch("/api/factory-reset", {method: "POST"});
                        const result = await response.json();
                        UIUtils.showAlert('Success', 'Factory reset initiated...', 'success');
                    } catch (error) {
                        UIUtils.showAlert('Error', 'Failed to factory reset: ' + error.message, 'error');
                    }
                }
            }
        }
    </script>
</body></html>
      )";
        res.setContent(html, "text/html");
      },
      {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Add API endpoints with proper documentation and error handling
  webPlatform.registerApiRoute(
      "/restart",
      [](WebRequest &req, WebResponse &res) {
        res.setContent(R"({"success":true,"message":"Restarting device..."})",
                       "application/json");

        DEBUG_PRINTLN("Device restart requested via API");

        // Restart after a short delay to send response first
        delay(1000);
        ESP.restart();
      },
      {AuthType::LOCAL_ONLY}, WebModule::WM_POST,
      API_DOC("Restart device", "Restarts the device immediately. All current "
                                "connections will be terminated."));

  webPlatform.registerApiRoute(
      "/factory-reset",
      [](WebRequest &req, WebResponse &res) {
        DEBUG_PRINTLN("Factory reset requested via API");

        // Clear WiFi credentials
        webPlatform.resetWiFiCredentials();

        res.setContent(
            R"({"success":true,"message":"Factory reset complete. Restarting..."})",
            "application/json");

        // Restart after a short delay to send response first
        delay(1000);
        ESP.restart();
      },
      {AuthType::LOCAL_ONLY}, WebModule::WM_POST,
      API_DOC("Factory reset device",
              "Clears all WiFi credentials and restarts the device in "
              "configuration mode."));
}

void setup() {
  Serial.begin(115200);
  DEBUG_PRINTLN("Starting Basic WebPlatform Application...");

  // Set up the navigation menu using PROGMEM-friendly const char* strings
  std::vector<NavigationItem> navItems = {
      NavigationItem("Home", "/"), NavigationItem("About", "/about"),
      NavigationItem("Settings", "/settings")};
  webPlatform.setNavigationMenu(navItems);

  // Set up custom error pages (optional)
  webPlatform.setErrorPage(404, R"(
        <html><head><title>Page Not Found</title></head><body>
            <h1>404 - Page Not Found</h1>
            <p>The requested page could not be found.</p>
            <a href="/">Return Home</a>
        </body></html>
    )");

  // Register modules if you have any
  // webPlatform.registerModule("/sensors", &sensorModule);

  // Add URL redirects (optional)
  webPlatform::addRedirect("/config", "/settings");
  webPlatform::addRedirect("/home", "/");

  // Initialize WebPlatform with device name
  DEBUG_PRINTLN("Initializing WebPlatform...");
  webPlatform.begin("MyDevice", "1.0.0");

  // register application routes after webPlatform.begin() this allows
  // the webPlatform to register its internal routes first so that
  // if you reregister the same route (ex /assets/style.css /asset/favicon.ico)
  // your registrations will override the defaults
  setupApplicationRoutes();

  // Demonstrate storage system usage
  if (webPlatform.isConnected()) {
    // Store some basic configuration using the default JSON driver (fast
    // access)
    StorageManager::query("config").store("my_config", "test123");

    // Example of using LittleFS driver for larger data
    String deviceInfo =
        R"({"model":"ESP32","firmware":"1.0.0","features":["wifi","https","webui"]})";
    StorageManager::driver("littlefs").store("device", "info", deviceInfo);
  }

  // Log initialization results
  if (webPlatform.isConnected()) {
    DEBUG_PRINTLN("=== Basic WebPlatform Application Ready ===");
    DEBUG_PRINT("Device URL: ");
    DEBUG_PRINTLN(webPlatform.getBaseUrl());
    DEBUG_PRINT("HTTPS Enabled: ");
    DEBUG_PRINTLN(webPlatform.isHttpsEnabled() ? "Yes" : "No");
    DEBUG_PRINT("Registered Routes: ");
    DEBUG_PRINTLN(webPlatform.getRouteCount());
#if WEB_PLATFORM_OPENAPI
    DEBUG_PRINTLN("OpenAPI Documentation: Enabled");
#else
    DEBUG_PRINTLN("OpenAPI Documentation: Disabled (enable with "
                  "-DWEB_PLATFORM_OPENAPI=1)");
#endif
  } else {
    DEBUG_PRINTLN("=== Running in WiFi Configuration Mode ===");
    DEBUG_PRINT("Connect to WiFi network: ");
    DEBUG_PRINTLN(webPlatform.getAPName());
    DEBUG_PRINTLN("Open browser to configure WiFi settings");
  }
}

void loop() {
  // Handle all WebPlatform operations
  webPlatform.handle();

  // Optional: Add some indication of operation with different blink rates
  static unsigned long lastBlink = 0;
  unsigned long blinkInterval =
      webPlatform.isConnected()
          ? 2000
          : 500; // Slow when connected, fast when configuring

  if (millis() - lastBlink > blinkInterval) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}