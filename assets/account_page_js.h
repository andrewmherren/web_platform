#ifndef ACCOUNT_PAGE_JS_H
#define ACCOUNT_PAGE_JS_H

#include <Arduino.h>

// Account page specific JavaScript
const char ACCOUNT_PAGE_JS[] PROGMEM = R"rawliteral(
// Account page functionality
class AccountManager {
  constructor() {
    this.init();
    this.userId = null;
    this.fetchCurrentUser();
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

  async fetchCurrentUser() {
    try {
      const response = await AuthUtils.fetchJSON('/api/user', {
        method: 'GET'
      });
      
      if (response.success && response.user) {
        this.userId = response.user.id;
      }
    } catch (error) {
      console.error('Error fetching user:', error);
    }
  }

  showTokenForm() {
    const tokenForm = document.getElementById('token-form');
    const showBtn = document.getElementById('show-token-form');
    
    if (tokenForm) tokenForm.style.display = 'block';
    if (showBtn) showBtn.style.display = 'none';
  }

  async deleteToken(token) {
    UIUtils.showConfirm(
      'Delete API Token',
      'Are you sure you want to delete this token? This action cannot be undone.',
      async () => {
        try {
          const response = await AuthUtils.fetch('/api/tokens/' + token, {
            method: 'DELETE'
          });

          const data = await response.json();
          
          if (data.success) {
            window.location.reload();
          } else {
            UIUtils.showAlert('Error', 'Failed to delete token: ' + data.message, 'error');
          }
        } catch (error) {
          console.error('Delete token error:', error);
          UIUtils.showAlert('Error', 'Failed to delete token due to network error.', 'error');
        }
      }
    );
  }

  async updatePassword(event) {
    event.preventDefault();
    
    const form = event.target;
    const password = form.password.value;
    const confirmPassword = form.confirmPassword.value;
    
    if (password !== confirmPassword) {
      UIUtils.showAlert('Password Mismatch', 'The passwords you entered do not match. Please try again.', 'warning');
      return;
    }
    
    const submitBtn = form.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, 'Updating...');
    
    try {
      const formData = new FormData();
      formData.append('password', password);

      const response = await AuthUtils.fetchJSON('/api/user', {
        method: 'PUT',
        body: JSON.stringify({ password: password })
      });

      if (response.success) {
        UIUtils.showAlert('Success', 'Password updated successfully!', 'success');
        form.reset();
      } else {
        UIUtils.showAlert('Update Failed', 'Error: ' + response.message, 'error');
      }
    } catch (error) {
      console.error('Password update error:', error);
      UIUtils.showAlert('Network Error', 'Failed to update password. Please check your connection and try again.', 'error');
    } finally {
      UIUtils.updateButtonState(submitBtn, false, 'Update Password');
    }
  } 

  async createToken(event) {
    event.preventDefault();
    
    if (!this.userId) {
      UIUtils.showAlert('Error', 'User ID not available. Please refresh the page.', 'error');
      return;
    }
    
    const form = event.target;
    const tokenName = form.tokenName.value;
    
    if (!tokenName.trim()) {
      UIUtils.showAlert('Missing Information', 'Please enter a token name to identify this API token.', 'warning');
      return;
    }
    
    const submitBtn = form.querySelector('button[type="submit"]');
    UIUtils.updateButtonState(submitBtn, true, 'Creating...');
    
    try {
      // Use RESTful endpoint
      const response = await AuthUtils.fetchJSON(`/api/users/${this.userId}/tokens`, {
        method: 'POST',
        body: JSON.stringify({ name: tokenName })
      });
      
      if (response.success) {
        this.displayNewToken(response.token);
        form.reset();
        form.style.display = 'none';
        const showBtn = document.getElementById('show-token-form');
        if (showBtn) showBtn.style.display = 'block';
      } else {
        UIUtils.showAlert('Creation Failed', 'Error: ' + response.message, 'error');
      }
    } catch (error) {
      console.error('Token creation error:', error);
      UIUtils.showAlert('Network Error', 'Failed to create token. Please check your connection and try again.', 'error');
    } finally {
      UIUtils.updateButtonState(submitBtn, false, 'Create Token');
    }
  }

  displayNewToken(token) {
    // Use the new modal system to display the token
    UIUtils.showTokenModal(token);
    
    // Hide the old display element if it exists
    const tokenDisplay = document.getElementById('token-display');
    if (tokenDisplay) {
      tokenDisplay.style.display = 'none';
      tokenDisplay.textContent = '';
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