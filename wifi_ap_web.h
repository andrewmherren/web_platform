#ifndef WIFI_AP_WEB_H
#define WIFI_AP_WEB_H

// Embedded HTML content for WiFi configuration interface
const char WIFI_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>WiFi Management</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 500px; margin: 0 auto; }
    h1 { color: #0066cc; }
    .status-card {
      background-color: #f5f5f5;
      border-radius: 8px;
      padding: 15px;
      margin-bottom: 20px;
    }
    /* Loading overlay styles */
    .loading-overlay {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background-color: rgba(0, 0, 0, 0.7);
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      z-index: 1000;
      color: white;
      display: none;
    }
    .spinner {
      border: 5px solid #f3f3f3;
      border-top: 5px solid #0066cc;
      border-radius: 50%;
      width: 50px;
      height: 50px;
      animation: spin 1s linear infinite;
      margin-bottom: 20px;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    .loading-text {
      font-size: 24px;
      margin-bottom: 10px;
    }
    .loading-subtext {
      font-size: 16px;
      max-width: 80%;
      text-align: center;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label { display: block; margin-bottom: 5px; font-weight: bold; }
    input[type="text"], input[type="password"], select {
      width: 100%;
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      box-sizing: border-box;
    }
    .password-container {
      position: relative;
      display: flex;
      align-items: center;
    }.password-toggle {
    position: absolute;
    right: 10px;
    cursor: pointer;
    user-select: none;
    color: #666;
    font-size: 14px;
    font-weight: bold;
    z-index: 10;
    background: none;
    border: none;
    padding: 0 8px;
    height: 100%;
  }
    .password-toggle:hover {
      color: #333;
    }
    .password-container input {
      padding-right: 40px;
    }
    .btn {
      background-color: #0066cc;
      color: white;
      padding: 10px 15px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      margin: 5px 5px 5px 0;
    }
    .btn:hover { background-color: #0055aa; }
    .btn-danger {
      background-color: #dc3545;
    }
    .btn-danger:hover { background-color: #c82333; }
    .btn-secondary {
      background-color: #6c757d;
    }
    .btn-secondary:hover { background-color: #5a6268; }
    .hidden { display: none; }
    .status-message {
      padding: 10px;
      border-radius: 4px;
      margin: 10px 0;
      font-weight: bold;
    }
    .success { background-color: #d4edda; color: #155724; }
    .error { background-color: #f8d7da; color: #721c24; }
    .info { background-color: #d1ecf1; color: #0c5460; }
    .network-list {
      max-height: 200px;
      overflow-y: auto;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    .network-item {
      padding: 10px;
      border-bottom: 1px solid #eee;
      cursor: pointer;
    }
    .network-item:hover {
      background-color: #f8f9fa;
    }
    .network-item:last-child {
      border-bottom: none;
    }
    .network-name {
      font-weight: bold;
    }
    .network-details {
      font-size: 0.9em;
      color: #666;
    }
  </style>
</head>
<body>
  <!-- Loading overlay -->
  <div id="loadingOverlay" class="loading-overlay">
    <div class="spinner"></div>
    <div class="loading-text" id="loadingText">Connecting to WiFi...</div>
    <div class="loading-subtext" id="loadingSubtext">The device will restart after saving credentials. Please wait...</div>
  </div>
  
  <div class="container">
    <h1>WiFi Management</h1>
    
    <div class="status-card">
      <h2>Current Status</h2>
      <div id="currentStatus">Loading...</div>
      <div id="statusMessage" class="status-message hidden"></div>
    </div>
    
    <div class="status-card">
      <h2>Available Networks</h2>
      <button id="scanBtn" class="btn btn-secondary">Scan Networks</button>
      <div id="networkList" class="network-list hidden"></div>
    </div>
    
    <div class="status-card">
      <h2>Connect to Network</h2>
      <form id="wifiForm">
        <div class="form-group">
          <label for="ssid">WiFi Network Name (SSID):</label>
          <input type="text" id="ssid" name="ssid" required>
        </div>
        
        <div class="form-group">
          <label for="password">WiFi Password:</label>
          <div class="password-container">
            <input type="password" id="password" name="password">
            <button type="button" class="password-toggle" onclick="togglePasswordVisibility()">SHOW</button>
          </div>
        </div>
        
        <button type="submit" class="btn">Connect</button>
        <button type="button" id="resetBtn" class="btn btn-danger">Reset WiFi Settings</button>
      </form>
    </div>
    
    <div class="status-card">
      <h2>Actions</h2>
      <button id="disconnectBtn" class="btn btn-secondary">Disconnect</button>
      <a href="/" class="btn btn-secondary">Back to Main</a>
    </div>
  </div>

  <script>
    let currentWiFiStatus = null;

    // Load status on page load
    window.onload = function() {
      loadWiFiStatus();
    };

    function loadWiFiStatus() {
      fetch('/api/wifi/status')
        .then(response => response.json())
        .then(data => {
          currentWiFiStatus = data;
          updateStatusDisplay(data);
        })
        .catch(error => {
          console.error('Error loading WiFi status:', error);
          showMessage('Failed to load WiFi status', 'error');
        });
    }

    function updateStatusDisplay(status) {
      const statusDiv = document.getElementById('currentStatus');
      let html = '';
      
      if (status.connected) {
        html = `
          <p><strong>Status:</strong> Connected</p>
          <p><strong>Network:</strong> ${status.ssid}</p>
          <p><strong>IP Address:</strong> ${status.ip}</p>
          <p><strong>Signal Strength:</strong> ${status.rssi} dBm</p>
          <p><strong>MAC Address:</strong> ${status.mac}</p>
        `;
        document.getElementById('disconnectBtn').style.display = 'inline-block';
      } else {
        html = `
          <p><strong>Status:</strong> ${status.state}</p>
          ${status.saved_ssid ? `<p><strong>Saved Network:</strong> ${status.saved_ssid}</p>` : ''}
        `;
        document.getElementById('disconnectBtn').style.display = 'none';
      }
      
      statusDiv.innerHTML = html;
    }

    function scanNetworks() {
      document.getElementById('scanBtn').disabled = true;
      document.getElementById('scanBtn').textContent = 'Scanning...';
      showMessage('Scanning for networks...', 'info');
      
      // Mini loading overlay just for the scan
      showLoadingOverlay('Scanning for WiFi networks...', 'This may take a few seconds');
      
      fetch('/api/wifi/scan')
        .then(response => response.json())
        .then(data => {
          hideLoadingOverlay();
          displayNetworks(data.networks);
          showMessage(`Found ${data.networks.length} networks`, 'success');
        })
        .catch(error => {
          hideLoadingOverlay();
          console.error('Error scanning networks:', error);
          showMessage('Failed to scan networks', 'error');
        })
        .finally(() => {
          document.getElementById('scanBtn').disabled = false;
          document.getElementById('scanBtn').textContent = 'Scan Networks';
        });
    }

    function displayNetworks(networks) {
      const listDiv = document.getElementById('networkList');
      let html = '';
      
      networks.forEach(network => {
        html += `
          <div class="network-item" onclick="selectNetwork('${network.ssid}')">
            <div class="network-name">${network.ssid}</div>
            <div class="network-details">
              Signal: ${network.rssi} dBm | ${network.encrypted ? 'Secured' : 'Open'}
            </div>
          </div>
        `;
      });
      
      listDiv.innerHTML = html;
      listDiv.classList.remove('hidden');
    }

    function selectNetwork(ssid) {
      document.getElementById('ssid').value = ssid;
    }

    function showLoadingOverlay(text, subtext) {
      document.getElementById('loadingText').textContent = text || 'Processing...';
      if (subtext) {
        document.getElementById('loadingSubtext').textContent = subtext;
      }
      document.getElementById('loadingOverlay').style.display = 'flex';
    }
    
    function hideLoadingOverlay() {
      document.getElementById('loadingOverlay').style.display = 'none';
    }
    
    function connectToWiFi(ssid, password) {
      showMessage('Connecting to WiFi...', 'info');
      
      // Show loading overlay immediately
      showLoadingOverlay(
        'Connecting to "' + ssid + '"...',
        'The device will restart after saving credentials. This may take up to 30 seconds.'
      );
      
      fetch('/api/wifi/connect', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          ssid: ssid,
          password: password
        })
      })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          // Keep overlay visible but update text
          showLoadingOverlay(
            'WiFi credentials saved!',
            'Device is restarting... You will be disconnected. Please reconnect to your WiFi network after 20-30 seconds.'
          );
          
          // Don't reload - let user manually reconnect to their network
        } else {
          hideLoadingOverlay();
          showMessage('Error: ' + data.message, 'error');
        }
      })
      .catch(error => {
        console.error('Error connecting to WiFi:', error);
        hideLoadingOverlay();
        showMessage('Failed to connect to WiFi', 'error');
      });
    }

    function disconnectWiFi() {
      if (confirm('Are you sure you want to disconnect from WiFi?')) {
        fetch('/api/wifi/disconnect', {
          method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showMessage('Disconnected from WiFi', 'success');
            setTimeout(loadWiFiStatus, 2000);
          } else {
            showMessage('Error: ' + data.message, 'error');
          }
        })
        .catch(error => {
          console.error('Error disconnecting WiFi:', error);
          showMessage('Failed to disconnect', 'error');
        });
      }
    }

    function resetWiFi() {
      if (confirm('Are you sure you want to reset WiFi settings? This will clear all saved credentials and restart the device.')) {
        // Show loading overlay
        showLoadingOverlay(
          'Resetting WiFi settings...',
          'The device will restart shortly. Please wait.'
        );
        
        fetch('/api/wifi/reset', {
          method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            // Keep overlay visible with updated message
            showLoadingOverlay(
              'WiFi settings reset!',
              'Device is restarting... Please wait.'
            );
          } else {
            hideLoadingOverlay();
            showMessage('Error: ' + data.message, 'error');
          }
        })
        .catch(error => {
          console.error('Error resetting WiFi:', error);
          hideLoadingOverlay();
          showMessage('Failed to reset WiFi settings', 'error');
        });
      }
    }

    function showMessage(message, type) {
      const messageDiv = document.getElementById('statusMessage');
      messageDiv.textContent = message;
      messageDiv.className = `status-message ${type}`;
      messageDiv.classList.remove('hidden');
      
      if (type === 'success' || type === 'info') {
        setTimeout(() => {
          messageDiv.classList.add('hidden');
        }, 5000);
      }
    }

    function togglePasswordVisibility() {
      const passwordInput = document.getElementById('password');
      const toggleButton = document.querySelector('.password-toggle');
      
      if (passwordInput.type === 'password') {
        passwordInput.type = 'text';
        toggleButton.textContent = 'HIDE';
        toggleButton.title = 'Hide password';
      } else {
        passwordInput.type = 'password';
        toggleButton.textContent = 'SHOW';
        toggleButton.title = 'Show password';
      }
    }

    // Event listeners
    document.getElementById('scanBtn').addEventListener('click', scanNetworks);
    document.getElementById('disconnectBtn').addEventListener('click', disconnectWiFi);
    document.getElementById('resetBtn').addEventListener('click', resetWiFi);

    document.getElementById('wifiForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const ssid = document.getElementById('ssid').value;
      const password = document.getElementById('password').value;
      
      if (!ssid) {
        showMessage('Please enter a network name', 'error');
        return;
      }
      
      connectToWiFi(ssid, password);
    });

    // Initialize password toggle tooltip
    document.querySelector('.password-toggle').title = 'Show password';
  </script>
</body>
</html>
)rawliteral";

// Embedded HTML content for WiFi success page
const char WIFI_SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Configuration Saved</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }
    .container { max-width: 400px; margin: 0 auto; }
    h1 { color: #00aa00; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Configuration Saved!</h1>
    <p>WiFi credentials have been saved. The device will now restart and attempt to connect to your WiFi network.</p>
    <p>If the connection fails, the configuration portal will become available again.</p>
  </div>
</body>
</html>
)rawliteral";

#endif // WIFI_AP_WEB_H