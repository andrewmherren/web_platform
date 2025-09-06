#ifndef CONFIG_PORTAL_SUCCESS_JS_H
#define CONFIG_PORTAL_SUCCESS_JS_H

#include <Arduino.h>

// Success page after saving credentials
const char CONFIG_PORTAL_SUCCESS_JS[] PROGMEM = R"rawliteral(

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
)rawliteral";

#endif // CONFIG_PORTAL_SUCCESS_HTML_H