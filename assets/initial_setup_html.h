#ifndef INITIAL_SETUP_HTML_H
#define INITIAL_SETUP_HTML_H

#include <Arduino.h>

const char INITIAL_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Initial Setup - {{DEVICE_NAME}}</title>
  <link rel="stylesheet" href="/assets/web-platform-style.css">
  <link rel="icon" type="image/svg+xml" href="/assets/favicon.svg">
</head>
<body class="auth-body">
  <div class="auth-container">
    <div class="auth-card">
      <div class="auth-header">
        <h1 class="auth-title">Initial Setup</h1>
        <p class="auth-subtitle">Welcome to {{DEVICE_NAME}}! Please set your admin password.</p>
      </div>
      
      <form id="setupForm" class="auth-form" method="POST" action="/setup">
        <input type="hidden" name="_csrf" value="{{csrfToken}}">
        
        <div class="form-group">
          <label for="password" class="form-label">Admin Password</label>
          <input type="password" id="password" name="password" class="form-control" required 
                 minlength="6" placeholder="Enter a secure password">
          <div class="form-help">Password must be at least 6 characters long.</div>
        </div>
        
        <div class="form-group">
          <label for="confirmPassword" class="form-label">Confirm Password</label>
          <input type="password" id="confirmPassword" name="confirmPassword" class="form-control" 
                 required minlength="6" placeholder="Confirm your password">
        </div>
        
        <button type="submit" class="btn btn-primary auth-submit">
          Set Password & Continue
        </button>
      </form>
      
      <div class="auth-footer">
        <p class="auth-note">
          This password will be used to access the device management interface.
          Choose a strong, unique password for security.
        </p>
      </div>
    </div>
  </div>

  <script src="/assets/web-platform-utils.js"></script>
  <script>
    document.getElementById('setupForm').addEventListener('submit', function(e) {
      e.preventDefault();
      
      const password = document.getElementById('password').value;
      const confirmPassword = document.getElementById('confirmPassword').value;
      
      if (password !== confirmPassword) {
        UIUtils.showAlert('Passwords do not match', 'error');
        return;
      }
      
      if (password.length < 6) {
        UIUtils.showAlert('Password must be at least 6 characters long', 'error');
        return;
      }
      
      // Submit the form using fetch with proper error handling
      const formData = new FormData(this);
      
      fetch('/setup', {
        method: 'POST',
        body: formData,
        headers: {
          'X-CSRF-Token': document.querySelector('input[name="_csrf"]').value
        }
      }).then(response => {
        if (response.ok) {
          UIUtils.showAlert('Password set successfully! Redirecting...', 'success');
          setTimeout(() => {
            window.location.href = '/portal';
          }, 1500);
        } else {
          return response.text().then(text => {
            throw new Error(text || 'Failed to set password');
          });
        }
      }).catch(error => {
        console.error('Setup error:', error);
        UIUtils.showAlert(error.message || 'Setup failed. Please try again.', 'error');
      });
    });
  </script>
</body>
</html>
)rawliteral";

#endif // INITIAL_SETUP_HTML_H