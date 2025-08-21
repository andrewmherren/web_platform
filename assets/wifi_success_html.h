#ifndef WIFI_SUCCESS_HTML_H
#define WIFI_SUCCESS_HTML_H

#include <Arduino.h>

const char WIFI_SUCCESS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <title>Configuration Saved - Ticker</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <link rel="stylesheet" href="/assets/style.css" type="text/css">
  <!-- Optional app-specific theme CSS is loaded after default styles -->
  <style>
    /* Additional styles for success page */
    body {
      text-align: center;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }
    
    .container {
      max-width: 500px;
      margin: 0 auto;
      background: rgba(255, 255, 255, 0.1);
      padding: 40px 30px;
      border-radius: 15px;
      backdrop-filter: blur(10px);
      -webkit-backdrop-filter: blur(10px);
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
      border: 1px solid rgba(255, 255, 255, 0.2);
    }
    
    .success-icon {
      width: 100px;
      height: 100px;
      margin: 0 auto 30px;
      border-radius: 50%;
      background: rgba(76, 175, 80, 0.3);
      border: 3px solid #4CAF50;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #4CAF50;
      font-size: 3em;
    }
    
    h1 {
      color: #ffffff;
      margin-bottom: 25px;
      font-size: 2.2rem;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
    }
    
    p {
      margin-bottom: 20px;
      color: rgba(255, 255, 255, 0.9);
      font-size: 1.1rem;
    }
    
    .countdown {
      background: rgba(76, 175, 80, 0.2);
      border: 1px solid rgba(76, 175, 80, 0.5);
      border-radius: 10px;
      padding: 20px;
      margin: 25px 0;
      font-size: 1.3rem;
      font-weight: bold;
      color: #4CAF50;
    }
    
    @media (max-width: 375px) {
      .container {
        padding: 30px 20px;
      }
      
      h1 {
        font-size: 1.8rem;
      }
      
      .success-icon {
        width: 80px;
        height: 80px;
        font-size: 2.5em;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="success-icon">
      ✓
    </div>
    <h1>Configuration Saved!</h1>
    <p>WiFi credentials have been saved successfully. The device will now restart and attempt to connect to your WiFi network.</p>
    <p>If the connection fails, the configuration portal will become available again.</p>
    <div class="countdown" id="countdown">
      Restarting in <span id="timer">30</span> seconds...
    </div>
  </div>
  
  <script>
    let seconds = 30;
    const timerElement = document.getElementById('timer');
    
    const countdown = setInterval(() => {
      seconds--;
      timerElement.textContent = seconds;
      
      if (seconds <= 0) {
        clearInterval(countdown);
        timerElement.parentElement.innerHTML = "Device is restarting...";
      }
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

#endif // WIFI_SUCCESS_HTML_H