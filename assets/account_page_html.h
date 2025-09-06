#ifndef ACCOUNT_PAGE_HTML_H
#define ACCOUNT_PAGE_HTML_H

#include <Arduino.h>

const char ACCOUNT_PAGE_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - Account Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="csrf-token" content="{{csrfToken}}">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
    <link rel="icon" href="/assets/favicon.ico" sizes="any">
    <script src="/assets/web-platform-utils.js"></script>
    <script src="/assets/account-page.js"></script>
</head>
<body>
    <div class="container">
        {{NAV_MENU}}
        <h1>Account Settings</h1>
        
        <div id="statusMessage" class="alert" style="display: none;"></div>
        
        <div class="card">
            <h3>Update Password</h3>
            <form id="updatePasswordForm">
                <div class="form-group">
                    <label for="password">New Password:</label>
                    <input type="password" id="password" name="password" class="form-control" required minlength="4">
                </div>
                <div class="form-group">
                    <label for="confirmPassword">Confirm Password:</label>
                    <input type="password" id="confirmPassword" name="confirmPassword" class="form-control" required minlength="4">
                </div>
                <div class="button-group">
                    <button type="submit" class="btn btn-primary">Update Password</button>
                </div>
            </form>
        </div>
        
        <div class="card">
            <h3>API Tokens</h3>
            <p>Create API tokens to access this device's API from other applications.</p>
            
            <form id="createTokenForm">
                <div class="form-group">
                    <label for="tokenName">Token Name:</label>
                    <input type="text" id="tokenName" name="tokenName" class="form-control" required 
                           placeholder="e.g. 'Home Assistant Integration'">
                </div>
                <div class="button-group">
                    <button type="submit" class="btn btn-primary">Create Token</button>
                </div>
            </form>
            
            
            
            <h4 class="mt-3">Your Tokens</h4>
            <div id="tokenContainer">
                <p>Loading tokens...</p>
            </div>
        </div>
        
        <div class="button-group mt-3">
            <a href="/" class="btn btn-secondary">Back to Home</a>
            <a href="/logout" class="btn btn-danger">Logout</a>
        </div>
    </div>
</body>
</html>
)HTML";

#endif // ACCOUNT_PAGE_HTML_H