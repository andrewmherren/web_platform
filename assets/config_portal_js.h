#ifndef CONFIG_PORTAL_JS_H
#define CONFIG_PORTAL_JS_H

#include <Arduino.h>

// Enhanced JavaScript for WiFi configuration portal
const char CONFIG_PORTAL_JS[] PROGMEM = R"(
// WiFi Configuration Portal JavaScript
class WiFiSetup {
    constructor() {
        this.selectedNetwork = null;
        this.isScanning = false;
        this.networks = [];
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
        
        if (scanBtn) scanBtn.addEventListener('click', () => this.scanNetworks());
        if (clearBtn) clearBtn.addEventListener('click', () => this.clearForm());
        if (form) form.addEventListener('submit', (e) => this.handleSubmit(e));
    }
    
    async scanNetworks() {
        if (this.isScanning) return;
        
        this.isScanning = true;
        this.updateScanButton('⟳ Scanning...', true);
        this.showLoading('Scanning for networks...');
        
        try {
            const response = await fetch('/api/scan');
            const data = await response.json();
            
            if (data.networks && data.networks.length > 0) {
                this.networks = data.networks;
                this.displayNetworks();
            } else {
                this.showMessage('No networks found. Try scanning again.', 'info');
            }
        } catch (error) {
            console.error('Scan error:', error);
            this.showMessage('Scan failed. Please try again.', 'error');
        } finally {
            this.isScanning = false;
            this.updateScanButton('🔍 Scan for Networks', false);
        }
    }
    
    displayNetworks() {
        const networkList = document.getElementById('network-list');
        if (!networkList) return;
        
        networkList.innerHTML = '';
        
        this.networks.forEach(network => {
            const div = document.createElement('div');
            div.className = 'network-item';
            div.onclick = () => this.selectNetwork(network.ssid, div);
            
            const signalStrength = this.getSignalStrengthDescription(network.rssi);
            const securityIcon = network.encryption ? '🔒' : '🔓';
            
            div.innerHTML = `
                <div class="network-name">${this.escapeHtml(network.ssid)}</div>
                <div class="network-info">
                    <span class="security-icon" title="${network.encryption ? 'Secured' : 'Open'}">${securityIcon}</span>
                    <span class="signal-strength" title="${network.rssi}dBm">${signalStrength}</span>
                </div>
            `;
            
            networkList.appendChild(div);
        });
    }
    
    selectNetwork(ssid, element) {
        // Remove previous selection
        document.querySelectorAll('.network-item').forEach(item => {
            item.classList.remove('selected');
        });
        
        // Mark current selection
        element.classList.add('selected');
        this.selectedNetwork = ssid;
        
        // Fill form
        const ssidInput = document.getElementById('ssid');
        const passwordInput = document.getElementById('password');
        
        if (ssidInput) {
            ssidInput.value = ssid;
        }
        if (passwordInput) {
            passwordInput.focus();
        }
    }
    
    clearForm() {
        const ssidInput = document.getElementById('ssid');
        const passwordInput = document.getElementById('password');
        
        if (ssidInput) ssidInput.value = '';
        if (passwordInput) passwordInput.value = '';
        
        this.selectedNetwork = null;
        
        document.querySelectorAll('.network-item').forEach(item => {
            item.classList.remove('selected');
        });
    }
    
    handleSubmit(event) {
        const ssid = document.getElementById('ssid')?.value;
        if (!ssid || ssid.trim() === '') {
            event.preventDefault();
            this.showMessage('Please select a network or enter a network name.', 'error');
            return false;
        }
        
        // Show loading state
        const submitBtn = event.target.querySelector('button[type="submit"]');
        if (submitBtn) {
            submitBtn.innerHTML = '⟳ Connecting...';
            submitBtn.disabled = true;
        }
        
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
        if (networkList) {
            networkList.innerHTML = `<div class="loading">${message}</div>`;
        }
    }
    
    showMessage(message, type = 'info') {
        const networkList = document.getElementById('network-list');
        if (networkList) {
            const className = type === 'error' ? 'error' : 'loading';
            networkList.innerHTML = `<div class="${className}">${message}</div>`;
        }
    }
    
    getSignalStrengthDescription(rssi) {
        if (rssi >= -50) return '📶 Excellent';
        if (rssi >= -60) return '📶 Good';
        if (rssi >= -70) return '📶 Fair';
        return '📶 Weak';
    }
    
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// Initialize WiFi setup when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    window.wifiSetup = new WiFiSetup();
});

// Additional utility functions
function refreshNetworks() {
    if (window.wifiSetup) {
        window.wifiSetup.scanNetworks();
    }
}

function selectNetworkByName(ssid) {
    if (window.wifiSetup) {
        const networkItems = document.querySelectorAll('.network-item');
        networkItems.forEach(item => {
            const nameEl = item.querySelector('.network-name');
            if (nameEl && nameEl.textContent.trim() === ssid) {
                window.wifiSetup.selectNetwork(ssid, item);
            }
        });
    }
}
)";

#endif // CONFIG_PORTAL_JS_H