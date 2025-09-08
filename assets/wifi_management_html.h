#ifndef WIFI_MANAGEMENT_HTML_H
#define WIFI_MANAGEMENT_HTML_H

#include <Arduino.h>

const char WIFI_MANAGEMENT_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - WiFi Management</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="stylesheet" href="/assets/web-platform-style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head>
<body>
    <div class="container">
        {{NAV_MENU}}
        <h1>WiFi Management</h1>
        
        <div class="status-card">
            <h3>Current Connection</h3>
            <div class="form-group">
                <label>SSID:</label>
                <div class="status-value" id="currentSsid">Loading...</div>
            </div>
            <div class="form-group">
                <label>Signal Strength:</label>
                <div class="status-value" id="signalStrength">Loading...</div>
            </div>
            <div class="form-group">
                <label>IP Address:</label>
                <div class="status-value" id="ipAddress">Loading...</div>
            </div>
            <div class="form-group">
                <label>MAC Address:</label>
                <div class="status-value" id="macAddress">Loading...</div>
            </div>
        </div>
        
        <div class="card wifi-setup">
            <div class="network-scanner">
                <h3>Available Networks</h3>
                <button type="button" class="btn scan-button" id="scan-button">
                    <span id="scan-text">🔍 Scan for Networks</span>
                </button>
                <div class="network-list" id="network-list">
                    <div class="loading">Click "Scan for Networks" to discover WiFi networks</div>
                </div>
            </div>
            
            <form id="connectForm">
                <div class="form-group">
                    <label for="ssid">Network Name (SSID):</label>
                    <input type="text" id="ssid" name="ssid" class="form-control" placeholder="Select network or enter manually" required>
                </div>
                <div class="form-group">
                    <label for="password">Password:</label>
                    <div class="password-field">
                        <input type="password" id="password" name="password" class="form-control" placeholder="Leave empty for open networks">
                        <button type="button" class="password-toggle" onclick="togglePassword()" title="Show password">
                            Show
                        </button>
                    </div>
                </div>
                <div class="button-group">
                    <button type="submit" class="btn btn-primary">Connect to WiFi</button>
                    <button type="button" class="btn btn-secondary" id="clear-button">Clear Form</button>
                </div>
            </form>
            <div id="connectionStatus"></div>
        </div>
        
        <div class="button-group mt-3">
            <a href="/" class="btn btn-secondary">Back to Home</a>
        </div>
    </div>
    <script src="/assets/web-platform-utils.js"></script>
    <script src="/assets/wifi.js"></script>
</body>
</html>
)HTML";

#endif // WIFI_MANAGEMENT_HTML_H