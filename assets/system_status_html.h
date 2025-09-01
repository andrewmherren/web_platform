#ifndef SYSTEM_STATUS_HTML_H
#define SYSTEM_STATUS_HTML_H

#include <Arduino.h>

const char SYSTEM_STATUS_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - System Status</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="csrf-token" content="{{csrfToken}}">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head>
<body>
    <div class="container">
        <h1>System Status</h1>
        
        <div class="status-grid">
            <div class="status-card">
                <h3>Device Information</h3>
                <div class="form-group">
                    <label>Device Name:</label>
                    <div class="status-value">{{DEVICE_NAME}}</div>
                </div>
                <div class="form-group">
                    <label>Uptime:</label>
                    <div class="status-value">{{UPTIME}} seconds</div>
                </div>
                <div class="form-group">
                    <label>Free Heap:</label>
                    <div class="status-value">{{FREE_HEAP}} bytes</div>
                </div>
                <div class="form-group">
                    <label>Platform Mode:</label>
                    <div class="status-value">{{PLATFORM_MODE}}</div>
                </div>
            </div>
            
            <div class="status-card">
                <h3>Network Information</h3>
                <div class="form-group">
                    <label>WiFi SSID:</label>
                    <div class="status-value">{{WIFI_SSID}}</div>
                </div>
                <div class="form-group">
                    <label>IP Address:</label>
                    <div class="status-value">{{IP_ADDRESS}}</div>
                </div>
                <div class="form-group">
                    <label>Hostname:</label>
                    <div class="status-value">{{HOSTNAME}}</div>
                </div>
                <div class="form-group">
                    <label>MAC Address:</label>
                    <div class="status-value">{{MAC_ADDRESS}}</div>
                </div>
                <div class="form-group">
                    <label>Signal Strength:</label>
                    <div class="status-value">{{SIGNAL_STRENGTH}} dBm</div>
                </div>
            </div>
        </div>
        
        <div class="status-card">
            <h3>Web Server Status</h3>
            <div class="form-group">
                <label>Server Port:</label>
                <div class="status-value">{{SERVER_PORT}}</div>
            </div>
            <div class="form-group">
                <label>HTTPS:</label>
                <div class="status-value">{{HTTPS_STATUS}}</div>
            </div>
            <div class="form-group">
                <label>Registered Modules:</label>
                <div class="status-value">{{MODULE_COUNT}}</div>
            </div>
            <div class="form-group">
                <label>Registered Routes:</label>
                <div class="status-value">{{ROUTE_COUNT}}</div>
            </div>
        </div>
        
        <div class="status-card">
            <h3>Modules</h3>
            <table style="width: 100%">
                <tr>
                    <th style="text-align: left">Name</th>
                    <th style="text-align: left">Version</th>
                    <th style="text-align: left">Path</th>
                </tr>
                {{MODULE_TABLE}}
            </table>
        </div>
        
        <div class="button-group mt-3">
            <a href="/" class="btn btn-secondary">Back to Home</a>
        </div>
    </div>
</body>
</html>
)HTML";

#endif // SYSTEM_STATUS_HTML_H