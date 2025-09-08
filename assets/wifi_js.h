#ifndef WIFI_JS_H
#define WIFI_JS_H

#include <Arduino.h>

// Unified WiFi management JavaScript for both config portal and WiFi management pages
const char WIFI_JS[] PROGMEM = R"rawliteral(
// Unified WiFi JavaScript Handler
class WiFiHandler {
  constructor(options = {}) {
    // Configuration options for different pages
    this.config = {
      scanButtonId: options.scanButtonId || 'scan-button',
      formId: options.formId || 'wifi-form',
      networkListId: options.networkListId || 'network-list',
      clearButtonId: options.clearButtonId || 'clear-button',
      scanTextId: options.scanTextId || 'scan-text',
      loadNetworkStatus: options.loadNetworkStatus || false,
      instanceName: options.instanceName || 'wifiHandler',
      ...options
    };
    
    this.selectedNetwork = null;
    this.isScanning = false;
    this.init();
  }

  init() {
    this.bindEvents();
    // Auto-scan on page load
    setTimeout(() => this.scanNetworks(), 500);
    
    // Load current network status if enabled
    if (this.config.loadNetworkStatus) {
      this.loadCurrentNetworkStatus();
    }
  }
    
  bindEvents() {
    const scanBtn = document.getElementById(this.config.scanButtonId);
    const form = document.getElementById(this.config.formId);
    const clearBtn = document.getElementById(this.config.clearButtonId);

    if (scanBtn) scanBtn.onclick = () => this.scanNetworks();
    if (form) form.onsubmit = (e) => this.handleFormSubmit(e);
    if (clearBtn) clearBtn.onclick = () => this.clearForm();
  }

  async scanNetworks() {
    if (this.isScanning) return;
    
    this.isScanning = true;
    this.updateScanButton('⟳ Scanning...', true);
    this.showLoading('Scanning for networks...');
    
    try {
      const data = await NetworkUtils.scanNetworks();
      
      if (data.networks && data.networks.length > 0) {
        this.displayNetworks(data.networks);
      } else {
        this.showMessage('No networks found. Try scanning again.');
      }
    } catch (error) {
      console.error('Scan error:', error);
      this.showMessage('Scan failed. Please try again.');
    } finally {
      this.isScanning = false;
      this.updateScanButton('🔍 Scan for Networks', false);
    }
  }
    
  displayNetworks(networks) {
    const networkList = document.getElementById(this.config.networkListId);
    if (!networkList) return;
    
    networkList.innerHTML = '';
    
    networks.forEach(network => {
      const item = NetworkUtils.renderNetworkItem(network, (ssid, element) => {
        this.selectNetwork(ssid, element);
      });
      networkList.appendChild(item);
    });
  }

  selectNetwork(ssid, element) {
    UIUtils.selectNetworkItem(element);
    this.selectedNetwork = ssid;
    
    const ssidInput = document.getElementById('ssid');
    const passwordInput = document.getElementById('password');
    
    if (ssidInput) ssidInput.value = ssid;
    if (passwordInput) passwordInput.focus();
  }
    
  clearForm() {
    const ssidInput = document.getElementById('ssid');
    const passwordInput = document.getElementById('password');
    
    if (ssidInput) ssidInput.value = '';
    if (passwordInput) passwordInput.value = '';
    
    this.selectedNetwork = null;
    UIUtils.selectNetworkItem(null);
  }

  // WiFi management specific method
  resetWiFi() {
    UIUtils.showConfirm(
      'Reset WiFi Settings',
      'This will reset all WiFi settings and restart the device in configuration mode. This action cannot be undone.',
      () => {
        AuthUtils.fetch('/api/reset', { method: 'POST' })
          .then(() => {
            UIUtils.showAlert('WiFi Reset', 'WiFi settings have been reset. The device will restart in configuration mode.', 'info');
          })
          .catch(() => {
            // Ignore errors as device may restart quickly
          });
      }
    );
  }
    
  async handleFormSubmit(event) {
    event.preventDefault();
    
    const ssidInput = document.getElementById('ssid');
    const passwordInput = document.getElementById('password');
    const ssid = ssidInput ? ssidInput.value.trim() : '';
    const password = passwordInput ? passwordInput.value : '';
    
    if (!ssid) {
      UIUtils.showAlert('Network Required', 'Please select a network or enter a network name to continue.', 'warning');
      return false;
    }
    
    const submitBtn = event.target.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, '⟳ Connecting...');
    
    try {
      const response = await AuthUtils.fetchJSON('/api/wifi', {
        method: 'POST',
        body: JSON.stringify({
          ssid: ssid,
          password: password
        })
      });
      
      const data = await response.json();
      
      if (data.success) {
        UIUtils.showAlert('Success!', 
          `WiFi credentials saved for network "${data.ssid}". Device will restart to connect.`, 
          'success');
        
        // Disable form to prevent double-submission
        const form = document.getElementById(this.config.formId);
        if (form) {
          const inputs = form.querySelectorAll('input, button');
          inputs.forEach(input => input.disabled = true);
        }
      } else {
        UIUtils.showAlert('Configuration Failed', data.error || 'Failed to save WiFi credentials', 'error');
        UIUtils.updateButtonState(submitBtn, false, 'Connect to WiFi');
      }
    } catch (error) {
      console.error('WiFi config error:', error);
      UIUtils.showAlert('Connection Error', 'Failed to communicate with device', 'error');
      UIUtils.updateButtonState(submitBtn, false, 'Connect to WiFi');
    }
    
    return false;
  }
  
  updateScanButton(text, disabled) {
    const scanText = document.getElementById(this.config.scanTextId);
    const scanBtn = document.getElementById(this.config.scanButtonId);
    
    if (scanText) scanText.innerHTML = text;
    if (scanBtn) scanBtn.disabled = disabled;
  }

  showLoading(message) {
    const networkList = document.getElementById(this.config.networkListId);
    UIUtils.showLoading(networkList, message);
  }

  showMessage(message) {
    const networkList = document.getElementById(this.config.networkListId);
    UIUtils.showError(networkList, message);
  }
  
  // Load current network status for WiFi management page
  async loadCurrentNetworkStatus() {
    try {
      const response = await AuthUtils.fetch('/api/network');
      const data = await response.json();
      
      const elements = {
        'currentSsid': data?.network?.ssid || 'Not connected',
        'signalStrength': data?.network?.signalStrength ? data.network.signalStrength + ' dBm' : 'N/A',
        'ipAddress': data?.network?.ipAddress || 'N/A',
        'macAddress': data?.network?.macAddress || 'N/A'
      };
      
      Object.entries(elements).forEach(([id, value]) => {
        const element = document.getElementById(id);
        if (element) element.textContent = value;
      });
    } catch (error) {
      console.error('Failed to fetch WiFi status:', error);
      
      // Set error messages
      ['currentSsid', 'signalStrength', 'ipAddress', 'macAddress'].forEach(id => {
        const element = document.getElementById(id);
        if (element) element.textContent = 'Error loading data';
      });
    }
  }
}

// Password toggle functionality - shared by both pages
function togglePassword() {
  const passwordField = document.getElementById('password');
  const toggleButton = document.querySelector('.password-toggle');
  
  if (!passwordField || !toggleButton) return;
  
  if (passwordField.type === 'password') {
    passwordField.type = 'text';
    toggleButton.innerHTML = 'Hide';
    toggleButton.title = 'Hide password';
  } else {
    passwordField.type = 'password';
    toggleButton.innerHTML = 'Show';
    toggleButton.title = 'Show password';
  }
}

// Auto-initialization - detects page type and configures appropriately
document.addEventListener('DOMContentLoaded', function() {
  // Detect which page we're on based on form ID
  const configForm = document.getElementById('wifi-form');
  const managementForm = document.getElementById('connectForm');
  
  let wifiHandler;
  
  if (configForm) {
    // Config portal page
    window.configPortal = window.wifiHandler = new WiFiHandler({
      scanButtonId: 'scan-button',
      formId: 'wifi-form',
      networkListId: 'network-list',
      clearButtonId: 'clear-button',
      scanTextId: 'scan-text',
      loadNetworkStatus: false,
      instanceName: 'configPortal'
    });
    wifiHandler = window.configPortal;
  } else if (managementForm) {
    // WiFi management page
    window.wifiManager = window.wifiHandler = new WiFiHandler({
      scanButtonId: 'scan-button',
      formId: 'connectForm',
      networkListId: 'network-list',
      clearButtonId: 'clear-button',
      scanTextId: 'scan-text',
      loadNetworkStatus: true,
      instanceName: 'wifiManager'
    });
    wifiHandler = window.wifiManager;
  }
  
  // Add CSRF protection to the form as fallback
  const form = configForm || managementForm;
  if (form) {
    form.addEventListener('submit', function(e) {
      const csrf = AuthUtils.getCsrfToken();
      if (csrf) {
        // Remove any existing CSRF input
        const existingCsrf = form.querySelector('input[name="_csrf"]');
        if (existingCsrf) {
          existingCsrf.remove();
        }
        
        // Add fresh CSRF token
        const input = document.createElement('input');
        input.type = 'hidden';
        input.name = '_csrf';
        input.value = csrf;
        form.appendChild(input);
      }
    });
  }
});
)rawliteral";

#endif // WIFI_JS_H