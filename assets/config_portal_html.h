#ifndef CONFIG_PORTAL_HTML_H
#define CONFIG_PORTAL_HTML_H

#include <Arduino.h> // Configuration portal main page with enhanced UI
const char CONFIG_PORTAL_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/tickertape-theme.css">
    <style>
        .wifi-setup { max-width: 500px; margin: 20px auto; }
        .network-scanner { margin-bottom: 20px; }
        .scan-button { 
            width: 100%; 
            margin-bottom: 15px; 
            background: linear-gradient(45deg, #4CAF50, #45a049);
        }
        .network-list { max-height: 300px; overflow-y: auto; }
        .network-item { 
            cursor: pointer; 
            padding: 12px; 
            margin: 6px 0; 
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.1);
            transition: all 0.3s ease;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .network-item:hover { 
            background: rgba(255,255,255,0.1);
            border-color: rgba(255,255,255,0.3);
            transform: translateY(-2px);
        }
        .network-item.selected {
            background: rgba(76, 175, 80, 0.2);
            border-color: #4CAF50;
        }
        .network-name { font-weight: bold; }
        .network-info { 
            display: flex; 
            align-items: center; 
            gap: 8px; 
            color: rgba(255,255,255,0.7);
            font-size: 0.9em;
        }
        .security-icon { font-size: 1.2em; }
        .signal-strength { font-size: 0.8em; }
        .loading { 
            text-align: center; 
            color: rgba(255,255,255,0.7);
            font-style: italic;
        }
        .security-notice {
            background: rgba(76, 175, 80, 0.1);
            border: 1px solid rgba(76, 175, 80, 0.3);
            border-radius: 8px;
            padding: 12px;
            margin-bottom: 20px;
        }
        .security-notice h4 {
            color: #4CAF50;
            margin: 0 0 8px 0;
        }
    </style>
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
            
            <form id="wifi-form" action="/save" method="post">
                <div class="form-group">
                    <label for="ssid">Network Name (SSID):</label>
                    <input type="text" id="ssid" name="ssid" placeholder="Select network or enter manually" required>
                </div>
                <div class="form-group">
                    <label for="password">Password:</label>
                    <input type="password" id="password" name="password" placeholder="Leave empty for open networks">
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
    
    <script>
        var selectedNetwork = null;
        var isScanning = false;
        
        function scanNetworks() {
            if (isScanning) return;
            
            isScanning = true;
            var button = document.getElementById('scan-text');
            var networkList = document.getElementById('network-list');
            
            button.innerHTML = '⟳ Scanning...';
            networkList.innerHTML = '<div class="loading">Scanning for networks...</div>';
            
            fetch('/api/scan')
                .then(function(response) { return response.json(); })
                .then(function(data) {
                    networkList.innerHTML = '';
                    
                    if (data.networks && data.networks.length > 0) {
                        data.networks.forEach(function(network) {
                            var div = document.createElement('div');
                            div.className = 'network-item';
                            div.onclick = function() { selectNetwork(network.ssid, div); };
                            
                            var networkName = document.createElement('div');
                            networkName.className = 'network-name';
                            networkName.textContent = network.ssid;
                            
                            var networkInfo = document.createElement('div');
                            networkInfo.className = 'network-info';
                            
                            var securityIcon = document.createElement('span');
                            securityIcon.className = 'security-icon';
                            securityIcon.innerHTML = network.encryption ? '🔒' : '🔓';
                            
                            var signalStrength = document.createElement('span');
                            signalStrength.className = 'signal-strength';
                            signalStrength.textContent = network.rssi + 'dBm';
                            
                            networkInfo.appendChild(securityIcon);
                            networkInfo.appendChild(signalStrength);
                            
                            div.appendChild(networkName);
                            div.appendChild(networkInfo);
                            networkList.appendChild(div);
                        });
                    } else {
                        networkList.innerHTML = '<div class="loading">No networks found. Try scanning again.</div>';
                    }
                })
                .catch(function(error) {
                    console.error('Scan error:', error);
                    networkList.innerHTML = '<div class="loading">Scan failed. Please try again.</div>';
                })
                .finally(function() {
                    isScanning = false;
                    button.innerHTML = '🔍 Scan for Networks';
                });
        }
        
        function selectNetwork(ssid, element) {
            var items = document.querySelectorAll('.network-item');
            for (var i = 0; i < items.length; i++) {
                items[i].classList.remove('selected');
            }
            
            element.classList.add('selected');
            selectedNetwork = ssid;
            
            document.getElementById('ssid').value = ssid;
            document.getElementById('password').focus();
        }
        
        function clearForm() {
            document.getElementById('ssid').value = '';
            document.getElementById('password').value = '';
            selectedNetwork = null;
            
            var items = document.querySelectorAll('.network-item');
            for (var i = 0; i < items.length; i++) {
                items[i].classList.remove('selected');
            }
        }
        
        function escapeHtml(text) {
            var div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
        
        window.addEventListener('load', function() {
            setTimeout(scanNetworks, 500);
        });
    </script>
</body>
</html>
)HTML";

#endif // CONFIG_PORTAL_HTML_H