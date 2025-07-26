#ifndef WIFI_AP_WEB_H
#define WIFI_AP_WEB_H // Embedded HTML content for WiFi configuration interface
const char WIFI_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>WiFi Management</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    :root {
      --primary-color: #0066cc;
      --primary-hover: #0055aa;
      --secondary-color: #6c757d;
      --secondary-hover: #5a6268;
      --danger-color: #dc3545;
      --danger-hover: #c82333;
      --success-color: #28a745;
      --info-color: #17a2b8;
      --warning-color: #ffc107;
      --background-color: #f8f9fa;
      --card-background: #ffffff;
      --text-color: #333333;
      --text-muted: #6c757d;
      --border-color: #dee2e6;
      --border-radius: 8px;
      --box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      line-height: 1.6;
      color: var(--text-color);
      background-color: var(--background-color);
      padding: 16px;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
    }
    
    .container {
      width: 100%;
      max-width: 500px;
      margin: 0 auto;
    }
    
    h1 {
      color: var(--primary-color);
      font-size: 1.8rem;
      margin-bottom: 16px;
      text-align: center;
    }
    
    h2 {
      font-size: 1.3rem;
      margin-bottom: 12px;
      color: var(--text-color);
    }
    
    .status-card {
      background-color: var(--card-background);
      border-radius: var(--border-radius);
      padding: 16px;
      margin-bottom: 16px;
      box-shadow: var(--box-shadow);
      transition: all 0.3s ease;
    }
    
    /* Loading overlay styles */
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
    
    .form-group {
      margin-bottom: 16px;
    }
    
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: 600;
      color: var(--text-color);
    }
    
    input[type="text"], input[type="password"], select {
      width: 100%;
      padding: 12px;
      border: 1px solid var(--border-color);
      border-radius: var(--border-radius);
      font-size: 16px; /* Prevent zoom on mobile */
      transition: border-color 0.3s ease;
      background-color: white;
    }
    
    input[type="text"]:focus, input[type="password"]:focus {
      outline: none;
      border-color: var(--primary-color);
      box-shadow: 0 0 0 3px rgba(0, 102, 204, 0.2);
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
      color: var(--secondary-color);
      font-size: 14px;
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
      color: var(--text-color);
    }
    
    .password-container input {
      padding-right: 60px;
    }
    
    .btn {
      background-color: var(--primary-color);
      color: white;
      padding: 12px 16px;
      border: none;
      border-radius: var(--border-radius);
      cursor: pointer;
      margin: 4px 4px 4px 0;
      font-size: 16px;
      font-weight: 500;
      min-width: 100px;
      transition: background-color 0.2s ease, transform 0.1s ease;
      text-align: center;
      display: inline-block;
      touch-action: manipulation;
    }
    
    .btn:active {
      transform: translateY(1px);
    }
    
    .btn:hover, .btn:focus {
      background-color: var(--primary-hover);
      outline: none;
    }
    
    .btn-danger {
      background-color: var(--danger-color);
    }
    
    .btn-danger:hover, .btn-danger:focus {
      background-color: var(--danger-hover);
    }
    
    .btn-secondary {
      background-color: var(--secondary-color);
    }
    
    .btn-secondary:hover, .btn-secondary:focus {
      background-color: var(--secondary-hover);
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
      border-radius: var(--border-radius);
      margin: 12px 0;
      font-weight: 500;
      box-shadow: var(--box-shadow);
      animation: fadeIn 0.3s ease;
    }
    
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(-10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .success {
      background-color: #d4edda;
      color: #155724;
      border-left: 4px solid var(--success-color);
    }
    
    .error {
      background-color: #f8d7da;
      color: #721c24;
      border-left: 4px solid var(--danger-color);
    }
    
    .info {
      background-color: #d1ecf1;
      color: #0c5460;
      border-left: 4px solid var(--info-color);
    }
    
    .network-list {
      max-height: 300px;
      overflow-y: auto;
      border: 1px solid var(--border-color);
      border-radius: var(--border-radius);
      margin-top: 12px;
      background-color: white;
      animation: fadeIn 0.3s ease;
      -webkit-overflow-scrolling: touch; /* Smooth scrolling on iOS */
    }
    
    .network-item {
      padding: 14px 16px;
      border-bottom: 1px solid var(--border-color);
      cursor: pointer;
      transition: background-color 0.2s ease;
      display: flex;
      align-items: center;
    }
    
    .network-item:hover, .network-item:active {
      background-color: rgba(0, 102, 204, 0.05);
    }
    
    .network-item:last-child {
      border-bottom: none;
    }
    
    .network-icon {
      margin-right: 12px;
      color: var(--text-muted);
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
      color: var(--text-muted);
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
    
    /* Responsive adjustments */
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
    
    /* Improve status display */
    .status-item {
      display: flex;
      margin-bottom: 8px;
      align-items: baseline;
    }
    
    .status-label {
      font-weight: 600;
      min-width: 120px;
      color: var(--text-muted);
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
      background-color: var(--primary-color);
      border-radius: 1px;
    }
    
    /* For touch devices */
    @media (hover: none) {
      .btn {
        padding: 14px 16px; /* Larger touch targets */
      }
      
      .network-item {
        padding: 16px; /* Larger touch area */
      }
    }
  </style>
</head><body>
  <!-- Loading overlay -->
  <div id="loadingOverlay" class="loading-overlay">
    <div class="spinner"></div>
    <div class="loading-text" id="loadingText">Connecting to WiFi...</div>
    <div class="loading-subtext" id="loadingSubtext">The device will restart after saving credentials. Please wait...</div>
  </div>
  
  <div class="container">
    <h1>WiFi Setup</h1>
    
    <div class="status-card">
      <h2>Current Status</h2>
      <div id="currentStatus">Loading...</div>
      <div id="statusMessage" class="status-message hidden"></div>
    </div>
    
    <div class="status-card">
      <h2>Available Networks</h2>
      <button id="scanBtn" class="btn btn-secondary btn-full">
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: text-bottom; margin-right: 6px;">
          <circle cx="12" cy="12" r="10"></circle>
          <line x1="12" y1="8" x2="12" y2="12"></line>
          <line x1="12" y1="16" x2="12.01" y2="16"></line>
        </svg>
        Scan for Networks
      </button>
      <div id="networkList" class="network-list hidden"></div>
    </div>
    
    <div class="status-card">
      <h2>Connect to Network</h2>
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
            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: text-bottom; margin-right: 6px;">
              <path d="M20 5H9l-7 7 7 7h11a2 2 0 0 0 2-2V7a2 2 0 0 0-2-2z"></path>
              <line x1="18" y1="9" x2="12" y2="15"></line>
              <line x1="12" y1="9" x2="18" y2="15"></line>
            </svg>
            Connect
          </button>
        </div>
      </form>
    </div>
    
    <div class="status-card">
      <h2>Actions</h2>
      <div class="button-group">
        <button id="disconnectBtn" class="btn btn-secondary">Disconnect</button>
        <button id="resetBtn" class="btn btn-danger">Reset WiFi</button>
        <a href="/" class="btn btn-secondary">Back to Main</a>
      </div>
    </div>
  </div><script>
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
        // Get signal strength indicator (0-4)
        const signalStrength = getSignalStrengthLevel(status.rssi);
        const signalBars = generateSignalBars(signalStrength);
        
        html = `
          <div class="status-item">
            <div class="status-label">Status:</div>
            <div class="status-value">Connected</div>
          </div>
          <div class="status-item">
            <div class="status-label">Network:</div>
            <div class="status-value">${status.ssid}</div>
          </div>
          <div class="status-item">
            <div class="status-label">IP Address:</div>
            <div class="status-value">${status.ip}</div>
          </div>
          <div class="status-item">
            <div class="status-label">Signal:</div>
            <div class="status-value">
              ${status.rssi} dBm
              <div class="signal-indicator">
                ${signalBars}
              </div>
            </div>
          </div>
          <div class="status-item">
            <div class="status-label">MAC Address:</div>
            <div class="status-value">${status.mac}</div>
          </div>
        `;
        document.getElementById('disconnectBtn').style.display = 'inline-block';
      } else {
        html = `
          <div class="status-item">
            <div class="status-label">Status:</div>
            <div class="status-value">${formatState(status.state)}</div>
          </div>
        `;
        
        if (status.saved_ssid) {
          html += `
            <div class="status-item">
              <div class="status-label">Saved Network:</div>
              <div class="status-value">${status.saved_ssid}</div>
            </div>
          `;
        }
        
        if (status.ap_ip) {
          html += `
            <div class="status-item">
              <div class="status-label">Setup IP:</div>
              <div class="status-value">${status.ap_ip}</div>
            </div>
          `;
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
      // Convert RSSI to a 0-4 signal strength level
      if (rssi >= -65) return 4;      // Excellent
      else if (rssi >= -70) return 3; // Good
      else if (rssi >= -80) return 2; // Fair
      else if (rssi >= -90) return 1; // Poor
      else return 0;                  // Very Poor
    }
    
    function generateSignalBars(level) {
      let bars = '';
      const maxBars = 4;
      
      for (let i = 0; i < maxBars; i++) {
        const height = 5 + (i * 2); // Increasing height bars: 5px, 7px, 9px, 11px
        const opacity = i < level ? '1' : '0.2';
        
        bars += `<div class="signal-bar" style="height: ${height}px; opacity: ${opacity}"></div>`;
      }
      
      return bars;
    }

    function scanNetworks() {
      const scanBtn = document.getElementById('scanBtn');
      scanBtn.disabled = true;
      scanBtn.innerHTML = `
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: text-bottom; margin-right: 6px; animation: spin 2s linear infinite;">
          <path d="M21 12a9 9 0 1 1-6.219-8.56"></path>
        </svg>
        Scanning...
      `;
      showMessage('Scanning for networks...', 'info');
      
      // Mini loading overlay for the scan
      showLoadingOverlay('Scanning for WiFi networks...', 'This may take a few seconds');
      
      fetch('/api/wifi/scan')
        .then(response => response.json())
        .then(data => {
          hideLoadingOverlay();
          displayNetworks(data.networks);
          showMessage(`Found ${data.networks.length} networks`, 'success');
          
          // Scroll to network list
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
          scanBtn.innerHTML = `
            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: text-bottom; margin-right: 6px;">
              <circle cx="12" cy="12" r="10"></circle>
              <line x1="12" y1="8" x2="12" y2="12"></line>
              <line x1="12" y1="16" x2="12.01" y2="16"></line>
            </svg>
            Scan for Networks
          `;
        });
    }

    function displayNetworks(networks) {
      // Sort networks by signal strength (strongest first)
      networks.sort((a, b) => b.rssi - a.rssi);
      
      const listDiv = document.getElementById('networkList');
      let html = '';
      
      if (networks.length === 0) {
        html = '<div style="padding: 16px; text-align: center; color: var(--text-muted);">No networks found</div>';
      } else {
        networks.forEach(network => {
          const signalStrength = getSignalStrengthLevel(network.rssi);
          const signalBars = generateSignalBars(signalStrength);
          
          html += `
            <div class="network-item" onclick="selectNetwork('${escapeHtml(network.ssid)}')">
              <div class="network-icon">
                <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
                  <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
                  <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
                  <line x1="12" y1="20" x2="12.01" y2="20"></line>
                </svg>
              </div>
              <div class="network-content">
                <div class="network-name">${escapeHtml(network.ssid)}</div>
                <div class="network-details">
                  <span class="signal-strength">${network.rssi} dBm</span>
                  <div class="signal-indicator">
                    ${signalBars}
                  </div>
                  <span class="security-icon" style="margin-left: 8px;">
                    ${network.encrypted ? 
                      '<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>' : 
                      '<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 9.9-1"></path></svg>'}
                  </span>
                </div>
              </div>
            </div>
          `;
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
      
      // Scroll to password field on mobile
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
      
      // Prevent scrolling when overlay is shown
      document.body.style.overflow = 'hidden';
    }
    
    function hideLoadingOverlay() {
      document.getElementById('loadingOverlay').style.display = 'none';
      
      // Restore scrolling
      document.body.style.overflow = '';
    }
    
    function connectToWiFi(ssid, password) {
      showMessage('Connecting to WiFi...', 'info');
      
      // Show loading overlay immediately
      showLoadingOverlay(
        'Connecting to "' + escapeHtml(ssid) + '"...',
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
      
      // Scroll to message if not visible
      const rect = messageDiv.getBoundingClientRect();
      if (rect.top < 0 || rect.bottom > window.innerHeight) {
        messageDiv.scrollIntoView({
          behavior: 'smooth',
          block: 'center'
        });
      }
      
      // Auto-hide success and info messages
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
      
      // Keep focus on the password field
      passwordInput.focus();
    }

    // Event listeners
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

    // Initialize password toggle tooltip
    document.querySelector('.password-toggle').title = 'Show password';
    
    // Add touch-friendly focus styles for mobile
    document.querySelectorAll('input').forEach(input => {
      input.addEventListener('focus', function() {
        this.style.boxShadow = '0 0 0 3px rgba(0, 102, 204, 0.2)';
      });
      
      input.addEventListener('blur', function() {
        this.style.boxShadow = '';
      });
    });
  </script>
</body>
</html>
)rawliteral";         // Embedded HTML content for WiFi success page
const char WIFI_SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Configuration Saved</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    :root {
      --success-color: #28a745;
      --text-color: #333333;
      --background-color: #f8f9fa;
      --card-background: #ffffff;
      --border-radius: 8px;
      --box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      line-height: 1.6;
      color: var(--text-color);
      background-color: var(--background-color);
      padding: 20px;
      text-align: center;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
      display: flex;
      flex-direction: column;
      justify-content: center;
      min-height: 100vh;
    }
    
    .container {
      max-width: 400px;
      margin: 0 auto;
      background-color: var(--card-background);
      border-radius: var(--border-radius);
      padding: 30px 20px;
      box-shadow: var(--box-shadow);
    }
    
    .success-icon {
      width: 80px;
      height: 80px;
      margin: 0 auto 20px;
      border-radius: 50%;
      background-color: var(--success-color);
      display: flex;
      align-items: center;
      justify-content: center;
      color: white;
    }
    
    h1 {
      color: var(--success-color);
      margin-bottom: 20px;
      font-size: 1.8rem;
    }
    
    p {
      margin-bottom: 15px;
      color: var(--text-color);
    }
    
    .countdown {
      font-size: 1.2rem;
      font-weight: bold;
      margin: 20px 0;
      color: var(--success-color);
    }
    
    @media (max-width: 375px) {
      .container {
        padding: 20px 15px;
      }
      
      h1 {
        font-size: 1.5rem;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="success-icon">
      <svg xmlns="http://www.w3.org/2000/svg" width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
        <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
        <polyline points="22 4 12 14.01 9 11.01"></polyline>
      </svg>
    </div>
    <h1>Configuration Saved!</h1>
    <p>WiFi credentials have been saved. The device will now restart and attempt to connect to your WiFi network.</p>
    <p>If the connection fails, the configuration portal will become available again.</p>
    <div class="countdown" id="countdown">Restarting in <span id="timer">30</span> seconds...</div>
  </div>
  
  <script>
    // Simple countdown timer
    let seconds = 30;
    const timerElement = document.getElementById('timer');
    
    const countdown = setInterval(() => {
      seconds--;
      timerElement.textContent = seconds;
      
      if (seconds <= 0) {
        clearInterval(countdown);
        timerElement.parentElement.textContent = "Device is restarting...";
      }
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

#endif // WIFI_AP_WEB_H