#ifndef WEB_PLATFORM_UTILS_JS_H
#define WEB_PLATFORM_UTILS_JS_H

#include <Arduino.h>

// Common JavaScript utilities shared across all pages
const char WEB_PLATFORM_UTILS_JS[] PROGMEM = R"rawliteral(
// TickerTape Shared JavaScript Utilities

// Authentication utilities
const AuthUtils = {
  getCsrfToken() {
    const meta = document.querySelector('meta[name="csrf-token"]');
    return meta ? meta.getAttribute('content') : null;
  },

  // Make authenticated fetch request with CSRF token
  async fetch(url, options = {}) {
    const csrfToken = this.getCsrfToken();
    const headers = {
      'Content-Type': 'application/x-www-form-urlencoded',
      ...options.headers
    };
    
    if (csrfToken) {
      headers['X-CSRF-Token'] = csrfToken;
    }

    return fetch(url, {
      ...options,
      headers,
      credentials: 'same-origin'
    });
  },

  // Make authenticated JSON fetch request
  async fetchJSON(url, options = {}) {
    const response = await this.fetch(url, options);
    return response.json();
  }
};

// Network scanning utilities
const NetworkUtils = {
  async scanNetworks() {
    try {
      const response = await AuthUtils.fetchJSON('/api/scan');
      return response;
    } catch (error) {
      console.error('Network scan failed:', error);
      throw error;
    }
  },

  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  },

  getSignalStrengthIcon(rssi) {
    if (rssi >= -50) return '📶';
    if (rssi >= -60) return '📶';
    if (rssi >= -70) return '📶';
    return '📶';
  },

  getSignalStrengthText(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  },

  renderNetworkItem(network, clickHandler) {
    const div = document.createElement('div');
    div.className = 'network-item';
    
    const nameDiv = document.createElement('div');
    nameDiv.className = 'network-name';
    nameDiv.textContent = network.ssid;
    
    const infoDiv = document.createElement('div');
    infoDiv.className = 'network-info';
    
    const securityIcon = document.createElement('span');
    securityIcon.className = 'security-icon';
    securityIcon.innerHTML = network.encryption ? '🔒' : '🔓';
    securityIcon.title = network.encryption ? 'Secured' : 'Open';
    
    const signalSpan = document.createElement('span');
    signalSpan.className = 'signal-strength';
    signalSpan.textContent = `${network.rssi}dBm`;
    signalSpan.title = this.getSignalStrengthText(network.rssi);
    
    infoDiv.appendChild(securityIcon);
    infoDiv.appendChild(signalSpan);
    
    div.appendChild(nameDiv);
    div.appendChild(infoDiv);
    
    if (clickHandler) {
      div.style.cursor = 'pointer';
      div.onclick = () => clickHandler(network.ssid, div);
    }
    
    return div;
  }
};

// Form utilities
const FormUtils = {
  // Handle form submission with CSRF
  async submitForm(form, endpoint, successCallback, errorCallback) {
    const formData = new FormData(form);
    const csrfToken = AuthUtils.getCsrfToken();
    
    if (csrfToken) {
      formData.append('_csrf', csrfToken);
    }

    try {
      const response = await AuthUtils.fetch(endpoint, {
        method: 'POST',
        body: formData
      });
      
      const result = await response.json();
      
      if (result.success) {
        if (successCallback) successCallback(result);
      } else {
        if (errorCallback) errorCallback(result.message || 'Unknown error');
      }
    } catch (error) {
      console.error('Form submission error:', error);
      if (errorCallback) errorCallback('Network error');
    }
  },

  // Clear form fields
  clearForm(form) {
    const inputs = form.querySelectorAll('input, select, textarea');
    inputs.forEach(input => {
      if (input.type === 'checkbox' || input.type === 'radio') {
        input.checked = false;
      } else {
        input.value = '';
      }
    });
  }
};

// UI utilities
const UIUtils = {
  showLoading(element, message = 'Loading...') {
    if (element) {
      element.innerHTML = `<div class="loading">${message}</div>`;
    }
  },

  showError(element, message = 'An error occurred') {
    if (element) {
      element.innerHTML = `<div class="error">${message}</div>`;
    }
  },

  updateButtonState(button, loading, text) {
    if (!button) return;
    
    if (loading) {
      button.disabled = true;
      button.dataset.originalText = button.textContent;
      button.textContent = text || 'Loading...';
    } else {
      button.disabled = false;
      button.textContent = button.dataset.originalText || text || 'Submit';
    }
  },

  selectNetworkItem(selectedElement) {
    // Clear all selections
    document.querySelectorAll('.network-item').forEach(item => {
      item.classList.remove('selected');
    });
    
    // Mark new selection
    if (selectedElement) {
      selectedElement.classList.add('selected');
    }
  }
};

// Common initialization
document.addEventListener('DOMContentLoaded', function() {
  // Add CSRF token to all forms that don't already have it
  const forms = document.querySelectorAll('form');
  const csrfToken = AuthUtils.getCsrfToken();
  
  if (csrfToken) {
    forms.forEach(form => {
      const existingCsrf = form.querySelector('input[name="_csrf"]');
      if (!existingCsrf) {
        const csrfInput = document.createElement('input');
        csrfInput.type = 'hidden';
        csrfInput.name = '_csrf';
        csrfInput.value = csrfToken;
        form.appendChild(csrfInput);
      }
    });
  }
});

// Global utilities for backward compatibility
window.escapeHtml = NetworkUtils.escapeHtml;
window.AuthUtils = AuthUtils;
window.NetworkUtils = NetworkUtils;
window.FormUtils = FormUtils;
window.UIUtils = UIUtils;
)rawliteral";

#endif // WEB_PLATFORM_UTILS_JS_H