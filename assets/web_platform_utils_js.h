#ifndef WEB_PLATFORM_UTILS_JS_H
#define WEB_PLATFORM_UTILS_JS_H

#include <Arduino.h>

// Common JavaScript utilities shared across all pages
const char WEB_PLATFORM_UTILS_JS[] PROGMEM = R"rawliteral(
// Web-platform Shared JavaScript Utilities

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
      ...options.headers
    };
    
    // Only set Content-Type for non-FormData requests
    if (options.body && !(options.body instanceof FormData)) {
      headers['Content-Type'] = 'application/x-www-form-urlencoded';
    }
    
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
    const csrfToken = this.getCsrfToken();
    const headers = {
      'Content-Type': 'application/json',
      ...options.headers
    };
    
    if (csrfToken) {
      headers['X-CSRF-Token'] = csrfToken;
    }

    const response = await fetch(url, {
      ...options,
      headers,
      credentials: 'same-origin'
    });
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
  },

  // Modal system
  createModal() {
    // Remove any existing modal
    this.destroyModal();
    
    const modalOverlay = document.createElement('div');
    modalOverlay.id = 'modal-overlay';
    modalOverlay.className = 'modal-overlay';
    
    const modalContainer = document.createElement('div');
    modalContainer.className = 'modal-container';
    
    const modalContent = document.createElement('div');
    modalContent.className = 'modal-content';
    
    const modalHeader = document.createElement('div');
    modalHeader.className = 'modal-header';
    
    const modalTitle = document.createElement('h3');
    modalTitle.className = 'modal-title';
    
    const modalClose = document.createElement('button');
    modalClose.className = 'modal-close';
    modalClose.innerHTML = '&times;';
    modalClose.onclick = () => this.destroyModal();
    
    const modalBody = document.createElement('div');
    modalBody.className = 'modal-body';
    
    const modalFooter = document.createElement('div');
    modalFooter.className = 'modal-footer';
    
    modalHeader.appendChild(modalTitle);
    modalHeader.appendChild(modalClose);
    modalContent.appendChild(modalHeader);
    modalContent.appendChild(modalBody);
    modalContent.appendChild(modalFooter);
    modalContainer.appendChild(modalContent);
    modalOverlay.appendChild(modalContainer);
    
    // Close on overlay click
    modalOverlay.onclick = (e) => {
      if (e.target === modalOverlay) {
        this.destroyModal();
      }
    };
    
    // Close on Escape key
    const escapeHandler = (e) => {
      if (e.key === 'Escape') {
        this.destroyModal();
        document.removeEventListener('keydown', escapeHandler);
      }
    };
    document.addEventListener('keydown', escapeHandler);
    
    document.body.appendChild(modalOverlay);
    
    return {
      overlay: modalOverlay,
      title: modalTitle,
      body: modalBody,
      footer: modalFooter,
      close: () => this.destroyModal()
    };
  },

  destroyModal() {
    const existingModal = document.getElementById('modal-overlay');
    if (existingModal) {
      existingModal.remove();
    }
  },

  showAlert(title, message, type = 'info') {
    const modal = this.createModal();
    modal.title.textContent = title;
    modal.body.innerHTML = `<div class="alert alert-${type}">${message}</div>`;
    
    const okButton = document.createElement('button');
    okButton.className = 'btn btn-primary';
    okButton.textContent = 'OK';
    okButton.onclick = () => modal.close();
    
    modal.footer.appendChild(okButton);
    
    // Auto-focus OK button
    setTimeout(() => okButton.focus(), 100);
    
    return modal;
  },

  showConfirm(title, message, onConfirm, onCancel) {
    const modal = this.createModal();
    modal.title.textContent = title;
    modal.body.innerHTML = `<div class="alert alert-warning">${message}</div>`;
    
    const cancelButton = document.createElement('button');
    cancelButton.className = 'btn btn-secondary';
    cancelButton.textContent = 'Cancel';
    cancelButton.onclick = () => {
      if (onCancel) onCancel();
      modal.close();
    };
    
    const confirmButton = document.createElement('button');
    confirmButton.className = 'btn btn-danger';
    confirmButton.textContent = 'Confirm';
    confirmButton.onclick = () => {
      if (onConfirm) onConfirm();
      modal.close();
    };
    
    modal.footer.appendChild(cancelButton);
    modal.footer.appendChild(confirmButton);
    
    // Auto-focus cancel button for safety
    setTimeout(() => cancelButton.focus(), 100);
    
    return modal;
  },

  showTokenModal(token) {
    const modal = this.createModal();
    modal.title.textContent = 'API Token Created';
    
    modal.body.innerHTML = `
      <div class="alert alert-success">
        <p><strong>Your new API token has been created successfully!</strong></p>
        <p class="token-warning">⚠️ <strong>Important:</strong> This token will only be shown once. Please copy and save it securely.</p>
      </div>
      <div class="token-container">
        <label for="new-token">API Token:</label>
        <div class="token-display-box">
          <input type="text" id="new-token" class="token-input" value="${token}" readonly>
          <button class="btn btn-copy" onclick="UIUtils.copyToken()">Copy</button>
        </div>
      </div>
    `;
    
    const closeButton = document.createElement('button');
    closeButton.className = 'btn btn-primary';
    closeButton.textContent = 'I\'ve Saved My Token';
    closeButton.onclick = () => modal.close();
    
    modal.footer.appendChild(closeButton);
    
    // Auto-select the token text
    setTimeout(() => {
      const tokenInput = document.getElementById('new-token');
      if (tokenInput) {
        tokenInput.select();
        tokenInput.focus();
      }
    }, 100);
    
    return modal;
  },

  copyToken() {
    const tokenInput = document.getElementById('new-token');
    if (tokenInput) {
      tokenInput.select();
      document.execCommand('copy');
      
      // Show feedback
      const copyBtn = document.querySelector('.btn-copy');
      if (copyBtn) {
        const originalText = copyBtn.textContent;
        copyBtn.textContent = 'Copied!';
        copyBtn.classList.add('btn-success');
        setTimeout(() => {
          copyBtn.textContent = originalText;
          copyBtn.classList.remove('btn-success');
        }, 2000);
      }
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

// Time and date utilities
const TimeUtils = {
  // Format a UTC timestamp to local time
  formatTimestamp(timestamp, options = {}) {
    try {
      if (!timestamp) {
        return 'Unknown date';
      }
      
      // Handle various timestamp formats
      let date;
      if (typeof timestamp === 'string') {
        // ISO string format (e.g., "2025-09-06T12:49:38Z")
        if (timestamp.includes('T') || timestamp.includes('-')) {
          date = new Date(timestamp);
        } else {
          // Unix timestamp as string
          const timestampNum = parseInt(timestamp);
          date = new Date(timestampNum * 1000); // Convert seconds to milliseconds
        }
      } else {
        // Numeric timestamp (assume Unix timestamp in seconds)
        date = new Date(timestamp * 1000);
      }
      
      // Validate the date
      if (isNaN(date.getTime())) {
        console.warn('Invalid timestamp:', timestamp);
        return 'Invalid date';
      }
      
      // Default formatting options
      const defaultOptions = {
        year: 'numeric',
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
        timeZoneName: 'short'
      };
      
      const formatOptions = { ...defaultOptions, ...options };
      return date.toLocaleString(undefined, formatOptions);
    } catch (error) {
      console.error('Error formatting timestamp:', error, 'for timestamp:', timestamp);
      return 'Date error';
    }
  },

  // Format timestamp as relative time (e.g., "2 hours ago")
  formatRelativeTime(timestamp) {
    try {
      if (!timestamp) {
        return 'Unknown time';
      }
      
      // Parse timestamp similar to formatTimestamp
      let date;
      if (typeof timestamp === 'string') {
        if (timestamp.includes('T') || timestamp.includes('-')) {
          date = new Date(timestamp);
        } else {
          const timestampNum = parseInt(timestamp);
          date = new Date(timestampNum * 1000);
        }
      } else {
        date = new Date(timestamp * 1000);
      }
      
      if (isNaN(date.getTime())) {
        console.warn('Invalid timestamp for relative time:', timestamp);
        return 'Invalid date';
      }
      
      const now = new Date();
      const diffMs = now.getTime() - date.getTime();
      const minutes = Math.floor(diffMs / 60000);
      
      if (minutes < 1) {
        return 'Just now';
      } else if (minutes < 60) {
        return minutes === 1 ? '1 minute ago' : `${minutes} minutes ago`;
      } else if (minutes < 1440) {
        const hours = Math.floor(minutes / 60);
        return hours === 1 ? '1 hour ago' : `${hours} hours ago`;
      } else {
        const days = Math.floor(minutes / 1440);
        if (days > 7) {
          // For older dates, show full date
          return this.formatTimestamp(timestamp, {
            year: 'numeric',
            month: 'short',
            day: 'numeric'
          });
        }
        return days === 1 ? '1 day ago' : `${days} days ago`;
      }
    } catch (error) {
      console.error('Error formatting relative time:', error, 'for timestamp:', timestamp);
      return 'Time error';
    }
  },

  // Get the user's timezone
  getTimezone() {
    return Intl.DateTimeFormat().resolvedOptions().timeZone;
  },

  // Format just the time portion (useful for "Current Time" displays)
  formatTime(timestamp, options = {}) {
    const defaultOptions = {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      timeZoneName: 'short'
    };
    return this.formatTimestamp(timestamp, { ...defaultOptions, ...options });
  },

  // Format just the date portion
  formatDate(timestamp, options = {}) {
    const defaultOptions = {
      year: 'numeric',
      month: 'short',
      day: 'numeric'
    };
    return this.formatTimestamp(timestamp, { ...defaultOptions, ...options });
  }
};

// Global utilities for backward compatibility
window.escapeHtml = NetworkUtils.escapeHtml;
window.AuthUtils = AuthUtils;
window.NetworkUtils = NetworkUtils;
window.FormUtils = FormUtils;
window.UIUtils = UIUtils;
window.TimeUtils = TimeUtils;
)rawliteral";

#endif // WEB_PLATFORM_UTILS_JS_H