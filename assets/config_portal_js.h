#ifndef CONFIG_PORTAL_JS_H
#define CONFIG_PORTAL_JS_H

#include <Arduino.h>

// Config portal page specific JavaScript (cleaned from HTML)
const char CONFIG_PORTAL_JS[] PROGMEM = R"rawliteral(
// Config Portal JavaScript
class ConfigPortal {
  constructor() {
    this.selectedNetwork = null;
    this.isScanning = false;
    this.init();
  }

  init() {
    this.bindEvents();
    // Auto-scan on page load
    setTimeout(() => this.scanNetworks(), 500);
  }

  bindEvents() {
    const scanBtn = document.getElementById('scan-button');
    const form = document.getElementById('wifi-form');
    const clearBtn = document.getElementById('clear-button');
    
    if (scanBtn) scanBtn.onclick = () => this.scanNetworks();
    if (clearBtn) clearBtn.onclick = () => this.clearForm();
    if (form) form.onsubmit = (e) => this.handleSubmit(e);
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
    const networkList = document.getElementById('network-list');
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
    
  async handleSubmit(event) {
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
      // Get CSRF token from automatically injected meta tag
      
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
        const form = document.getElementById('wifi-form');
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
    const scanText = document.getElementById('scan-text');
    const scanBtn = document.getElementById('scan-button');
    
    if (scanText) scanText.innerHTML = text;
    if (scanBtn) scanBtn.disabled = disabled;
  }

  showLoading(message) {
    const networkList = document.getElementById('network-list');
    UIUtils.showLoading(networkList, message);
  }

  showMessage(message) {
    const networkList = document.getElementById('network-list');
    UIUtils.showError(networkList, message);
  }
}
  
// Initialize when page loads
document.addEventListener('DOMContentLoaded', function() {
  window.configPortal = new ConfigPortal();
  
  // Add CSRF protection to the WiFi form as fallback
  const form = document.getElementById('wifi-form');
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

#endif // CONFIG_PORTAL_JS_H