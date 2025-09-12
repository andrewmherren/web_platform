/**
 * Environmental Sensor Module - Implementation File
 */

#include "environmental_sensor_module.h"

// Constructor
EnvironmentalSensorModule::EnvironmentalSensorModule()
    : temperature(23.5), humidity(45.2), sensorEnabled(true),
      sensorLocation("Office"), lastReading(0), tempThreshold(30.0),
      humidityThreshold(60.0), alertsEnabled(true) {
  updateSensorReadings();
}

// Module lifecycle methods
void EnvironmentalSensorModule::begin() {
  Serial.println("Environmental Sensor Module initialized");
  updateSensorReadings();
}

void EnvironmentalSensorModule::handle() {
  // Update sensor readings periodically
  if (millis() - lastReading > 30000) { // Every 30 seconds
    updateSensorReadings();
  }
}

// Public getters
float EnvironmentalSensorModule::getTemperature() const { return temperature; }
float EnvironmentalSensorModule::getHumidity() const { return humidity; }
bool EnvironmentalSensorModule::isEnabled() const { return sensorEnabled; }
String EnvironmentalSensorModule::getLocation() const { return sensorLocation; }

// Configuration setters
void EnvironmentalSensorModule::setLocation(const String& location) {
  sensorLocation = location;
}

void EnvironmentalSensorModule::setTempThreshold(float threshold) {
  tempThreshold = threshold;
}

void EnvironmentalSensorModule::setHumidityThreshold(float threshold) {
  humidityThreshold = threshold;
}

void EnvironmentalSensorModule::setAlertsEnabled(bool enabled) {
  alertsEnabled = enabled;
}

// Required IWebModule methods
std::vector<WebRoute> EnvironmentalSensorModule::getHttpRoutes() {
  return {
      // Public main page - local network only
      WebRoute("/", WebModule::WM_GET,
               [this](WebRequest &req, WebResponse &res) {
                 mainPageHandler(req, res);
               },
               {AuthType::LOCAL_ONLY}),

      // Public API for current readings - session or page token required
      WebRoute("/api/current", WebModule::WM_GET,
               [this](WebRequest &req, WebResponse &res) {
                 getCurrentDataHandler(req, res);
               },
               {AuthType::SESSION, AuthType::PAGE_TOKEN}),

      // Protected configuration page - requires login
      WebRoute("/config", WebModule::WM_GET,
               [this](WebRequest &req, WebResponse &res) {
                 configPageHandler(req, res);
               },
               {AuthType::SESSION}),

      // Protected configuration API - requires login + CSRF for forms
      WebRoute("/api/config", WebModule::WM_POST,
               [this](WebRequest &req, WebResponse &res) {
                 updateConfigHandler(req, res);
               },
               {AuthType::SESSION, AuthType::PAGE_TOKEN}),

      // API endpoint for external systems (requires API token)
      WebRoute("/api/data", WebModule::WM_GET,
               [this](WebRequest &req, WebResponse &res) {
                 getDataAPIHandler(req, res);
               },
               {AuthType::TOKEN, AuthType::SESSION, AuthType::PAGE_TOKEN}),

      // Administrative control (session or token auth)
      WebRoute("/api/control", WebModule::WM_POST,
               [this](WebRequest &req, WebResponse &res) {
                 controlAPIHandler(req, res);
               },
               {AuthType::TOKEN, AuthType::SESSION, AuthType::PAGE_TOKEN}),
  };
}

std::vector<WebRoute> EnvironmentalSensorModule::getHttpsRoutes() {
  // Same routes for HTTPS
  return getHttpRoutes();
}

String EnvironmentalSensorModule::getModuleName() const {
  return "Environmental Sensor";
}

String EnvironmentalSensorModule::getModuleVersion() const { return "1.2.0"; }

String EnvironmentalSensorModule::getModuleDescription() const {
  return "Environmental monitoring with temperature and humidity sensors";
}

// Private methods
void EnvironmentalSensorModule::updateSensorReadings() {
  if (!sensorEnabled)
    return;

  // Simulate sensor readings with some variation
  temperature = 20.0 + random(0, 150) / 10.0; // 20.0 - 35.0°C
  humidity = 30.0 + random(0, 400) / 10.0;    // 30.0 - 70.0%
  lastReading = millis();

  // Check thresholds and log alerts (in real implementation)
  if (alertsEnabled) {
    if (temperature > tempThreshold) {
      Serial.println("ALERT: Temperature threshold exceeded: " +
                     String(temperature) + "°C");
    }
    if (humidity > humidityThreshold) {
      Serial.println(
          "ALERT: Humidity threshold exceeded: " + String(humidity) + "%");
    }
  }
}

void EnvironmentalSensorModule::mainPageHandler(WebRequest &req,
                                                WebResponse &res) {
  String html =
      R"(
            <!DOCTYPE html>
            <html><head>
                <title>Environmental Sensor</title>
                <link rel="stylesheet" href="/assets/style.css">
                <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
                <link rel="icon" href="/assets/favicon.ico" sizes="any">
                <script>
                    // Auto-refresh data every 30 seconds
                    function updateReadings() {
                        fetch('/sensors/api/current')
                            .then(response => response.json())
                            .then(data => {
                                document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '\u00B0C';
                                document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                                // Calculate time since last reading using the seconds_since_reading field
                                const secondsSince = data.seconds_since_reading;
                                
                                let timeText;
                                if (secondsSince < 2) {
                                    timeText = 'Just now';
                                } else if (secondsSince < 60) {
                                    timeText = secondsSince + ' seconds ago';
                                } else if (secondsSince < 3600) {
                                    const minutes = Math.floor(secondsSince / 60);
                                    timeText = minutes === 1 ? '1 minute ago' : minutes + ' minutes ago';
                                } else {
                                    const hours = Math.floor(secondsSince / 3600);
                                    timeText = hours === 1 ? '1 hour ago' : hours + ' hours ago';
                                }
                                
                                document.getElementById('last-update').textContent = timeText;
                                
                                // Update status indicators
                                const tempCard = document.getElementById('temp-card');
                                const humCard = document.getElementById('hum-card');
                                
                                tempCard.className = 'status-card ' + 
                                    (data.temperature > )" +
      String(tempThreshold) + R"( ? 'warning' : 'success');
                                humCard.className = 'status-card ' + 
                                    (data.humidity > )" +
      String(humidityThreshold) + R"( ? 'warning' : 'success');
                            })
                            .catch(error => console.error('Error updating readings:', error));
                    }
                    
                    // Update on page load and every 30 seconds
                    document.addEventListener('DOMContentLoaded', updateReadings);
                    setInterval(updateReadings, 30000);
                </script>
            </head><body>
                <div class="container">
                  {{NAV_MENU}}
                    <h1>Environmental Sensor</h1>
                    <p>Location: <strong>)" +
      sensorLocation + R"(</strong></p>
                    
                    <div class="status-grid">
                        <div id="temp-card" class="status-card )" +
      String(temperature > tempThreshold ? "warning" : "success") + R"(">
                            <h3>Temperature</h3>
                            <p id="temperature" class="large-text">)" +
      String(temperature, 1) + R"(&deg;C</p>
                            <small>Threshold: )" +
      String(tempThreshold, 1) + R"(&deg;C</small>
                        </div>
                        <div id="hum-card" class="status-card )" +
      String(humidity > humidityThreshold ? "warning" : "success") + R"(">
                            <h3>Humidity</h3>
                            <p id="humidity" class="large-text">)" +
      String(humidity, 1) + R"(%</p>
                            <small>Threshold: )" +
      String(humidityThreshold, 1) + R"(%</small>
                        </div>
                        <div class="status-card info">
                            <h3>Sensor Status</h3>
                            <p style="color: rgba(255, 255, 255, 0.9);">)" +
      String(sensorEnabled ? "Active" : "Disabled") + R"(</p>
                            <small id="last-update" style="color: rgba(255, 255, 255, 0.7);">Updated just now</small>
                        </div>
                    </div>
                    
                    <div class="card">
                        <h2>Quick Actions</h2>
                        <button class="btn btn-primary" onclick='updateReadings()'>Refresh Data</button>
                        <button class="btn btn-secondary" onclick="window.location='/sensors/config'">Configuration</button>
                    </div>

                    <div class="card mt-3">
                        <h2>API Endpoints</h2>
                        
                        <h3>Public Access</h3>
                        <div class="status-card mb-2">
                            <p><strong>GET</strong> <code>/sensors/api/current</code></p>
                            <p>Current sensor readings</p>
                        </div>
                        
                        <h3>API Token Required</h3>
                        <div class="status-card mb-1">
                            <p><strong>GET</strong> <code>/sensors/api/data</code></p>
                            <p>Detailed sensor data with configuration</p>
                        </div>
                        <div class="status-card">
                            <p><strong>POST</strong> <code>/sensors/api/control</code></p>
                            <p>Control sensor operations (refresh, enable, disable, reset-alerts)</p>
                        </div>
                    </div>
                </div>
            </body></html>
        )";
  res.setContent(html, "text/html");
}

void EnvironmentalSensorModule::getCurrentDataHandler(WebRequest &req,
                                                      WebResponse &res) {
  // Public endpoint - anyone can access current readings
  DynamicJsonDocument doc(200);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = time(nullptr); // Unix timestamp in seconds
  doc["seconds_since_reading"] =
      (millis() - lastReading) / 1000; // Seconds since last reading
  doc["location"] = sensorLocation;
  doc["status"] = sensorEnabled ? "active" : "disabled";

  String json;
  serializeJson(doc, json);
  res.setContent(json, "application/json");
}

void EnvironmentalSensorModule::configPageHandler(WebRequest &req,
                                                   WebResponse &res) {
  // Protected configuration page
  String html = R"(
            <!DOCTYPE html>
            <html><head>
                <title>Sensor Configuration</title>
                <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
                <link rel="icon" href="/assets/favicon.ico" sizes="any">
                <link rel="stylesheet" href="/assets/style.css">
                <script src="/assets/web-platform-utils.js"></script>
            </head><body>
                <div class="container">
                  {{NAV_MENU}}
                    <h1>Sensor Configuration</h1>
                    <form id="config-form" method="post" action="/sensors/api/config">
                        <div class="card">
                            <h2>Sensor Settings</h2>
                            <div class="form-group">
                                <label for="location">Sensor Location:</label>
                                <input type="text" id="location" name="location" class="form-control" 
                                       value=")" +
                sensorLocation + R"(" required>
                            </div>
                            <div class="form-group">
                                <label>
                                    <input type="checkbox" name="enabled" )" +
                String(sensorEnabled ? "checked" : "") + R"(>
                                    Enable Sensor Readings
                                </label>
                            </div>
                        </div>
                        
                        <div class="card">
                            <h2>Alert Thresholds</h2>
                            <div class="form-group">
                                <label for="temp-threshold">Temperature Threshold (&deg;C):</label>
                                <input type="number" id="temp-threshold" name="temp-threshold" 
                                       class="form-control" step="0.1" value=")" +
                String(tempThreshold, 1) + R"(" required>
                            </div>
                            <div class="form-group">
                                <label for="humidity-threshold">Humidity Threshold (%):</label>
                                <input type="number" id="humidity-threshold" name="humidity-threshold" 
                                       class="form-control" step="0.1" value=")" +
                String(humidityThreshold, 1) + R"(" required>
                            </div>
                            <div class="form-group">
                                <label>
                                    <input type="checkbox" name="alerts-enabled" )" +
                String(alertsEnabled ? "checked" : "") + R"(>
                                    Enable Threshold Alerts (printed to serial log)
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
                        
                        try {
                            const result = await AuthUtils.fetch('/sensors/api/config', {
                                method: 'POST',
                                headers: {
                                    // content type header is important. application/x-www-form-urlencoded
                                    // tells the server to parse form data while application/json tells the
                                    // server to parse json body data
                                    'Content-Type': 'application/x-www-form-urlencoded'
                                },
                                body: new URLSearchParams(formData)
                            }).then(response => response.json());
                            
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

void EnvironmentalSensorModule::updateConfigHandler(WebRequest &req,
                                                     WebResponse &res) {
  // Handle configuration updates (protected by session + CSRF)
  // note: because we sent application/x-www-form-urlencoded content-type
  // (formdata)
  //       we get the results from the request with getParam. If we had sent
  //       application/json then we would use getJsonParam
  String newLocation = req.getParam("location");
  bool newEnabled = req.getParam("enabled") == "on";
  float newTempThreshold = req.getParam("temp-threshold").toFloat();
  float newHumidityThreshold = req.getParam("humidity-threshold").toFloat();
  bool newAlertsEnabled = req.getParam("alerts-enabled") == "on";

  Serial.println("Configuration update request:");
  Serial.println("  Location: " + newLocation);
  Serial.println("  Enabled: " + String(newEnabled ? "Yes" : "No"));
  Serial.println("  Temperature Threshold: " + String(newTempThreshold) + "°C");
  Serial.println("  Humidity Threshold: " + String(newHumidityThreshold) + "%");
  Serial.println("  Alerts: " + String(newAlertsEnabled ? "Enabled" : "Disabled"));

  // Validate inputs
  if (newLocation.length() == 0 || newTempThreshold <= 0 ||
      newHumidityThreshold <= 0) {
    res.setStatus(400);
    res.setContent(
        "{\"success\":false,\"error\":\"Invalid configuration parameters\"}",
        "application/json");
    return;
  }

  // Update configuration
  sensorLocation = newLocation;
  sensorEnabled = newEnabled;
  tempThreshold = newTempThreshold;
  humidityThreshold = newHumidityThreshold;
  alertsEnabled = newAlertsEnabled;

  Serial.println("Configuration updated successfully:");
  Serial.println("  Location: " + sensorLocation);
  Serial.println("  Enabled: " + String(sensorEnabled ? "Yes" : "No"));
  Serial.println("  Temperature Threshold: " + String(tempThreshold) + "°C");
  Serial.println("  Humidity Threshold: " + String(humidityThreshold) + "%");
  Serial.println("  Alerts: " + String(alertsEnabled ? "Enabled" : "Disabled"));

  res.setContent(
      "{\"success\":true,\"message\":\"Configuration updated successfully\"}",
      "application/json");
}

void EnvironmentalSensorModule::getDataAPIHandler(WebRequest &req,
                                                   WebResponse &res) {
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

void EnvironmentalSensorModule::controlAPIHandler(WebRequest &req,
                                                   WebResponse &res) {
  // Control API for external systems (session or token auth)
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
    response =
        "{\"success\":false,\"error\":\"Unknown command: " + command + "\"}";
  }

  res.setContent(response, "application/json");
}

// Global instance of the module
EnvironmentalSensorModule sensorModule;