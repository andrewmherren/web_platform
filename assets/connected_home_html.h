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
    <style>
        .device-info {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-top: 20px;
        }
        .module-list {
            margin-top: 20px;
        }
        .module-item {
            display: flex;
            justify-content: space-between;
            padding: 12px;
            margin-bottom: 8px;
            border-radius: 8px;
            background: rgba(255,255,255,0.1);
        }
        .module-item .btn {
            margin-left: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome to {{DEVICE_NAME}}</h1>
        
        <div class="device-info">
            <div class="status-card">
                <h3>Device Status</h3>
                <div class="form-group">
                    <label>WiFi Network:</label>
                    <div class="status-value">{{WIFI_SSID}}</div>
                </div>
                <div class="form-group">
                    <label>IP Address:</label>
                    <div class="status-value">{{IP_ADDRESS}}</div>
                </div>
                <div class="form-group">
                    <label>Signal Strength:</label>
                    <div class="status-value">{{SIGNAL_STRENGTH}} dBm</div>
                </div>
                <div class="form-group">
                    <label>Uptime:</label>
                    <div class="status-value">{{UPTIME}} seconds</div>
                </div>
            </div>
            
            <div class="status-card">
                <h3>Web Server</h3>
                <div class="form-group">
                    <label>Server Protocol:</label>
                    <div class="status-value">{{SERVER_PROTOCOL}}</div>
                </div>
                <div class="form-group">
                    <label>Server Port:</label>
                    <div class="status-value">{{SERVER_PORT}}</div>
                </div>
                <div class="form-group">
                    <label>Hostname:</label>
                    <div class="status-value">{{HOSTNAME}}</div>
                </div>
                <div class="form-group">
                    <label>Free Memory:</label>
                    <div class="status-value">{{FREE_MEMORY}} KB</div>
                </div>
            </div>
        </div>
        
        <div class="status-card module-list">
            <h3>Installed Modules</h3>
            {{MODULE_LIST}}
        </div>
        
        <div class="button-group mt-3">
            <a href="/status" class="btn btn-secondary">System Status</a>
            <a href="/wifi" class="btn btn-secondary">WiFi Settings</a>
        </div>
    </div>
</body>
</html>
)HTML";

#endif // CONNECTED_HOME_HTML_H