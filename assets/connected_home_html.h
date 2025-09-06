#ifndef CONNECTED_HOME_HTML_H
#define CONNECTED_HOME_HTML_H

#include <Arduino.h>

const char CONNECTED_HOME_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - Home</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head>
<body>
    <div class="container">
        {{NAV_MENU}}
        <h1>{{DEVICE_NAME}} Web Interface</h1>
        
        <div class="status-grid">
            <div class="status-card">
                <h3>Device Status</h3>
                <div class="form-group">
                    <label>SSID:</label>
                    <div class="status-value" id="wifiSsid">Loading...</div>
                </div>
                <div class="form-group">
                    <label>IP Address:</label>
                    <div class="status-value" id="ipAddress">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Signal Strength:</label>
                    <div class="status-value" id="signalStrength">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Uptime:</label>
                    <div class="status-value" id="uptime">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Server:</label>
                    <div class="status-value" id="serverProtocol">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Port:</label>
                    <div class="status-value" id="serverPort">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Hostname:</label>
                    <div class="status-value" id="hostname">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Free Memory:</label>
                    <div class="status-value"><span id="freeMemory">Loading...</span> KB</div>
                </div>
            </div>
        </div>
        
        <div class="status-card">
            <h3>Available Modules</h3>
            <div id="moduleList">
                <p>Loading modules...</p>
            </div>
        </div>
        
        <div class="button-group mt-3">
            <a href="/status" class="btn">System Status</a>
            <a href="/wifi" class="btn">WiFi Setup</a>
            <a href="/account" class="btn">Account Settings</a>
        </div>
    </div>
    <script src="/assets/home-page.js"></script>
    <script src="/assets/web-platform-utils.js"></script>
</body>
</html>
)HTML";

#endif // CONNECTED_HOME_HTML_H