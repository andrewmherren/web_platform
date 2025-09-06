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
        
        <div class="card">
            <h3>Available Networks</h3>
            <div class="button-group">
                <button id="scanButton" class="btn btn-primary">Scan for Networks</button>
            </div>
            <div id="scanResults">
                <p>Click "Scan for Networks" to see available WiFi networks.</p>
            </div>
        </div>
        
        <div class="card">
            <h3>Connect to Network</h3>
            <form id="connectForm">
                <div class="form-group">
                    <label for="ssid">SSID:</label>
                    <input type="text" id="ssid" name="ssid" class="form-control" required>
                </div>
                <div class="form-group">
                    <label for="password">Password:</label>
                    <input type="password" id="password" name="password" class="form-control">
                    <small>Leave blank for open networks</small>
                </div>
                <div class="button-group">
                    <button type="submit" class="btn btn-primary">Connect</button>
                </div>
            </form>
            <div id="connectionStatus"></div>
        </div>
        
        <div class="button-group mt-3">
            <a href="/" class="btn btn-secondary">Back to Home</a>
        </div>
    </div>
    <script src="/assets/web-platform-utils.js"></script>
    <script src="/assets/wifi-management.js"></script>
</body>
</html>
)HTML";

#endif // WIFI_MANAGEMENT_HTML_H