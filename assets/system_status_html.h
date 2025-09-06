#ifndef SYSTEM_STATUS_HTML_H
#define SYSTEM_STATUS_HTML_H

#include <Arduino.h>

const char SYSTEM_STATUS_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - System Status</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
    <script src="/assets/system-status.js"></script>
    <script src="/assets/web-platform-utils.js"></script>
</head>
<body>
    <div class="container">
        {{NAV_MENU}}
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
                    <div class="status-value" id="uptimeValue">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Free Heap:</label>
                    <div class="status-value with-gauge">
                        <span><span id="freeHeap">-</span> bytes (<span id="freeHeapPercent">-</span>% free)</span>
                        <div class="gauge-inline">
                            <div id="memory-gauge" class="gauge">
                                <div id="memory-gauge-fill" class="gauge-fill"></div>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="form-group">
                    <label>Platform Mode:</label>
                    <div class="status-value" id="platformMode">Loading...</div>
                </div>
            </div>
            
            <div class="status-card">
                <h3>Network Information</h3>
                <div class="form-group">
                    <label>WiFi SSID:</label>
                    <div class="status-value" id="wifiSsid">Loading...</div>
                </div>
                <div class="form-group">
                    <label>IP Address:</label>
                    <div class="status-value" id="ipAddress">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Hostname:</label>
                    <div class="status-value" id="hostname">Loading...</div>
                </div>
                <div class="form-group">
                    <label>MAC Address:</label>
                    <div class="status-value" id="macAddress">Loading...</div>
                </div>
                <div class="form-group">
                    <label>Signal Strength:</label>
                    <div class="status-value" id="signalStrength">Loading...</div>
                </div>
            </div>
        </div>
        
        <div class="status-card">
            <h3>Web Server Status</h3>
            <div class="form-group">
                <label>Server Port:</label>
                <div class="status-value" id="serverPort">Loading...</div>
            </div>
            <div class="form-group">
                <label>HTTPS:</label>
                <div class="status-value" id="httpsStatus">Loading...</div>
            </div>
            <div class="form-group">
                <label>Registered Modules:</label>
                <div class="status-value" id="moduleCount">Loading...</div>
            </div>
            <div class="form-group">
                <label>Registered Routes:</label>
                <div class="status-value" id="routeCount">Loading...</div>
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
                <tbody id="modulesTableBody">
                    <tr>
                        <td colspan="3">Loading...</td>
                    </tr>
                </tbody>
            </table>
        </div>
        
        <div class="status-card">
            <h3>Storage Information</h3>
            <div class="form-group">
                <label>Flash Size:</label>
                <div class="status-value" id="flashSize">Loading...</div>
            </div>
            <div class="form-group">
                <label>Used Space:</label>
                <div class="status-value with-gauge">
                    <span><span id="usedSpace">-</span> MB (<span id="usedSpacePercent">-</span>% used)</span>
                    <div class="gauge-inline">
                        <div id="storage-gauge" class="gauge">
                            <div id="storage-gauge-fill" class="gauge-fill"></div>
                        </div>
                    </div>
                </div>
            </div>
            <div class="form-group">
                <label>Available Space:</label>
                <div class="status-value" id="availableSpace">Loading...</div>
            </div>
        </div>
        
        <div class="button-group mt-3">
            <a href="/" class="btn btn-secondary">Back to Home</a>
        </div>
    </div>
    
    <style>
        /* Additional styles for the gauges */
        .gauge {
            width: 100%;
            height: 20px;
            background-color: #e0e0e0;
            border-radius: 10px;
            position: relative;
            overflow: hidden;
        }
        
        .gauge-fill {
            height: 100%;
            width: 0%; /* Will be set by JavaScript */
            border-radius: 10px;
            transition: width 0.5s ease;
        }
        
        .gauge-good .gauge-fill {
            background-color: #4CAF50;
            box-shadow: 0 0 5px rgba(76, 175, 80, 0.5);
        }
        
        .gauge-warning .gauge-fill {
            background-color: #FF9800;
            box-shadow: 0 0 5px rgba(255, 152, 0, 0.5);
        }
        
        .gauge-danger .gauge-fill {
            background-color: #f44336;
            box-shadow: 0 0 5px rgba(244, 67, 54, 0.5);
        }
    </style>
</body>
</html>
)HTML";

#endif // SYSTEM_STATUS_HTML_H