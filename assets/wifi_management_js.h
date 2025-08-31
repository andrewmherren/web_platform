#ifndef WIFI_MANAGEMENT_JS_H
#define WIFI_MANAGEMENT_JS_H

#include <Arduino.h>

// WiFi management page specific JavaScript
const char WIFI_MANAGEMENT_JS[] PROGMEM = R"rawliteral(
// WiFi Management JavaScript
class WiFiManager {
  constructor() {
    this.init();
  }

  init() {
    this.bindEvents();
  }

  bindEvents() {
    const scanBtn = document.getElementById('scan-btn');
    const clearBtn = document.getElementById('clear-btn');
    const resetBtn = document.getElementById('reset-btn');
    const form = document.getElementById('wifi-form');

    if (scanBtn) scanBtn.onclick = () => this.scanNetworks();
    if (clearBtn) clearBtn.onclick = () => this.clearForm();
    if (resetBtn) resetBtn.onclick = () => this.resetWiFi();
    if (form) form.onsubmit = (e) => this.connectToNetwork(e);
  }

  async scanNetworks() {
    const btn = document.getElementById('scan-btn');
    const list = document.getElementById('network-list');
    
    UIUtils.updateButtonState(btn, true, 'Scanning...');
    UIUtils.showLoading(list, 'Scanning for networks...');
    
    try {
      const data = await NetworkUtils.scanNetworks();
      
      if (data.networks && data.networks.length > 0) {
        this.displayNetworks(data.networks, list);
      } else {
        UIUtils.showError(list, 'No networks found');
      }
    } catch (error) {
      console.error('Scan failed:', error);
      UIUtils.showError(list, 'Scan failed. Please try again.');
    } finally {
      UIUtils.updateButtonState(btn, false, 'Scan Networks');
    }
  }

  displayNetworks(networks, container) {
    container.innerHTML = '';
    
    networks.forEach(network => {
      const item = document.createElement('div');
      item.className = 'network-item';
      item.style.cursor = 'pointer';
      
      const nameDiv = document.createElement('div');
      nameDiv.innerHTML = `<strong>${NetworkUtils.escapeHtml(network.ssid)}</strong> ${network.encryption ? 'SECURED' : 'OPEN'}`;
      
      const signalDiv = document.createElement('div');
      signalDiv.className = 'signal-strength';
      signalDiv.textContent = network.rssi + ' dBm';
      
      item.appendChild(nameDiv);
      item.appendChild(signalDiv);
      
      item.onclick = () => {
        document.getElementById('ssid').value = network.ssid;
        document.getElementById('password').focus();
      };
      
      container.appendChild(item);
    });
  }

  clearForm() {
    const ssidInput = document.getElementById('ssid');
    const passwordInput = document.getElementById('password');
    
    if (ssidInput) ssidInput.value = '';
    if (passwordInput) passwordInput.value = '';
  }

  resetWiFi() {
    if (!confirm('This will reset all WiFi settings and restart the device. Continue?')) {
      return;
    }
    
    AuthUtils.fetch('/api/reset', { method: 'POST' })
      .then(() => {
        alert('WiFi settings reset. Device will restart in configuration mode.');
      })
      .catch(() => {
        // Ignore errors as device may restart quickly
      });
  }

  async connectToNetwork(event) {
    event.preventDefault();
    
    const ssidInput = document.getElementById('ssid');
    const passwordInput = document.getElementById('password');
    const ssid = ssidInput ? ssidInput.value.trim() : '';
    
    if (!ssid) {
      alert('Please enter a network name');
      return;
    }
    
    const submitBtn = event.target.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, 'Connecting...');
    
    try {
      const formData = new FormData();
      formData.append('ssid', ssid);
      formData.append('password', passwordInput ? passwordInput.value : '');
      
      const response = await AuthUtils.fetch('/api/connect', {
        method: 'POST',
        body: formData
      });
      
      const data = await response.json();
      
      if (data.status === 'restarting') {
        alert('Connecting to network: ' + ssid + '. Device will restart.');
      } else {
        alert('Error: ' + data.message);
      }
    } catch (error) {
      console.error('Connection error:', error);
      // Device might restart quickly, so this error is expected
    } finally {
      UIUtils.updateButtonState(submitBtn, false, 'Connect');
    }
  }
}

// Initialize when page loads
document.addEventListener('DOMContentLoaded', function() {
  window.wifiManager = new WiFiManager();
});
)rawliteral";

#endif // WIFI_MANAGEMENT_JS_H