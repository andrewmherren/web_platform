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

  handleSubmit(event) {
    const ssidInput = document.getElementById('ssid');
    const ssid = ssidInput ? ssidInput.value.trim() : '';
    
    if (!ssid) {
      event.preventDefault();
      UIUtils.showAlert('Network Required', 'Please select a network or enter a network name to continue.', 'warning');
      return false;
    }
    
    const submitBtn = event.target.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, '⟳ Connecting...');
    
    return true;
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
});
)rawliteral";

#endif // CONFIG_PORTAL_JS_H