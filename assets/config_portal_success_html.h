#ifndef CONFIG_PORTAL_SUCCESS_HTML_H
#define CONFIG_PORTAL_SUCCESS_HTML_H

#include <Arduino.h>

// Success page after saving credentials
const char CONFIG_PORTAL_SUCCESS_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>{{DEVICE_NAME}} - Configuration Saved</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="csrf-token" content="{{csrfToken}}">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <link rel="stylesheet" href="/assets/web-platform-style.css">
    <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
  <link rel="icon" href="/assets/favicon.ico" sizes="any">
</head>
<body>
    <div class="container">
        <div class="status-card success-container">
            <div class="success-icon">✅</div>
            <h1>WiFi Configuration Saved!</h1>
            <p>Connecting to network: <strong>{{NETWORK_SSID}}</strong></p>
            
            <div class="countdown" id="countdown">3</div>
            <div class="progress-bar">
                <div class="progress-fill" id="progress"></div>
            </div>
            
            <p>Device will restart automatically to establish connection.</p>
            <p><small>You may need to reconnect to the new network to access the device.</small></p>
            <p><small>After restart, you can access the device at: <strong>http://{{DEVICE_NAME}}.local</strong></small></p>
        </div>
    </div>
    
    <script src="/assets/web-platform-utils.js"></script>
    <script>
        // Success page countdown functionality
        let countdown = 3;
        const countdownEl = document.getElementById('countdown');
        const progressEl = document.getElementById('progress');
        
        function updateCountdown() {
            countdownEl.textContent = countdown;
            progressEl.style.width = ((3 - countdown) / 3 * 100) + '%';
            
            if (countdown <= 0) {
                countdownEl.textContent = 'Restarting...';
                progressEl.style.width = '100%';
                
                // Trigger restart using web-platform utilities
                AuthUtils.fetch('/api/reset', { method: 'POST' })
                    .catch(() => {}); // Ignore errors as device is restarting
                return;
            }
            
            countdown--;
            setTimeout(updateCountdown, 1000);
        }
        
        // Start countdown when page loads
        document.addEventListener('DOMContentLoaded', function() {
            setTimeout(updateCountdown, 100);
        });
    </script>
</body>
</html>
)HTML";

#endif // CONFIG_PORTAL_SUCCESS_HTML_H