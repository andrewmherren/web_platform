#ifndef WIFI_MANAGEMENT_H
#define WIFI_MANAGEMENT_H

#include <Arduino.h>

const char WIFI_MANAGEMENT_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - WiFi Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/tickertape-theme.css">
    <style>
        .wifi-info {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .network-list {
            max-height: 300px;
            overflow-y: auto;
            margin-top: 15px;
        }
        .network-item {
            padding: 10px;
            margin: 5px 0;
            border-radius: 6px;
            background: rgba(255,255,255,0.1);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .network-item:hover {
            background: rgba(255,255,255,0.2);
        }
        .signal-strength {
            font-size: 0.9em;
            color: rgba(255,255,255,0.7);
        }
        .danger-zone {
            border: 1px solid rgba(255, 87, 34, 0.5);
            background: rgba(255, 87, 34, 0.1);
        }
        .danger-zone h3 {
            color: #ff5722;
        }
    </style>
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
                <button type="button" class="btn btn-secondary" onclick="scanNetworks()" id="scan-btn">Scan Networks</button>
                <div class="network-list" id="network-list">
                    <div class="loading">Click "Scan Networks" to discover available WiFi networks</div>
                </div>
            </div>
        </div>
        
        <div class="status-card">
            <h3>Connect to Different Network</h3>
            <form id="wifi-form" method="post" onsubmit="connectToNetwork(event)">
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
                    <button type="button" class="btn btn-secondary" onclick="clearForm()">Clear</button>
                </div>
            </form>
        </div>
        
        <div class="status-card danger-zone">
            <h3>Warning: Reset WiFi Settings</h3>
            <p>This will clear all saved WiFi credentials and restart the device in configuration mode.</p>
            <button type="button" class="btn btn-danger" onclick="resetWiFi()">Reset WiFi Settings</button>
        </div>
        
        <div class="button-group mt-3">
            <a href="/" class="btn btn-secondary">Back to Home</a>
        </div>
    </div>
    
    <script>
        function scanNetworks() {
            var btn = document.getElementById("scan-btn");
            var list = document.getElementById("network-list");
            
            btn.disabled = true;
            btn.textContent = "Scanning...";
            list.innerHTML = "<div class=\"loading\">Scanning for networks...</div>";
            
            fetch("/api/scan")
                .then(function(response) { return response.json(); })
                .then(function(data) {
                    list.innerHTML = "";
                    
                    if (data.networks && data.networks.length > 0) {
                        data.networks.forEach(function(network) {
                            var item = document.createElement("div");
                            item.className = "network-item";
                            item.innerHTML = "<div><strong>" + escapeHtml(network.ssid) + "</strong> " + 
                                           (network.encryption ? "SECURED" : "OPEN") + 
                                           "</div><div class=\"signal-strength\">" + network.rssi + " dBm</div>";
                            item.style.cursor = "pointer";
                            item.onclick = function() {
                                document.getElementById("ssid").value = network.ssid;
                                document.getElementById("password").focus();
                            };
                            list.appendChild(item);
                        });
                    } else {
                        list.innerHTML = "<div class=\"loading\">No networks found</div>";
                    }
                })
                .catch(function(error) {
                    console.error("Scan failed:", error);
                    list.innerHTML = "<div class=\"loading\">Scan failed. Please try again.</div>";
                })
                .finally(function() {
                    btn.disabled = false;
                    btn.textContent = "Scan Networks";
                });
        }
        
        function clearForm() {
            document.getElementById("ssid").value = "";
            document.getElementById("password").value = "";
        }
        
        function resetWiFi() {
            if (confirm("This will reset all WiFi settings and restart the device. Continue?")) {
                fetch("/api/reset", { method: "POST" })
                    .then(function() {
                        alert("WiFi settings reset. Device will restart in configuration mode.");
                    })
                    .catch(function() {
                        // Ignore errors as device may restart quickly
                    });
            }
        }
        
        function connectToNetwork(event) {
            event.preventDefault();
            var ssid = document.getElementById("ssid").value;
            var password = document.getElementById("password").value;
            
            if (!ssid) {
                alert("Please enter a network name");
                return false;
            }
            
            var formData = new FormData();
            formData.append("ssid", ssid);
            formData.append("password", password);
            
            fetch("/api/connect", {
                method: "POST",
                body: formData
            })
            .then(function(response) { return response.json(); })
            .then(function(data) {
                if (data.status === "restarting") {
                    alert("Connecting to network: " + ssid + ". Device will restart.");
                    // Show a countdown or loading screen here if desired
                } else {
                    alert("Error: " + data.message);
                }
            })
            .catch(function(error) {
                console.error("Connection error:", error);
                // Device might restart quickly, so this error is expected
            });
            
            return false;
        }
        
        function escapeHtml(text) {
            var div = document.createElement("div");
            div.textContent = text;
            return div.innerHTML;
        }
    </script>
</body>
</html>
)HTML";

#endif // WIFI_MANAGEMENT_H