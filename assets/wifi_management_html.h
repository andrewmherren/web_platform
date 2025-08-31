#ifndef WIFI_MANAGEMENT_H
#define WIFI_MANAGEMENT_H

#include <Arduino.h>

const char WIFI_MANAGEMENT_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - WiFi Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="csrf-token" content="{{csrfToken}}">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="stylesheet" href="/assets/web-platform-styles.css">
</head>
<body>
    <div class="container">
        <h1>WiFi Settings</h1>
        
        <div class="wifi-info">
            <div class="status-card">
                <h3>Current Connection</h3>
                <div class="form-group">
                    <label>Network:</label>
                    <div class="status-value">{{CURRENT_SSID}}</div>
                </div>
                <div class="form-group">
                    <label>Signal Strength:</label>
                    <div class="status-value">{{SIGNAL_STRENGTH}} dBm</div>
                </div>
                <div class="form-group">
                    <label>IP Address:</label>
                    <div class="status-value">{{IP_ADDRESS}}</div>
                </div>
                <div class="form-group">
                    <label>MAC Address:</label>
                    <div class="status-value">{{MAC_ADDRESS}}</div>
                </div>
            </div>
            
            <div class="status-card">
                <h3>Available Networks</h3>
                <button type="button" class="btn btn-secondary" id="scan-btn">Scan Networks</button>
                <div class="network-list" id="network-list">
                    <div class="loading">Click "Scan Networks" to discover available WiFi networks</div>
                </div>
            </div>
        </div>
        
        <div class="status-card">
            <h3>Connect to Different Network</h3>
            <form id="wifi-form" method="post">
                <div class="form-group">
                    <label for="ssid">Network Name (SSID):</label>
                    <input type="text" id="ssid" name="ssid" placeholder="Enter network name" required>
                </div>
                <div class="form-group">
                    <label for="password">Password:</label>
                    <input type="password" id="password" name="password" placeholder="Leave empty for open networks">
                </div>
                <div class="button-group">
                    <button type="submit" class="btn btn-primary">Connect</button>
                    <button type="button" class="btn btn-secondary" id="clear-btn">Clear</button>
                </div>
            </form>
        </div>
        
        <div class="status-card danger-zone">
            <h3>Warning: Reset WiFi Settings</h3>
            <p>This will clear all saved WiFi credentials and restart the device in configuration mode.</p>
            <button type="button" class="btn btn-danger" id="reset-btn">Reset WiFi Settings</button>
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

#endif // WIFI_MANAGEMENT_H