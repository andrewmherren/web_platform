/**
 * Basic WebPlatform Application Example
 *
 * This example shows how to create a simple embedded web application using
 * WebPlatform. It demonstrates:
 * - Basic WebPlatform initialization
 * - Navigation menu setup
 * - Custom route registration
 * - Static asset serving
 * - WiFi configuration handling
 */

#include <Arduino.h>
#include <web_platform.h>

// Optional: Include your custom modules
// #include <sensor_module.h>
// SensorModule sensorModule;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Basic WebPlatform Application...");

  // Set up the navigation menu
  std::vector<NavigationItem> navItems = {
      NavigationItem("Home", "/"), NavigationItem("About", "/about"),
      NavigationItem("Settings", "/settings")};
  IWebModule::setNavigationMenu(navItems);

  // Set up custom error pages (optional)
  IWebModule::setErrorPage(404, R"(
        <html><head><title>Page Not Found</title></head><body>
            <h1>404 - Page Not Found</h1>
            <p>The requested page could not be found.</p>
            <a href="/">Return Home</a>
        </body></html>
    )");

  // Register routes before calling webPlatform.begin()

  // Register custom home page
  webPlatform.overrideRoute("/", [](WebRequest &req, WebResponse &res) {
    String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>My Device - Home</title>
                <link rel="stylesheet" href="/assets/style.css">
            </head><body>
                <div class="container">
                    <h1>Welcome to My Device</h1>
                    {{NAV_MENU}}
                    <p>This is a simple embedded web application built with WebPlatform.</p>
                    
                    <div class="status-grid">
                        <div class="status-card">
                            <h3>Device Status</h3>
                            <p class="success">Online</p>
                        </div>
                        <div class="status-card">
                            <h3>Uptime</h3>
                            <p>)" +
                  String(millis() / 1000) + R"( seconds</p>
                        </div>
                        <div class="status-card">
                            <h3>Free Memory</h3>
                            <p>)" +
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
  });

  // Register about page
  webPlatform.registerRoute("/about", [](WebRequest &req, WebResponse &res) {
    String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>About My Device</title>
                <link rel="stylesheet" href="/assets/style.css">
            </head><body>
                <div class="container">
                    <h1>About My Device</h1>
                    {{NAV_MENU}}
                    <div class="card">
                        <h2>Device Information</h2>
                        <table class="info-table">
                            <tr><td>Device Name:</td><td>)" +
                  String(webPlatform.getDeviceName()) + R"(</td></tr>
                            <tr><td>Firmware Version:</td><td>1.0.0</td></tr>
                            <tr><td>Platform:</td><td>)" +
#ifdef ESP32
                  "ESP32" +
#elif defined(ESP8266)
                            "ESP8266" +
#endif
                  R"(</td></tr>
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
  });

  // Register settings page
  webPlatform.registerRoute("/settings", [](WebRequest &req, WebResponse &res) {
    String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>Device Settings</title>
                <link rel="stylesheet" href="/assets/style.css">
            </head><body>
                <div class="container">
                    <h1>Device Settings</h1>
                    {{NAV_MENU}}
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
                    function confirmRestart() {
                        if (confirm('Are you sure you want to restart the device?')) {
                            fetch("/api/restart", {method: "POST"})
                                .then(() => alert("Device is restarting..."))
                                .catch(err => alert("Error: " + err));
                        }
                    }
                    
                    function confirmFactoryReset() {
                        if (confirm("Are you sure you want to factory reset? This will erase all settings!")) {
                            if (confirm("This action cannot be undone. Continue?")) {
                                fetch("/api/factory-reset", {method: "POST"})
                                    .then(() => alert("Factory reset initiated..."))
                                    .catch(err => alert("Error: " + err));
                            }
                        }
                    }
                </script>
            </body></html>
        )";
    res.setContent(html, "text/html");
  });

  // Add API endpoints
  webPlatform.registerRoute(
      "/api/restart",
      [](WebRequest &req, WebResponse &res) {
        if (req.getMethod() != WebModule::WM_POST) {
          res.setStatus(405);
          res.setContent("{\"error\":\"Method not allowed\"}",
                         "application/json");
          return;
        }

        res.setContent(
            "{\"success\":true,\"message\":\"Restarting device...\"}",
            "application/json");

        // Restart after a short delay
        delay(1000);
        ESP.restart();
      },
      {AuthType::NONE}, WebModule::WM_POST);

  webPlatform.registerRoute(
      "/api/factory-reset",
      [](WebRequest &req, WebResponse &res) {
        if (req.getMethod() != WebModule::WM_POST) {
          res.setStatus(405);
          res.setContent("{\"error\":\"Method not allowed\"}",
                         "application/json");
          return;
        }

        // Clear WiFi credentials
        webPlatform.resetWiFiCredentials();

        res.setContent("{\"success\":true,\"message\":\"Factory reset "
                       "complete. Restarting...\"}",
                       "application/json");

        // Restart after a short delay
        delay(1000);
        ESP.restart();
      },
      {AuthType::NONE}, WebModule::WM_POST);

  // Register modules if you have any
  // webPlatform.registerModule("/sensors", &sensorModule);

  // Add URL redirects (optional)
  IWebModule::addRedirect("/config", "/settings");
  IWebModule::addRedirect("/home", "/");

  // Initialize WebPlatform with device name
  webPlatform.begin("MyDevice");

  // Only register application routes when connected to WiFi
  if (webPlatform.isConnected()) {
    Serial.print("Application ready at: ");
    Serial.println(webPlatform.getBaseUrl());
  } else {
    Serial.println("Running in WiFi configuration mode");
    Serial.print("Connect to WiFi network: ");
    Serial.println(webPlatform.getAPName());
    Serial.println("Open browser to configure WiFi settings");
  }
}

void loop() {
  // Handle all WebPlatform operations
  webPlatform.handle();

  // Handle your modules (only when connected)
  if (webPlatform.isConnected()) {
    // sensorModule.handle();
  }

  // Optional: Add some indication of operation
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > (webPlatform.isConnected() ? 1000 : 250)) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}