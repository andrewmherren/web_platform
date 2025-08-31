#ifndef ACCOUNT_PAGE_JS_H
#define ACCOUNT_PAGE_JS_H

#include <Arduino.h>

// Account page specific JavaScript
const char ACCOUNT_PAGE_JS[] PROGMEM = R"rawliteral(
// Account page functionality
class AccountManager {
  constructor() {
    this.init();
  }

  init() {
    this.bindEvents();
  }

  bindEvents() {
    // Password form
    const passwordForm = document.getElementById('password-form');
    if (passwordForm) {
      passwordForm.onsubmit = (e) => this.updatePassword(e);
    }

    // Token form
    const tokenForm = document.getElementById('token-form');
    if (tokenForm) {
      tokenForm.onsubmit = (e) => this.createToken(e);
    }

    // Show token form button
    const showTokenBtn = document.getElementById('show-token-form');
    if (showTokenBtn) {
      showTokenBtn.onclick = () => this.showTokenForm();
    }
  }

  showTokenForm() {
    const tokenForm = document.getElementById('token-form');
    const showBtn = document.getElementById('show-token-form');
    
    if (tokenForm) tokenForm.style.display = 'block';
    if (showBtn) showBtn.style.display = 'none';
  }

  async deleteToken(token) {
    if (!confirm('Are you sure you want to delete this token?')) {
      return;
    }

    try {
      const formData = new FormData();
      formData.append('token', token);
      
      const response = await AuthUtils.fetch('/api/tokens/delete', {
        method: 'POST',
        body: formData
      });

      const data = await response.json();
      
      if (data.success) {
        window.location.reload();
      } else {
        alert('Error: ' + data.message);
      }
    } catch (error) {
      console.error('Delete token error:', error);
      alert('Failed to delete token');
    }
  }

  async updatePassword(event) {
    event.preventDefault();
    
    const form = event.target;
    const password = form.password.value;
    const confirmPassword = form.confirmPassword.value;
    
    if (password !== confirmPassword) {
      alert('Passwords do not match');
      return;
    }
    
    const submitBtn = form.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, 'Updating...');
    
    try {
      const formData = new FormData();
      formData.append('password', password);
      
      const response = await AuthUtils.fetch('/api/account/password', {
        method: 'POST',
        body: formData
      });

      const data = await response.json();
      
      if (data.success) {
        alert('Password updated successfully');
        form.reset();
      } else {
        alert('Error: ' + data.message);
      }
    } catch (error) {
      console.error('Password update error:', error);
      alert('Failed to update password');
    } finally {
      UIUtils.updateButtonState(submitBtn, false, 'Update Password');
    }
  }

  async createToken(event) {
    event.preventDefault();
    
    const form = event.target;
    const tokenName = form.tokenName.value;
    
    if (!tokenName.trim()) {
      alert('Please enter a token name');
      return;
    }
    
    const submitBtn = form.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, 'Creating...');
    
    try {
      const formData = new FormData();
      formData.append('name', tokenName);
      
      const response = await AuthUtils.fetch('/api/tokens/create', {
        method: 'POST',
        body: formData
      });

      const data = await response.json();
      
      if (data.success) {
        this.displayNewToken(data.token);
        form.reset();
      } else {
        alert('Error: ' + data.message);
      }
    } catch (error) {
      console.error('Token creation error:', error);
      alert('Failed to create token');
    } finally {
      UIUtils.updateButtonState(submitBtn, false, 'Create Token');
    }
  }

  displayNewToken(token) {
    const tokenDisplay = document.getElementById('token-display');
    if (tokenDisplay) {
      tokenDisplay.textContent = token;
      tokenDisplay.style.display = 'block';
      
      // Auto-hide after 30 seconds for security
      setTimeout(() => {
        tokenDisplay.style.display = 'none';
        tokenDisplay.textContent = '';
      }, 30000);
    }
  }
}

// Global function for delete buttons (called from HTML)
window.deleteToken = function(token) {
  if (window.accountManager) {
    window.accountManager.deleteToken(token);
  }
};

// Initialize when page loads
document.addEventListener('DOMContentLoaded', function() {
  window.accountManager = new AccountManager();
});
)rawliteral";

#endif // ACCOUNT_PAGE_JS_H