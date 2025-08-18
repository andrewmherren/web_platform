#ifndef WIFI_AP_WEB_H
#define WIFI_AP_WEB_H

const char WIFI_CONFIG_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <title>WiFi Management - Ticker</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <link rel="stylesheet" href="/assets/style.css" type="text/css">
  <style>
    /* WiFi-specific styles that extend the global theme */
    
    .form-group {
      margin-bottom: 16px;
    }
    
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: 600;
      color: #ffffff;
    }
    
    input[type="text"], input[type="password"], select {
      width: 100%;
      padding: 12px;
      border: 1px solid rgba(255, 255, 255, 0.3);
      border-radius: 10px;
      font-size: 16px;
      transition: border-color 0.3s ease;
      background: rgba(255, 255, 255, 0.1);
      color: white;
    }
    
    input[type="text"]:focus, input[type="password"]:focus {
      outline: none;
      border-color: rgba(255, 255, 255, 0.6);
      box-shadow: 0 0 0 3px rgba(255, 255, 255, 0.2);
      background: rgba(255, 255, 255, 0.15);
    }
    
    input::placeholder {
      color: rgba(255, 255, 255, 0.7);
    }
    
    .password-container {
      position: relative;
      display: flex;
      align-items: center;
    }
    
    .password-toggle {
      position: absolute;
      right: 12px;
      cursor: pointer;
      user-select: none;
      color: rgba(255, 255, 255, 0.7);
      font-size: 12px;
      font-weight: bold;
      z-index: 10;
      background: none;
      border: none;
      padding: 0 8px;
      height: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    
    .password-toggle:hover {
      color: rgba(255, 255, 255, 1);
    }
    
    .password-container input {
      padding-right: 60px;
    }
    
    .btn, .nav-links a {
      background: rgba(255, 255, 255, 0.2);
      color: white;
      text-decoration: none;
      padding: 12px 24px;
      margin: 4px 4px 4px 0;
      border-radius: 25px;
      border: 1px solid rgba(255, 255, 255, 0.3);
      cursor: pointer;
      font-size: 16px;
      font-weight: 500;
      min-width: 100px;
      transition: all 0.3s ease;
      text-align: center;
      display: inline-block;
      touch-action: manipulation;
    }
    
    .btn:hover, .btn:focus, .nav-links a:hover {
      background: rgba(255, 255, 255, 0.3);
      transform: translateY(-2px);
      outline: none;
    }
    
    .btn:active {
      transform: translateY(0px);
    }
    
    .btn-danger {
      background: rgba(244, 67, 54, 0.3);
      border-color: rgba(244, 67, 54, 0.5);
    }
    
    .btn-danger:hover, .btn-danger:focus {
      background: rgba(244, 67, 54, 0.5);
    }
    
    .btn-secondary {
      background: rgba(108, 117, 125, 0.3);
      border-color: rgba(108, 117, 125, 0.5);
    }
    
    .btn-secondary:hover, .btn-secondary:focus {
      background: rgba(108, 117, 125, 0.5);
    }
    
    .btn-full {
      width: 100%;
      margin: 8px 0;
    }
    
    .hidden {
      display: none;
    }
    
    .status-message {
      padding: 12px;
      border-radius: 10px;
      margin: 12px 0;
      font-weight: 500;
      animation: fadeIn 0.3s ease;
    }
    
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(-10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .success {
      background: rgba(76, 175, 80, 0.2);
      color: #4CAF50;
      border-left: 4px solid #4CAF50;
      font-weight: bold;
    }
    
    .error {
      background: rgba(244, 67, 54, 0.2);
      color: #f44336;
      border-left: 4px solid #f44336;
      font-weight: bold;
    }
    
    .info {
      background: rgba(33, 150, 243, 0.2);
      color: #2196F3;
      border-left: 4px solid #2196F3;
      font-weight: bold;
    }
    
    .network-list {
      max-height: 300px;
      overflow-y: auto;
      border: 1px solid rgba(255, 255, 255, 0.2);
      border-radius: 10px;
      margin-top: 12px;
      background: rgba(255, 255, 255, 0.05);
      animation: fadeIn 0.3s ease;
      -webkit-overflow-scrolling: touch;
    }
    
    .network-item {
      padding: 14px 16px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.1);
      cursor: pointer;
      transition: background-color 0.2s ease;
      display: flex;
      align-items: center;
      color: white;
    }
    
    .network-item:hover, .network-item:active {
      background: rgba(255, 255, 255, 0.1);
    }
    
    .network-item:last-child {
      border-bottom: none;
    }
    
    .network-icon {
      margin-right: 12px;
      color: rgba(255, 255, 255, 0.7);
      font-size: 1.2rem;
    }
    
    .network-content {
      flex: 1;
    }
    
    .network-name {
      font-weight: 600;
      margin-bottom: 2px;
    }
    
    .network-details {
      font-size: 0.85rem;
      color: rgba(255, 255, 255, 0.7);
      display: flex;
      align-items: center;
    }
    
    .signal-strength {
      display: inline-block;
      margin-right: 8px;
    }
    
    .security-icon {
      margin-left: 4px;
    }
    
    .button-group {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    
    .loading-overlay {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background-color: rgba(0, 0, 0, 0.8);
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      z-index: 1000;
      color: white;
      display: none;
      backdrop-filter: blur(3px);
      -webkit-backdrop-filter: blur(3px);
    }
    
    .spinner {
      border: 4px solid rgba(255, 255, 255, 0.3);
      border-top: 4px solid white;
      border-radius: 50%;
      width: 40px;
      height: 40px;
      animation: spin 1s linear infinite;
      margin-bottom: 24px;
    }
    
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    
    .loading-text {
      font-size: 1.5rem;
      margin-bottom: 12px;
      text-align: center;
      padding: 0 24px;
    }
    
    .loading-subtext {
      font-size: 1rem;
      max-width: 85%;
      text-align: center;
      opacity: 0.8;
      padding: 0 24px;
    }
    
    .status-item {
      display: flex;
      margin-bottom: 8px;
      align-items: baseline;
    }
    
    .status-label {
      font-weight: 600;
      min-width: 120px;
      color: rgba(255, 255, 255, 0.7);
    }
    
    .status-value {
      flex: 1;
    }
    
    .signal-indicator {
      display: inline-flex;
      align-items: center;
      margin-left: 6px;
    }
    
    .signal-bar {
      width: 4px;
      margin-right: 1px;
      background-color: #2196F3;
      border-radius: 1px;
    }
    
    @media (max-width: 375px) {
      body {
        padding: 12px 8px;
      }
      
      h1 {
        font-size: 1.5rem;
      }
      
      .status-card {
        padding: 12px;
      }
      
      .btn {
        padding: 10px 14px;
        font-size: 14px;
      }
    }
    
    @media (hover: none) {
      .btn {
        padding: 14px 16px;
      }
      
      .network-item {
        padding: 16px;
      }
    }
  </style>
</head>
<body>
  <div id="loadingOverlay" class="loading-overlay">
    <div class="spinner"></div>
    <div class="loading-text" id="loadingText">Connecting to WiFi...</div>
    <div class="loading-subtext" id="loadingSubtext">The device will restart after saving credentials. Please wait...</div>
  </div>
    
  <div class="container">
    <h1>WiFi Management</h1>
    
    <div class="status-card">
      <h3>Current Status</h3>
      <div id="currentStatus">Loading...</div>
      <div id="statusMessage" class="status-message hidden"></div>
    </div>
      
    <div class="status-card">
      <h3>Available Networks</h3>
      <button id="scanBtn" class="btn btn-secondary btn-full">
        Scan for Networks
      </button>
      <div id="networkList" class="network-list hidden"></div>
    </div>
    
    <div class="status-card">
      <h3>Connect to Network</h3>
      <form id="wifiForm">
        <div class="form-group">
          <label for="ssid">WiFi Network Name (SSID):</label>
          <input type="text" id="ssid" name="ssid" placeholder="Enter network name or select from scan" required autocapitalize="none">
        </div>
        
        <div class="form-group">
          <label for="password">WiFi Password:</label>
          <div class="password-container">
            <input type="password" id="password" name="password" placeholder="Network password (if required)">
            <button type="button" class="password-toggle" onclick="togglePasswordVisibility()">SHOW</button>
          </div>
        </div>
        
        <div class="button-group">
          <button type="submit" class="btn btn-full">
            Connect
          </button>
        </div>
      </form>
    </div>
    
    <div class="status-card">
      <h3>Actions</h3>
      <div class="button-group">
        <button id="disconnectBtn" class="btn btn-secondary">Disconnect</button>
        <button id="resetBtn" class="btn btn-danger">Reset WiFi</button>
      </div>
    </div>

    <!-- Navigation menu will be auto-injected here -->

    <div style="text-align: center; margin-top: 20px; opacity: 0.7; font-size: 0.9em;">
      <p>Ticker WiFi Management</p>
    </div>
  </div>

  <script>
    let currentWiFiStatus = null;

    window.onload = function() {
      loadWiFiStatus();
    };

    function loadWiFiStatus() {
      fetch('/wifi/api/status')
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
        const signalStrength = getSignalStrengthLevel(status.rssi);
        const signalBars = generateSignalBars(signalStrength);
        
        html = '<div class="status-item"><div class="status-label">Status:</div><div class="status-value">Connected</div></div>';
        html += '<div class="status-item"><div class="status-label">Network:</div><div class="status-value">' + status.ssid + '</div></div>';
        html += '<div class="status-item"><div class="status-label">IP Address:</div><div class="status-value">' + status.ip + '</div></div>';
        html += '<div class="status-item"><div class="status-label">Signal:</div><div class="status-value">' + status.rssi + ' dBm<div class="signal-indicator">' + signalBars + '</div></div></div>';
        html += '<div class="status-item"><div class="status-label">MAC Address:</div><div class="status-value">' + status.mac + '</div></div>';
        document.getElementById('disconnectBtn').style.display = 'inline-block';
      } else {
        html = '<div class="status-item"><div class="status-label">Status:</div><div class="status-value">' + formatState(status.state) + '</div></div>';
        
        if (status.saved_ssid) {
          html += '<div class="status-item"><div class="status-label">Saved Network:</div><div class="status-value">' + status.saved_ssid + '</div></div>';
        }
        
        if (status.ap_ip) {
          html += '<div class="status-item"><div class="status-label">Setup IP:</div><div class="status-value">' + status.ap_ip + '</div></div>';
        }
        
        document.getElementById('disconnectBtn').style.display = 'none';
      }
      
      statusDiv.innerHTML = html;
    }
    
    function formatState(state) {
      switch(state) {
        case 'connected': return 'Connected';
        case 'connecting': return 'Connecting...';
        case 'setup': return 'Setup Mode';
        case 'failed': return 'Connection Failed';
        default: return state;
      }
    }
    
    function getSignalStrengthLevel(rssi) {
      if (rssi >= -65) return 4;
      else if (rssi >= -70) return 3;
      else if (rssi >= -80) return 2;
      else if (rssi >= -90) return 1;
      else return 0;
    }
    
    function generateSignalBars(level) {
      let bars = '';
      const maxBars = 4;
      
      for (let i = 0; i < maxBars; i++) {
        const height = 5 + (i * 2);
        const opacity = i < level ? '1' : '0.2';
        
        bars += '<div class="signal-bar" style="height: ' + height + 'px; opacity: ' + opacity + '"></div>';
      }
      
      return bars;
    }

    function scanNetworks() {
      const scanBtn = document.getElementById('scanBtn');
      scanBtn.disabled = true;
      scanBtn.innerHTML = 'Scanning...';
      showMessage('Scanning for networks...', 'info');
      
      showLoadingOverlay('Scanning for WiFi networks...', 'This may take a few seconds');
      
      fetch('/wifi/api/scan')
        .then(response => response.json())
        .then(data => {
          hideLoadingOverlay();
          displayNetworks(data.networks);
          showMessage('Found ' + data.networks.length + ' networks', 'success');
          
          document.getElementById('networkList').scrollIntoView({
            behavior: 'smooth',
            block: 'start'
          });
        })
        .catch(error => {
          hideLoadingOverlay();
          console.error('Error scanning networks:', error);
          showMessage('Failed to scan networks', 'error');
        })
        .finally(() => {
          scanBtn.disabled = false;
          scanBtn.innerHTML = 'Scan for Networks';
        });
    }

    function displayNetworks(networks) {
      networks.sort((a, b) => b.rssi - a.rssi);
      
      const listDiv = document.getElementById('networkList');
      let html = '';
      
      if (networks.length === 0) {
        html = '<div style="padding: 16px; text-align: center; color: rgba(255, 255, 255, 0.7);">No networks found</div>';
      } else {
        networks.forEach(network => {
          const signalStrength = getSignalStrengthLevel(network.rssi);
          const signalBars = generateSignalBars(signalStrength);
          
          html += '<div class="network-item" onclick="selectNetwork(\'' + escapeHtml(network.ssid) + '\')">';
          html += '<div class="network-icon">📶</div>';
          html += '<div class="network-content">';
          html += '<div class="network-name">' + escapeHtml(network.ssid) + '</div>';
          html += '<div class="network-details">';
          html += '<span class="signal-strength">' + network.rssi + ' dBm</span>';
          html += '<div class="signal-indicator">' + signalBars + '</div>';
          html += '<span class="security-icon" style="margin-left: 8px;">' + (network.encrypted ? '🔒' : '🔓') + '</span>';
          html += '</div>';
          html += '</div>';
          html += '</div>';
        });
      }
      
      listDiv.innerHTML = html;
      listDiv.classList.remove('hidden');
    }
    
    function escapeHtml(text) {
      return text
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
    }

    function selectNetwork(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('password').focus();
      
      if (window.innerWidth < 768) {
        document.getElementById('password').scrollIntoView({
          behavior: 'smooth',
          block: 'center'
        });
      }
    }

    function showLoadingOverlay(text, subtext) {
      document.getElementById('loadingText').textContent = text || 'Processing...';
      if (subtext) {
        document.getElementById('loadingSubtext').textContent = subtext;
      }
      document.getElementById('loadingOverlay').style.display = 'flex';
      document.body.style.overflow = 'hidden';
    }
    
    function hideLoadingOverlay() {
      document.getElementById('loadingOverlay').style.display = 'none';
      document.body.style.overflow = '';
    }
    
    function connectToWiFi(ssid, password) {
      showMessage('Connecting to WiFi...', 'info');
      
      showLoadingOverlay(
        'Connecting to "' + escapeHtml(ssid) + '"...',
        'The device will restart after saving credentials. This may take up to 30 seconds.'
      );
      
      fetch('/wifi/api/connect', {
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
          showLoadingOverlay(
            'WiFi credentials saved!',
            'Device is restarting... You will be disconnected. Please reconnect to your WiFi network after 20-30 seconds.'
          );
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
        fetch('/wifi/api/disconnect', {
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
        showLoadingOverlay(
          'Resetting WiFi settings...',
          'The device will restart shortly. Please wait.'
        );
        
        fetch('/wifi/api/reset', {
          method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
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
      messageDiv.className = 'status-message ' + type;
      messageDiv.classList.remove('hidden');
      
      const rect = messageDiv.getBoundingClientRect();
      if (rect.top < 0 || rect.bottom > window.innerHeight) {
        messageDiv.scrollIntoView({
          behavior: 'smooth',
          block: 'center'
        });
      }
      
      if (type === 'success' || type === 'info') {
        setTimeout(() => {
          messageDiv.style.opacity = '0';
          setTimeout(() => {
            messageDiv.classList.add('hidden');
            messageDiv.style.opacity = '1';
          }, 300);
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
      
      passwordInput.focus();
    }

    document.getElementById('scanBtn').addEventListener('click', scanNetworks);
    document.getElementById('disconnectBtn').addEventListener('click', disconnectWiFi);
    document.getElementById('resetBtn').addEventListener('click', resetWiFi);

    document.getElementById('wifiForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const ssid = document.getElementById('ssid').value.trim();
      const password = document.getElementById('password').value;
      
      if (!ssid) {
        showMessage('Please enter a network name', 'error');
        document.getElementById('ssid').focus();
        return;
      }
      
      connectToWiFi(ssid, password);
    });

    document.querySelector('.password-toggle').title = 'Show password';
    
    document.querySelectorAll('input').forEach(input => {
      input.addEventListener('focus', function() {
        this.style.boxShadow = '0 0 0 3px rgba(255, 255, 255, 0.2)';
      });
      
      input.addEventListener('blur', function() {
        this.style.boxShadow = '';
      });
    });
  </script>
</body>
</html>
)rawliteral";

const char WIFI_SUCCESS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <title>Configuration Saved - Ticker</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <link rel="stylesheet" href="/assets/style.css" type="text/css">
  <style>
    /* Additional styles for success page */
    body {
      text-align: center;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }
    
    .container {
      max-width: 500px;
      margin: 0 auto;
      background: rgba(255, 255, 255, 0.1);
      padding: 40px 30px;
      border-radius: 15px;
      backdrop-filter: blur(10px);
      -webkit-backdrop-filter: blur(10px);
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
      border: 1px solid rgba(255, 255, 255, 0.2);
    }
    
    .success-icon {
      width: 100px;
      height: 100px;
      margin: 0 auto 30px;
      border-radius: 50%;
      background: rgba(76, 175, 80, 0.3);
      border: 3px solid #4CAF50;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #4CAF50;
      font-size: 3em;
    }
    
    h1 {
      color: #ffffff;
      margin-bottom: 25px;
      font-size: 2.2rem;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
    }
    
    p {
      margin-bottom: 20px;
      color: rgba(255, 255, 255, 0.9);
      font-size: 1.1rem;
    }
    
    .countdown {
      background: rgba(76, 175, 80, 0.2);
      border: 1px solid rgba(76, 175, 80, 0.5);
      border-radius: 10px;
      padding: 20px;
      margin: 25px 0;
      font-size: 1.3rem;
      font-weight: bold;
      color: #4CAF50;
    }
    
    @media (max-width: 375px) {
      .container {
        padding: 30px 20px;
      }
      
      h1 {
        font-size: 1.8rem;
      }
      
      .success-icon {
        width: 80px;
        height: 80px;
        font-size: 2.5em;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="success-icon">
      ✓
    </div>
    <h1>Configuration Saved!</h1>
    <p>WiFi credentials have been saved successfully. The device will now restart and attempt to connect to your WiFi network.</p>
    <p>If the connection fails, the configuration portal will become available again.</p>
    <div class="countdown" id="countdown">
      Restarting in <span id="timer">30</span> seconds...
    </div>
  </div>
  
  <script>
    let seconds = 30;
    const timerElement = document.getElementById('timer');
    
    const countdown = setInterval(() => {
      seconds--;
      timerElement.textContent = seconds;
      
      if (seconds <= 0) {
        clearInterval(countdown);
        timerElement.parentElement.innerHTML = "Device is restarting...";
      }
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

#endif // WIFI_AP_WEB_H