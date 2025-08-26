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
    <meta charset="UTF-8">
    <link rel="stylesheet" href="/assets/style.css">
    <style>
        .success-container {
            max-width: 500px;
            margin: 50px auto;
            text-align: center;
        }
        .success-icon {
            font-size: 4em;
            color: #4CAF50;
            margin-bottom: 20px;
        }
        .countdown {
            font-size: 2em;
            color: #4CAF50;
            font-weight: bold;
            margin: 20px 0;
        }
        .progress-bar {
            width: 100%;
            height: 6px;
            background: rgba(255,255,255,0.1);
            border-radius: 3px;
            overflow: hidden;
            margin: 20px 0;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #4CAF50, #45a049);
            width: 0%;
            transition: width 0.1s ease;
        }
    </style>
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
    
    <script>
        var countdown = 3;
        var countdownEl = document.getElementById('countdown');
        var progressEl = document.getElementById('progress');
        
        function updateCountdown() {
            countdownEl.textContent = countdown;
            progressEl.style.width = ((3 - countdown) / 3 * 100) + '%';
            
            if (countdown <= 0) {
                countdownEl.textContent = 'Restarting...';
                progressEl.style.width = '100%';
                
                // Trigger restart
                fetch('/api/reset', { method: 'POST' })
                    .catch(function() {}); // Ignore errors as device is restarting
                return;
            }
            
            countdown--;
            setTimeout(updateCountdown, 1000);
        }
        
        // Start countdown
        setTimeout(updateCountdown, 100);
    </script>
</body>
</html>
)HTML";

#endif // CONFIG_PORTAL_SUCCESS_HTML_H