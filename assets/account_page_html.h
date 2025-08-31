#ifndef ACCOUNT_PAGE_HTML_H
#define ACCOUNT_PAGE_HTML_H

#include <Arduino.h>

const char ACCOUNT_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Account - {{DEVICE_NAME}}</title>
  <link rel="stylesheet" href="/assets/styles.css">
  <link rel="stylesheet" href="/assets/web-platform-styles.css">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="csrf-token" content="{{csrfToken}}">
</head>
<body>
  <div class="container">
    <h1>Account Settings</h1>
    <p>Logged in as: <strong>{{username}}</strong> | <a href="/logout">Logout</a></p>
    
    <div class="card">
      <h2>Change Password</h2>
      <form id="password-form">
        <div class="form-group">
          <label for="password">New Password:</label>
          <input type="password" id="password" name="password" class="form-control" required>
        </div>
        <div class="form-group">
          <label for="confirmPassword">Confirm Password:</label>
          <input type="password" id="confirmPassword" name="confirmPassword" class="form-control" required>
        </div>
        <button type="submit" class="btn btn-primary">Update Password</button>
      </form>
    </div>
    
    <div class="card">
      <h2>API Tokens</h2>
      <p>API tokens can be used to authenticate API requests without logging in.</p>
      {{tokensHtml}}
      <div id="token-display" class="token-display"></div>
      <button id="show-token-form" class="btn btn-secondary">Create New Token</button>
      <form id="token-form" style="display: none;">
        <div class="form-group">
          <label for="tokenName">Token Name:</label>
          <input type="text" id="tokenName" name="tokenName" class="form-control" required>
        </div>
        <button type="submit" class="btn btn-primary">Create Token</button>
      </form>
    </div>
  </div>
  
  <script src="/assets/web-platform-utils.js"></script>
  <script src="/assets/account-page.js"></script>
</body>
</html>
)rawliteral";

#endif // ACCOUNT_PAGE_HTML_H