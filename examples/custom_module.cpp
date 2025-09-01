/**
 * Custom WebModule Example
 * 
 * This example shows how to create a custom web module that implements the IWebModule interface.
 * It demonstrates:
 * - IWebModule interface implementation
 * - Route definition with authentication requirements
 * - API endpoints for external access
 * - Web interface with forms and CSRF protection
 * - Static asset serving
 * - Module state management
 */

#include <Arduino.h>
#include <web_platform.h>
#include <ArduinoJson.h>

/**
 * Example: Environmental Sensor Module
 * 
 * This module simulates an environmental monitoring system with:
 * - Temperature and humidity readings
 * - Configuration settings
 * - Data logging capabilities
 * - Public and protected endpoints
 */
class EnvironmentalSensorModule : public IWebModule {
private:
    // Simulated sensor data
    float temperature = 23.5;
    float humidity = 45.2;
    bool sensorEnabled = true;
    String sensorLocation = "Office";
    unsigned long lastReading = 0;
    
    // Configuration
    float tempThreshold = 30.0;
    float humidityThreshold = 60.0;
    bool alertsEnabled = true;

public:
    EnvironmentalSensorModule() {
        updateSensorReadings();
    }

    // Required IWebModule methods
    std::vector<WebRoute> getHttpRoutes() override {
        return {
            // Public main page - no authentication required
            WebRoute("/", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    mainPageHandler(req, res);
                }),

            // Public API for current readings - accessible to anyone
            WebRoute("/api/current", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    getCurrentDataHandler(req, res);
                }, {AuthType::NONE}),

            // Protected configuration page - requires login
            WebRoute("/config", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    configPageHandler(req, res);
                }, {AuthType::SESSION}),

            // Protected configuration API - requires login + CSRF for forms
            WebRoute("/api/config", WebModule::WM_POST, 
                [this](WebRequest& req, WebResponse& res) {
                    updateConfigHandler(req, res);
                }, {AuthType::SESSION, AuthType::PAGE_TOKEN}),

            // API endpoint for external systems (requires API token)
            WebRoute("/api/data", WebModule::WM_GET, 
                [this](WebRequest& req, WebResponse& res) {
                    getDataAPIHandler(req, res);
                }, {AuthType::TOKEN}),

            // Administrative control (session or token auth)
            WebRoute("/api/control", WebModule::WM_POST, 
                [this](WebRequest& req, WebResponse& res) {
                    controlAPIHandler(req, res);
                }, {AuthType::SESSION, AuthType::TOKEN}),
        };
    }

    std::vector<WebRoute> getHttpsRoutes() override {
        // Same routes for HTTPS
        return getHttpRoutes();
    }

    String getModuleName() const override {
        return "Environmental Sensor";
    }

    String getModuleVersion() const override {
        return "1.2.0";
    }

    String getModuleDescription() const override {
        return "Environmental monitoring with temperature and humidity sensors";
    }

    // Module-specific public methods
    void begin() {
        Serial.println("Environmental Sensor Module initialized");
        updateSensorReadings();
    }

    void handle() {
        // Update sensor readings periodically
        if (millis() - lastReading > 30000) { // Every 30 seconds
            updateSensorReadings();
        }
    }

private:
    void updateSensorReadings() {
        if (!sensorEnabled) return;

        // Simulate sensor readings with some variation
        temperature = 20.0 + random(0, 150) / 10.0; // 20.0 - 35.0°C
        humidity = 30.0 + random(0, 400) / 10.0;    // 30.0 - 70.0%
        lastReading = millis();

        // Check thresholds and log alerts (in real implementation)
        if (alertsEnabled) {
            if (temperature > tempThreshold) {
                Serial.println("ALERT: Temperature threshold exceeded: " + String(temperature) + "°C");
            }
            if (humidity > humidityThreshold) {
                Serial.println("ALERT: Humidity threshold exceeded: " + String(humidity) + "%");
            }
        }
    }

    void mainPageHandler(WebRequest& req, WebResponse& res) {
        String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>Environmental Sensor</title>
                <link rel="stylesheet" href="/assets/style.css">
                <script>
                    // Auto-refresh data every 30 seconds
                    function updateReadings() {
                        fetch('/sensors/api/current')
                            .then(response => response.json())
                            .then(data => {
                                document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '°C';
                                document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                                document.getElementById('last-update').textContent = 
                                    new Date(data.timestamp * 1000).toLocaleString();
                                
                                // Update status indicators
                                const tempCard = document.getElementById('temp-card');
                                const humCard = document.getElementById('hum-card');
                                
                                tempCard.className = 'status-card ' + 
                                    (data.temperature > )" + String(tempThreshold) + R"( ? 'warning' : 'success');
                                humCard.className = 'status-card ' + 
                                    (data.humidity > )" + String(humidityThreshold) + R"( ? 'warning' : 'success');
                            })
                            .catch(error => console.error('Error updating readings:', error));
                    }
                    
                    // Update on page load and every 30 seconds
                    document.addEventListener('DOMContentLoaded', updateReadings);
                    setInterval(updateReadings, 30000);
                </script>
            </head><body>
                <div class="container">
                    <h1>Environmental Sensor</h1>
                    <p>Location: <strong>)" + sensorLocation + R"(</strong></p>
                    
                    <div class="status-grid">
                        <div id="temp-card" class="status-card )" + String(temperature > tempThreshold ? "warning" : "success") + R"(">
                            <h3>Temperature</h3>
                            <p id="temperature" class="large-text">)" + String(temperature, 1) + R"(°C</p>
                            <small>Threshold: )" + String(tempThreshold, 1) + R"(°C</small>
                        </div>
                        <div id="hum-card" class="status-card )" + String(humidity > humidityThreshold ? "warning" : "success") + R"(">
                            <h3>Humidity</h3>
                            <p id="humidity" class="large-text">)" + String(humidity, 1) + R"(%</p>
                            <small>Threshold: )" + String(humidityThreshold, 1) + R"(%</small>
                        </div>
                        <div class="status-card info">
                            <h3>Sensor Status</h3>
                            <p>)" + String(sensorEnabled ? "Active" : "Disabled") + R"(</p>
                            <small id="last-update">)" + String(millis() / 1000) + R"( seconds ago</small>
                        </div>
                    </div>
                    
                    <div class="card">
                        <h2>Quick Actions</h2>
                        <button class="btn btn-primary" onclick="updateReadings()">Refresh Data</button>
                        <button class="btn btn-secondary" onclick="window.location='/sensors/config'">Configuration</button>
                        <button class="btn btn-info" onclick="showAPIInfo()">API Information</button>
                    </div>
                    
                    <div id="api-info" class="card" style="display:none;">
                        <h2>API Endpoints</h2>
                        <p><strong>Public Access:</strong></p>
                        <ul>
                            <li><code>GET /sensors/api/current</code> - Current sensor readings</li>
                        </ul>
                        <p><strong>API Token Required:</strong></p>
                        <ul>
                            <li><code>GET /sensors/api/data</code> - Detailed sensor data</li>
                            <li><code>POST /sensors/api/control</code> - Control sensor operations</li>
                        </ul>
                    </div>
                </div>
                
                <script>
                    function showAPIInfo() {
                        const apiInfo = document.getElementById('api-info');
                        apiInfo.style.display = apiInfo.style.display === 'none' ? 'block' : 'none';
                    }
                </script>
            </body></html>
        )";
        res.setContent(html, "text/html");
    }

    void getCurrentDataHandler(WebRequest& req, WebResponse& res) {
        // Public endpoint - anyone can access current readings
        DynamicJsonDocument doc(200);
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        doc["timestamp"] = millis() / 1000;
        doc["location"] = sensorLocation;
        doc["status"] = sensorEnabled ? "active" : "disabled";

        String json;
        serializeJson(doc, json);
        res.setContent(json, "application/json");
    }

    void configPageHandler(WebRequest& req, WebResponse& res) {
        // Protected configuration page
        String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>Sensor Configuration</title>
                <meta name="csrf-token" content="{{csrfToken}}">
                <link rel="stylesheet" href="/assets/style.css">
            </head><body>
                <div class="container">
                    <h1>Sensor Configuration</h1>
                    
                    <form id="config-form" method="post" action="/sensors/api/config">
                        <div class="card">
                            <h2>Sensor Settings</h2>
                            <div class="form-group">
                                <label for="location">Sensor Location:</label>
                                <input type="text" id="location" name="location" class="form-control" 
                                       value=")" + sensorLocation + R"(" required>
                            </div>
                            <div class="form-group">
                                <label>
                                    <input type="checkbox" name="enabled" )" + String(sensorEnabled ? "checked" : "") + R"(>
                                    Enable Sensor Readings
                                </label>
                            </div>
                        </div>
                        
                        <div class="card">
                            <h2>Alert Thresholds</h2>
                            <div class="form-group">
                                <label for="temp-threshold">Temperature Threshold (°C):</label>
                                <input type="number" id="temp-threshold" name="temp-threshold" 
                                       class="form-control" step="0.1" value=")" + String(tempThreshold, 1) + R"(" required>
                            </div>
                            <div class="form-group">
                                <label for="humidity-threshold">Humidity Threshold (%):</label>
                                <input type="number" id="humidity-threshold" name="humidity-threshold" 
                                       class="form-control" step="0.1" value=")" + String(humidityThreshold, 1) + R"(" required>
                            </div>
                            <div class="form-group">
                                <label>
                                    <input type="checkbox" name="alerts-enabled" )" + String(alertsEnabled ? "checked" : "") + R"(>
                                    Enable Threshold Alerts
                                </label>
                            </div>
                        </div>
                        
                        <button type="submit" class="btn btn-primary">Save Configuration</button>
                        <a href="/sensors/" class="btn btn-secondary">Cancel</a>
                    </form>
                    
                    <div id="result"></div>
                </div>

                <script>
                    document.getElementById('config-form').addEventListener('submit', async function(e) {
                        e.preventDefault();
                        
                        const formData = new FormData(this);
                        const csrf = document.querySelector('meta[name="csrf-token"]').getAttribute('content');
                        formData.append('_csrf', csrf);
                        
                        try {
                            const response = await fetch('/sensors/api/config', {
                                method: 'POST',
                                body: formData,
                                credentials: 'same-origin'
                            });
                            
                            const result = await response.json();
                            
                            if (result.success) {
                                document.getElementById('result').innerHTML = 
                                    '<div class="card success"><p>Configuration saved successfully!</p></div>';
                                setTimeout(() => window.location = '/sensors/', 2000);
                            } else {
                                document.getElementById('result').innerHTML = 
                                    '<div class="card error"><p>Error: ' + result.error + '</p></div>';
                            }
                        } catch (error) {
                            document.getElementById('result').innerHTML = 
                                '<div class="card error"><p>Error: ' + error.message + '</p></div>';
                        }
                    });
                </script>
            </body></html>
        )";
        res.setContent(html, "text/html");
    }

    void updateConfigHandler(WebRequest& req, WebResponse& res) {
        // Handle configuration updates (protected by session + CSRF)
        String newLocation = req.getParam("location");
        bool newEnabled = req.getParam("enabled") == "on";
        float newTempThreshold = req.getParam("temp-threshold").toFloat();
        float newHumidityThreshold = req.getParam("humidity-threshold").toFloat();
        bool newAlertsEnabled = req.getParam("alerts-enabled") == "on";

        // Validate inputs
        if (newLocation.length() == 0 || newTempThreshold <= 0 || newHumidityThreshold <= 0) {
            res.setStatus(400);
            res.setContent("{\"success\":false,\"error\":\"Invalid configuration parameters\"}", 
                         "application/json");
            return;
        }

        // Update configuration
        sensorLocation = newLocation;
        sensorEnabled = newEnabled;
        tempThreshold = newTempThreshold;
        humidityThreshold = newHumidityThreshold;
        alertsEnabled = newAlertsEnabled;

        Serial.println("Configuration updated:");
        Serial.println("  Location: " + sensorLocation);
        Serial.println("  Enabled: " + String(sensorEnabled ? "Yes" : "No"));
        Serial.println("  Temperature Threshold: " + String(tempThreshold) + "°C");
        Serial.println("  Humidity Threshold: " + String(humidityThreshold) + "%");
        Serial.println("  Alerts: " + String(alertsEnabled ? "Enabled" : "Disabled"));

        res.setContent("{\"success\":true,\"message\":\"Configuration updated successfully\"}", 
                     "application/json");
    }

    void getDataAPIHandler(WebRequest& req, WebResponse& res) {
        // API endpoint for external systems (requires API token)
        DynamicJsonDocument doc(400);
        
        doc["sensor_info"]["name"] = getModuleName();
        doc["sensor_info"]["version"] = getModuleVersion();
        doc["sensor_info"]["location"] = sensorLocation;
        doc["sensor_info"]["enabled"] = sensorEnabled;
        
        doc["current_readings"]["temperature"] = temperature;
        doc["current_readings"]["humidity"] = humidity;
        doc["current_readings"]["timestamp"] = millis() / 1000;
        doc["current_readings"]["last_update"] = (millis() - lastReading) / 1000;
        
        doc["configuration"]["temp_threshold"] = tempThreshold;
        doc["configuration"]["humidity_threshold"] = humidityThreshold;
        doc["configuration"]["alerts_enabled"] = alertsEnabled;
        
        doc["status"]["temp_alert"] = temperature > tempThreshold;
        doc["status"]["humidity_alert"] = humidity > humidityThreshold;
        doc["status"]["operational"] = sensorEnabled;

        String json;
        serializeJson(doc, json);
        res.setContent(json, "application/json");
    }

    void controlAPIHandler(WebRequest& req, WebResponse& res) {
        // Control API for external systems (session or token auth)
        if (req.getMethod() != WebModule::WM_POST) {
            res.setStatus(405);
            res.setContent("{\"error\":\"Method not allowed\"}", "application/json");
            return;
        }

        String command = req.getParam("command");
        String response;

        if (command == "refresh") {
            updateSensorReadings();
            response = "{\"success\":true,\"message\":\"Sensor readings refreshed\"}";
            
        } else if (command == "enable") {
            sensorEnabled = true;
            response = "{\"success\":true,\"message\":\"Sensor enabled\"}";
            
        } else if (command == "disable") {
            sensorEnabled = false;
            response = "{\"success\":true,\"message\":\"Sensor disabled\"}";
            
        } else if (command == "reset-alerts") {
            // In real implementation, this would clear alert history
            response = "{\"success\":true,\"message\":\"Alert history cleared\"}";
            
        } else {
            res.setStatus(400);
            response = "{\"success\":false,\"error\":\"Unknown command: " + command + "\"}";
        }

        res.setContent(response, "application/json");
    }
};

// Global instance of the module
EnvironmentalSensorModule sensorModule;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Custom Module Example...");

    // Set up navigation
    std::vector<NavigationItem> navItems = {
        NavigationItem("Dashboard", "/"),
        NavigationItem("Environmental", "/sensors/"),
        NavigationItem("System", "/system")
    };
    IWebModule::setNavigationMenu(navItems);

    // Initialize WebPlatform
    webPlatform.begin("SensorDevice");

    // Initialize the custom module
    sensorModule.begin();

    if (webPlatform.isConnected()) {
        Serial.println("Registering environmental sensor module...");
        
        // Register the custom module at /sensors/
        webPlatform.registerModule("/sensors", &sensorModule);

        // Add a simple system page
        webPlatform.registerRoute("/system", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>System Information</title><link rel="stylesheet" href="/assets/style.css"></head>
                <body><div class="container">
                    <h1>System Information</h1>
                    <div class="card">
                        <table class="info-table">
                            <tr><td>Device:</td><td>)" + String(webPlatform.getDeviceName()) + R"(</td></tr>
                            <tr><td>Uptime:</td><td>)" + String(millis() / 1000) + R"( seconds</td></tr>
                            <tr><td>Free Memory:</td><td>)" + String(ESP.getFreeHeap()) + R"( bytes</td></tr>
                            <tr><td>WiFi SSID:</td><td>)" + WiFi.SSID() + R"(</td></tr>
                            <tr><td>IP Address:</td><td>)" + WiFi.localIP().toString() + R"(</td></tr>
                        </table>
                    </div>
                    <a href="/" class="btn btn-primary">Back to Dashboard</a>
                </div></body></html>
            )";
            res.setContent(html, "text/html");
        });

        // Simple dashboard linking to the sensor module
        webPlatform.registerRoute("/", [](WebRequest& req, WebResponse& res) {
            String html = R"(
                <!DOCTYPE html>
                <html><head><title>Sensor Device Dashboard</title><link rel="stylesheet" href="/assets/style.css"></head>
                <body><div class="container">
                    <h1>Sensor Device Dashboard</h1>
                    <div class="card">
                        <h2>Available Modules</h2>
                        <div class="btn-group">
                            <a href="/sensors/" class="btn btn-primary">Environmental Sensors</a>
                            <a href="/system" class="btn btn-secondary">System Information</a>
                        </div>
                    </div>
                </div></body></html>
            )";
            res.setContent(html, "text/html");
        });

        Serial.print("Custom module ready at: ");
        Serial.println(webPlatform.getBaseUrl() + "/sensors/");
    } else {
        Serial.println("WiFi configuration mode");
    }
}

void loop() {
    webPlatform.handle();

    if (webPlatform.isConnected()) {
        sensorModule.handle();
    }

    delay(100);
}