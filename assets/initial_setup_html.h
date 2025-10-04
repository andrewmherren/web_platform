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
  <link rel="stylesheet" href="/assets/style.css">
  <link rel="stylesheet" href="/assets/web-platform-style.css">
  <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
  <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head>
<body class="auth-body">
  <div class="auth-container">
    <div class="auth-card">
      <div class="auth-header">
        <h1 class="auth-title">Initial Setup</h1>
        <p class="auth-subtitle">Welcome to {{DEVICE_NAME}}! Create your admin account to get started.</p>
      </div>
      
      <form id="setupForm" class="auth-form" method="POST" action="/setup">
        <input type="hidden" name="_csrf" value="{{csrfToken}}">
        
        <div class="form-group">
          <label for="username" class="form-label">Username</label>
          <input type="text" id="username" name="username" class="form-control" required 
                 minlength="3" placeholder="Choose a username">
          <div class="form-help">Username must be at least 3 characters long.</div>
        </div>
        
        <div class="form-group">
          <label for="password" class="form-label">Password</label>
          <div class="password-field">
            <input type="password" id="password" name="password" class="form-control" required 
                   minlength="6" placeholder="Enter a secure password">
            <button type="button" class="password-toggle" onclick="togglePassword('password')" title="Show password">
              Show
            </button>
          </div>
          <div class="form-help">Password must be at least 6 characters long.</div>
        </div>
        
        <div class="form-group">
          <label for="confirmPassword" class="form-label">Confirm Password</label>
          <div class="password-field">
            <input type="password" id="confirmPassword" name="confirmPassword" class="form-control" 
                   required minlength="6" placeholder="Confirm your password">
            <button type="button" class="password-toggle" onclick="togglePassword('confirmPassword')" title="Show password">
              Show
            </button>
          </div>
        </div>
        
        <button type="submit" class="btn btn-primary auth-submit">
          Create Account & Continue
        </button>
      </form>
      
      <div class="auth-footer">
        <p class="auth-note">
          This will be your admin account for managing the device.
          Choose a secure username and password.
        </p>
      </div>
    </div>
  </div>

  <script src="/assets/web-platform-utils.js"></script>
  <script>
    // Password toggle functionality
    function togglePassword(fieldId) {
      const passwordField = document.getElementById(fieldId);
      const toggleButton = passwordField.nextElementSibling;
      
      if (passwordField.type === 'password') {
        passwordField.type = 'text';
        toggleButton.textContent = 'Hide';
        toggleButton.title = 'Hide password';
      } else {
        passwordField.type = 'password';
        toggleButton.textContent = 'Show';
        toggleButton.title = 'Show password';
      }
    }
    
    document.getElementById('setupForm').addEventListener('submit', function(e) {
      e.preventDefault();
      
      const username = document.getElementById('username').value;
      const password = document.getElementById('password').value;
      const confirmPassword = document.getElementById('confirmPassword').value;
      
      if (username.length < 3) {
        UIUtils.showAlert('Username must be at least 3 characters long', 'error');
        return;
      }
      
      if (password !== confirmPassword) {
        UIUtils.showAlert('Passwords do not match', 'error');
        return;
      }
      
      if (password.length < 6) {
        UIUtils.showAlert('Password must be at least 6 characters long', 'error');
        return;
      }
      
      // Submit the form using AuthUtils.fetch for automatic CSRF handling
      const formData = new FormData(this);
      
      AuthUtils.fetch('/api/user', {
        method: 'POST',
        body: formData
      }).then(response => {
        if (response.ok) {
          UIUtils.showAlert('Account created successfully! Redirecting...', 'success');
          setTimeout(() => {
            window.location.href = '/portal';
          }, 1500);
        } else {
          return response.text().then(text => {
            throw new Error(text || 'Failed to create account');
          });
        }
      }).catch(error => {
        console.error('Setup error:', error);
        UIUtils.showAlert(error.message || 'Account creation failed. Please try again.', 'error');
      });
    });
  </script>
</body>
</html>
)rawliteral";

#endif // INITIAL_SETUP_HTML_H