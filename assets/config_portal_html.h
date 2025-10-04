#ifndef CONFIG_PORTAL_HTML_H
#define CONFIG_PORTAL_HTML_H

#include <Arduino.h> // Configuration portal main page with enhanced UI
const char CONFIG_PORTAL_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="stylesheet" href="/assets/web-platform-style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head>
<body>
    <div class="container">
        <h1>{{DEVICE_NAME}} WiFi Setup</h1>
        
        {{SECURITY_NOTICE}}
        
        <div class="status-card wifi-setup">
            <div class="network-scanner">
                <h3>Available Networks</h3>
                <button type="button" class="btn scan-button" id="scan-button">
                    <span id="scan-text">🔍 Scan for Networks</span>
                </button>
                <div class="network-list" id="network-list">
                    <div class="loading">Click "Scan for Networks" to discover WiFi networks</div>
                </div>
            </div>
            
            <form id="wifi-form">
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
            
            <div class="mt-3">
                <p><small><strong>Note:</strong> Device will restart after saving credentials to establish connection.</small></p>
            </div>
        </div>
    </div>
    <script src="/assets/web-platform-utils.js"></script>
    <script src="/assets/wifi.js"></script>
</body>
</html>
)HTML";

#endif // CONFIG_PORTAL_HTML_H