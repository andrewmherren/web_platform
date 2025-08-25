#ifndef CONFIG_PORTAL_CSS_H
#define CONFIG_PORTAL_CSS_H

#include <Arduino.h>

// Enhanced CSS specifically for the WiFi configuration portal
const char CONFIG_PORTAL_CSS[] PROGMEM = R"(
/* WiFi Configuration Portal Styles */

/* Main configuration container */
.wifi-setup {
    max-width: 500px;
    margin: 20px auto;
}

/* Network scanner section */
.network-scanner {
    margin-bottom: 20px;
}

.scan-button {
    width: 100%;
    margin-bottom: 15px;
    background: linear-gradient(45deg, #4CAF50, #45a049);
    border: none;
    border-radius: 8px;
    padding: 12px;
    color: white;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.3s ease;
}

.scan-button:hover:not(:disabled) {
    background: linear-gradient(45deg, #45a049, #3d8b40);
    transform: translateY(-2px);
}

.scan-button:disabled {
    opacity: 0.7;
    cursor: not-allowed;
    transform: none;
}

/* Network list styling */
.network-list {
    max-height: 300px;
    overflow-y: auto;
    border: 1px solid rgba(255,255,255,0.1);
    border-radius: 8px;
    background: rgba(0,0,0,0.2);
}

.network-item {
    cursor: pointer;
    padding: 12px 16px;
    margin: 0;
    border-bottom: 1px solid rgba(255,255,255,0.05);
    transition: all 0.3s ease;
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.network-item:last-child {
    border-bottom: none;
}

.network-item:hover {
    background: rgba(255,255,255,0.1);
    transform: translateX(4px);
}

.network-item.selected {
    background: rgba(76, 175, 80, 0.2);
    border-left: 4px solid #4CAF50;
}

.network-name {
    font-weight: bold;
    font-size: 1em;
    color: #ffffff;
}

.network-info {
    display: flex;
    align-items: center;
    gap: 8px;
    color: rgba(255,255,255,0.7);
    font-size: 0.9em;
}

.security-icon {
    font-size: 1.2em;
}

.signal-strength {
    font-size: 0.8em;
    white-space: nowrap;
}

/* Loading and message states */
.loading {
    text-align: center;
    color: rgba(255,255,255,0.7);
    font-style: italic;
    padding: 20px;
}

.error {
    text-align: center;
    color: #f44336;
    font-style: italic;
    padding: 20px;
}

/* Security notice styling */
.security-notice {
    background: rgba(76, 175, 80, 0.1);
    border: 1px solid rgba(76, 175, 80, 0.3);
    border-radius: 8px;
    padding: 16px;
    margin-bottom: 20px;
}

.security-notice.https {
    background: rgba(33, 150, 243, 0.1);
    border-color: rgba(33, 150, 243, 0.3);
}

.security-notice h4 {
    color: #4CAF50;
    margin: 0 0 8px 0;
    font-size: 1.1em;
}

.security-notice.https h4 {
    color: #2196F3;
}

.security-notice p {
    margin: 0;
    font-size: 0.9em;
    line-height: 1.4;
}

.security-icon-large {
    display: inline-block;
    margin-right: 8px;
    font-size: 1.3em;
    vertical-align: middle;
}

/* Form enhancements */
.wifi-form .form-group {
    margin-bottom: 16px;
}

.wifi-form label {
    font-weight: bold;
    margin-bottom: 6px;
    display: block;
}

.wifi-form input[type="text"],
.wifi-form input[type="password"] {
    width: 100%;
    padding: 12px;
    margin: 0;
    border: 1px solid rgba(255,255,255,0.2);
    border-radius: 6px;
    background: rgba(255,255,255,0.1);
    color: white;
    font-size: 1em;
}

.wifi-form input[type="text"]:focus,
.wifi-form input[type="password"]:focus {
    outline: none;
    border-color: #4CAF50;
    box-shadow: 0 0 0 2px rgba(76, 175, 80, 0.2);
}

/* Button group styling */
.button-group {
    display: flex;
    gap: 10px;
    margin-top: 20px;
}

.button-group .btn {
    flex: 1;
}

/* Success page styles */
.success-container {
    max-width: 500px;
    margin: 50px auto;
    text-align: center;
}

.success-icon {
    font-size: 4em;
    color: #4CAF50;
    margin-bottom: 20px;
    animation: pulse 2s infinite;
}

.countdown {
    font-size: 2.5em;
    color: #4CAF50;
    font-weight: bold;
    margin: 20px 0;
    font-family: 'Courier New', monospace;
}

.progress-bar {
    width: 100%;
    height: 8px;
    background: rgba(255,255,255,0.1);
    border-radius: 4px;
    overflow: hidden;
    margin: 20px 0;
}

.progress-fill {
    height: 100%;
    background: linear-gradient(90deg, #4CAF50, #45a049);
    width: 0%;
    transition: width 0.3s ease;
    animation: shimmer 2s infinite;
}

/* Animations */
@keyframes pulse {
    0% { transform: scale(1); }
    50% { transform: scale(1.05); }
    100% { transform: scale(1); }
}

@keyframes shimmer {
    0% { background-position: -200px 0; }
    100% { background-position: 200px 0; }
}

/* Responsive adjustments */
@media (max-width: 600px) {
    .wifi-setup {
        margin: 10px;
    }
    
    .network-item {
        padding: 10px 12px;
        flex-direction: column;
        align-items: flex-start;
        gap: 4px;
    }
    
    .network-info {
        align-self: flex-end;
    }
    
    .button-group {
        flex-direction: column;
    }
    
    .countdown {
        font-size: 2em;
    }
}

/* Dark mode enhancements for config portal */
@media (prefers-color-scheme: dark) {
    .network-list {
        background: rgba(0,0,0,0.4);
    }
    
    .network-item:hover {
        background: rgba(255,255,255,0.05);
    }
    
    .wifi-form input[type="text"],
    .wifi-form input[type="password"] {
        background: rgba(255,255,255,0.05);
        border-color: rgba(255,255,255,0.1);
    }
}
)";

#endif // CONFIG_PORTAL_CSS_H