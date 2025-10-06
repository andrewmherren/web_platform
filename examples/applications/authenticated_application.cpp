/**
 * Authenticated WebPlatform Application Example
 *
 * This example demonstrates how to create a secure web application with user
 * authentication using current WebPlatform patterns. It shows:
 * - Session-based authentication for web interface
 * - API token authentication for programmatic access
 * - CSRF protection for forms using AuthUtils.js
 * - Route-level security with proper AuthRequirements
 * - OpenAPI documentation with conditional compilation
 * - Debug logging with build flags
 * - NTP time synchronization with automatic DST
 * - Modern JavaScript patterns with UIUtils and AuthUtils
 */

#include <Arduino.h>
#include <web_platform.h>

// Optional: Include your secure modules
// #include <secure_device_module.h>
// SecureDeviceModule secureModule;

void setupApplicationRoutes() {
  Serial.println("Setting up custom application routes...");

  // Protected dashboard (requires login)
  webPlatform.registerWebRoute(
      "/",
      [](WebRequest &req, WebResponse &res) {
        const AuthContext &auth = req.getAuthContext();

        String html =
            R"(<!DOCTYPE html>
            <html><head>
                <title>Secure Dashboard</title>
                <link rel="stylesheet" href="/assets/style.css">
                <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
                <link rel="icon" href="/assets/favicon.ico" sizes="any">
                <script src="/assets/web-platform-utils.js"></script>
            </head><body>
                <div class="container">
                    <h1>Welcome, )" +
            auth.username + R"(!</h1>
                    <p>You are successfully logged into the secure dashboard.</p>
                    {{NAV_MENU}}
                    <p></p>
                    <div class="status-grid">
                        <div class="status-card">
                            <h3>Authentication</h3>
                            <p class="success">Logged In</p>
                        </div>
                        <div class="status-card">
                            <h3>Session Type</h3>
                            <p>)" +
            String(auth.authenticatedVia == AuthType::SESSION ? "Web Session"
                                                              : "API Token") +
            R"(</p>
                        </div>
                        <div class="status-card">
                            <h3>Access Level</h3>
                            <p>)" +
            String(auth.username == "admin" ? "Administrator" : "User") +
            R"(</p>
                        </div>
                    </div><div class="card">
                        <h2>System Information</h2>
                        <table class="info-table">
                            <tr><td>Device:</td><td>)" +
            String(webPlatform.getDeviceName()) + R"(</td></tr>
                            <tr><td>Uptime:</td><td>)" +
            String(millis() / 1000) + R"( seconds</td></tr>
                            <tr><td>Free Memory:</td><td>)" +
            String(ESP.getFreeHeap()) + R"( bytes</td></tr>
                            <tr><td>HTTPS:</td><td>)" +
            String(webPlatform.isHttpsEnabled() ? "Enabled" : "Disabled") +
            R"(</td></tr>
                            <tr><td>NTP Status:</td><td>)" +
            String(NTPClient::isSynchronized() ? "Synchronized"
                                               : "Not synchronized") +
            R"(</td></tr><tr><td>Current Time:</td><td><span id="current-time">)" +
            (NTPClient::isSynchronized() ? NTPClient::getFormattedTime()
                                         : "Unknown") +
            R"(</span></td></tr></td></tr>
                        </table></div>
                </div>
                
                <script>
                    // Localize the current time display if available
                    document.addEventListener('DOMContentLoaded', function() {
                        const currentTimeElement = document.getElementById('current-time');
                        if (currentTimeElement && currentTimeElement.textContent !== 'Unknown') {
                            // The server provides UTC timestamp, convert to local time
                            const utcTime = currentTimeElement.textContent;
                            if (utcTime.includes('T') && utcTime.includes('Z')) {
                                const localTime = TimeUtils.formatTime(utcTime);
                                currentTimeElement.textContent = localTime;
                            }
                        }
                    });
                </script>
            </body></html>
        )";
        res.setContent(html, "text/html");
      },
      {AuthType::SESSION});

  // Device control page with CSRF-protected forms
  webPlatform.registerWebRoute(
      "/control",
      [](WebRequest &req, WebResponse &res) {
        String html = R"(<!DOCTYPE html>
            <html><head>
                <title>Device Control</title>
                <link rel="stylesheet" href="/assets/style.css">
                <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
                <link rel="icon" href="/assets/favicon.ico" sizes="any">
            </head><body>
                <div class="container">
                    {{NAV_MENU}}
                    <h1>Device Control Panel</h1>
                    <p></p>
                    <div class="card">
                        <h2>System Controls</h2>
                        <form id="control-form" method="post" action="/api/control">
                            <div class="form-group">
                                <label for="command">Command:</label>
                                <select id="command" name="command" class="form-control" required>
                                    <option value="">Select command...</option>
                                    <option value="status">Get Status</option>
                                    <option value="restart">Restart Device</option>
                                    <option value="reset-wifi">Reset WiFi</option>
                                </select>
                            </div>
                            <button type="submit" class="btn btn-primary">Execute Command</button>
                        </form>
                        
                        <div id="result" class="mt-3"></div>
                    </div>
                </div>
                <script src="/assets/web-platform-utils.js"></script>
                <script>
                    // Handle control form submission
                    document.getElementById('control-form').addEventListener('submit', async function(e) {
                        e.preventDefault();
                        
                        const formData = new FormData(this);
                        const command = formData.get('command');try {
                            const result = await AuthUtils.fetch('/api/control', {
                                method: 'POST',
                                headers: {
                                    'Content-Type': 'application/x-www-form-urlencoded'
                                },
                                body: 'command=' + encodeURIComponent(command)
                            }).then(response => response.json());
                            
                            document.getElementById('result').innerHTML = 
                                '<div class="card ' + (result.success ? 'success' : 'error') + '">' +
                                '<p>' + (result.message || result.error || 'Operation completed') + '</p>' +
                                '</div>';
                        } catch (error) {
                            document.getElementById('result').innerHTML = 
                                '<div class="card error"><p>Error: ' + error.message + '</p></div>';
                        }
                    });
                    
                    // Handle config form submission
                    const configForm = document.getElementById('config-form');
                    if (configForm) {
                        configForm.addEventListener('submit', async function(e) {
                            e.preventDefault();
                            
                            const formData = new FormData(this);
                            
                            try {
                                const result = await AuthUtils.fetch('/api/configure', {
                                    method: 'POST',
                                    body: formData
                                }).then(response => response.json());
                                
                                if (result.success) {
                                    UIUtils.showAlert('Configuration Saved', 'Configuration has been saved successfully!', 'success');
                                } else {
                                    UIUtils.showAlert('Save Failed', 'Error: ' + result.error, 'error');
                                }
                            } catch (error) {
                                UIUtils.showAlert('Network Error', 'Error: ' + error.message, 'error');
                            }
                        });
                    }
                </script>
            </body></html>
        )";
        res.setContent(html, "text/html");
      },
      {AuthType::SESSION, AuthType::PAGE_TOKEN, AuthType::TOKEN});

  // API Examples and Documentation page
  webPlatform.registerWebRoute("/examples",
                               [](WebRequest &req, WebResponse &res) {
                                 String html =
                                     R"(
            <!DOCTYPE html>
            <html><head>
                <title>API Examples & Documentation</title>
                <link rel="stylesheet" href="/assets/style.css">
                <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
                <link rel="icon" href="/assets/favicon.ico" sizes="any">
            </head><body>
                <div class="container">
                    {{NAV_MENU}}
                    <h1>API Examples & Documentation</h1>
                    <p></p>
                    <div class="card">
                        <h2>Getting Started</h2>
                        <p>To use the API endpoints, you'll need an API token. Follow these steps:</p>
                        <ol>
                            <li>Go to the <strong>Account</strong> page</li>
                            <li>Navigate to the <strong>API Tokens</strong> section</li>
                            <li>Click <strong>"Create New Token"</strong></li>
                            <li>Enter a description (e.g., "Home Assistant Integration")</li>
                            <li>Copy and save your token securely - it won't be shown again!</li>
                        </ol>
                    </div>
                    
                    <div class="card">
                        <h2>cURL Examples</h2>
                        <p>Use your API token with these endpoints:</p>
                        <pre class="code-block">
# Get device status
curl --insecure -H "Authorization: Bearer YOUR_TOKEN" )" +
                                     webPlatform.getBaseUrl() +
                                     R"(/api/status

# Execute a command  
curl --insecure -X POST -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"command":"status"}' \
  )" + webPlatform.getBaseUrl() +
                                     R"(/api/control

# Alternative: Using URL parameter
curl --insecure ")" + webPlatform.getBaseUrl() +
                                     R"(/api/status?access_token=YOUR_TOKEN"
                        </pre>
                    </div>
                    
                    <div class="card">
                        <h2>JavaScript/Fetch Example</h2>
                        <pre class="code-block">
const token = 'YOUR_API_TOKEN_HERE';
const baseUrl = ')" + webPlatform.getBaseUrl() +
                                     R"(';

// Get device status
fetch(baseUrl + '/api/status', {
    headers: {
        'Authorization': 'Bearer ' + token
    }
})
.then(response => response.json())
.then(data => {
    console.log('Device Status:', data);
    console.log('Uptime:', data.uptime + ' seconds');
    console.log('Free Memory:', data.free_memory + ' bytes');
});

// Send a command
fetch(baseUrl + '/api/control', {
    method: 'POST',
    headers: {
        'Authorization': 'Bearer ' + token,
        'Content-Type': 'application/json'
    },
    body: JSON.stringify({
        command: 'status'
    })
})
.then(response => response.json())
.then(data => console.log('Command result:', data));
                        </pre>
                    </div>
                    
                    <div class="card">
                        <h2>Python Example</h2>
                        <pre class="code-block">
import requests
import json

# Your API token from the Account page
token = 'YOUR_API_TOKEN_HERE'
base_url = ')" + webPlatform.getBaseUrl() +
                                     R"('
headers = {'Authorization': f'Bearer {token}'}

# Get device status
response = requests.get(f'{base_url}/api/status', headers=headers)
if response.status_code == 200:
    data = response.json()
    print(f"Device: {data['device']}")
    print(f"Uptime: {data['uptime']} seconds")
    print(f"Free Memory: {data['free_memory']} bytes")
else:
    print(f"Error: {response.status_code}")

# Send a command
command_data = {'command': 'status'}
response = requests.post(
    f'{base_url}/api/control',
    headers={**headers, 'Content-Type': 'application/json'},
    data=json.dumps(command_data)
)

if response.status_code == 200:
    result = response.json()
    print(f"Success: {result['success']}")
    print(f"Message: {result.get('message', 'No message')}")
else:
    print(f"Error: {response.status_code}")
                        </pre>
                    </div>
                    
                    <div class="card">
                        <h2>Available Endpoints</h2>
                        <table class="info-table">
                            <tr>
                                <th>Endpoint</th>
                                <th>Method</th>
                                <th>Description</th>
                            </tr>
                            <tr>
                                <td><code>/api/status</code></td>
                                <td>GET</td>
                                <td>Get device status information</td>
                            </tr>
                            <tr>
                                <td><code>/api/control</code></td>
                                <td>POST</td>
                                <td>Send commands to the device</td>
                            </tr>
                            <tr>
                                <td><code>/api/configure</code></td>
                                <td>POST</td>
                                <td>Update device configuration</td>
                            </tr>
                        </table>
                    </div>
                    
                    <div class="card">
                        <h2>Available Commands</h2>
                        <table class="info-table">
                            <tr>
                                <th>Command</th>
                                <th>Description</th>
                            </tr>
                            <tr>
                                <td><code>status</code></td>
                                <td>Check if device is operational</td>
                            </tr>
                            <tr>
                                <td><code>restart</code></td>
                                <td>Restart the device</td>
                            </tr>
                            <tr>
                                <td><code>reset-wifi</code></td>
                                <td>Clear WiFi credentials and restart</td>
                            </tr>
                        </table>
                    </div>
                </div>
            </body></html>
        )";
                                 res.setContent(html, "text/html");
                               },
                               {AuthType::SESSION});

  // API Endpoints - accessible via both session and token auth
  // Status API (can be called from web interface or via API token)
  webPlatform.registerApiRoute(
      "/status",
      [](WebRequest &req, WebResponse &res) {
        JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
          json["success"] = true;
          json["device"] = webPlatform.getDeviceName();
          json["uptime"] = millis() / 1000;
          json["free_memory"] = ESP.getFreeHeap();
          json["wifi_ssid"] = WiFi.SSID();
          json["ip_address"] = WiFi.localIP().toString();
          json["https_enabled"] = webPlatform.isHttpsEnabled();
          json["ntp_synchronized"] = NTPClient::isSynchronized();
          json["current_time"] = NTPClient::isSynchronized()
                                     ? NTPClient::getFormattedTime()
                                     : "Not synchronized";
          json["time_since_sync"] = NTPClient::getTimeSinceLastSync() / 1000;
        });
      },
      {AuthType::SESSION, AuthType::PAGE_TOKEN, AuthType::TOKEN},
      WebModule::WM_GET,
      API_DOC("Get device status",
              "Returns comprehensive device status including system info, WiFi "
              "details, and NTP sync status."));

  // Control API with CSRF protection for web forms
  webPlatform.registerApiRoute(
      "/control",
      [](WebRequest &req, WebResponse &res) {
        if (req.getMethod() != WebModule::WM_POST) {
          res.setStatus(405);
          res.setContent(R"({"error":"Method not allowed"})",
                         "application/json");
          return;
        }

        String result;
        // try getting the command as a route parameter
        String command = req.getParam("command");
        if (command.isEmpty()) {
          // if it wasn't in the route params try getting the command from the
          // body json
          command = req.getJsonParam("command");
        }

        if (command == "status") {
          result = R"({"success":true,"message":"Device is operational"})";
        } else if (command == "restart") {
          result =
              R"({"success":true,"message":"Device will restart in 3 seconds"})";
          res.setContent(result, "application/json");
          // Send response first, then restart after delay
          delay(3000);
          ESP.restart();
          return;
        } else if (command == "reset-wifi") {
          webPlatform.resetWiFiCredentials();
          result =
              R"({"success":true,"message":"WiFi credentials cleared. Device will restart."})";
          res.setContent(result, "application/json");
          // Send response first, then restart after delay
          delay(1000);
          ESP.restart();
          return;
        } else {
          res.setStatus(400);
          result = R"({"success":false,"error":"Unknown command"})";
        }

        res.setContent(result, "application/json");
      },
      {AuthType::SESSION, AuthType::TOKEN, AuthType::PAGE_TOKEN},
      WebModule::WM_POST,
      API_DOC("Execute device commands", "Execute system commands like status "
                                         "check, restart, or WiFi reset."));
}

void setup() {
  Serial.begin(115200);
  DEBUG_PRINTLN("Starting Authenticated WebPlatform Application...");

  // Set up navigation menu with authentication-aware
  // items
  std::vector<NavigationItem> navItems = {
      NavigationItem("Dashboard", "/"),
      NavigationItem("Device Control", "/control"),
      NavigationItem("Examples", "/examples"),
      Authenticated(NavigationItem("Account", "/account")),
      NavigationItem("Status", "/status"),
      Authenticated(NavigationItem("Logout", "/logout")),
      Unauthenticated(NavigationItem("Login", "/login"))};
  webPlatform.setNavigationMenu(navItems);

  // Register secure modules
  // webPlatform.registerModule("/secure", &secureModule);

  // Override module routes to add authentication if needed
  // webPlatform.registerWebRoute("/secure/admin", adminHandler,
  // {AuthType::SESSION});

  // Initialize WebPlatform
  DEBUG_PRINTLN("Initializing Secure WebPlatform...");
  webPlatform.begin("SecureDevice", "1.0.0");

  // register application routes after webPlatform.begin() this allows
  // the webPlatform to register its internal routes first so that
  // if you reregister the same route (ex /assets/style.css /asset/favicon.ico)
  // your registrations will override the defaults
  setupApplicationRoutes();

  if (webPlatform.isConnected()) {
    DEBUG_PRINTLN("=== Authenticated WebPlatform Application Ready ===");
    DEBUG_PRINTLN("Initial setup: Create your admin account when prompted");
    DEBUG_PRINT("Device URL: ");
    DEBUG_PRINTLN(webPlatform.getBaseUrl());
    DEBUG_PRINT("HTTPS Enabled: ");
    DEBUG_PRINTLN(webPlatform.isHttpsEnabled() ? "Yes" : "No");
    DEBUG_PRINT("NTP Synchronized: ");
    DEBUG_PRINTLN(NTPClient::isSynchronized() ? "Yes" : "No");
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
  }
}

void loop() {
  webPlatform.handle();

  if (webPlatform.isConnected()) {
    // Handle your secure modules
    // secureModule.handle();
  }

  // Status LED - slow blink when connected, fast when configuring
  static unsigned long lastBlink = 0;
  unsigned long interval = webPlatform.isConnected() ? 2000 : 500;

  if (millis() - lastBlink > interval) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}