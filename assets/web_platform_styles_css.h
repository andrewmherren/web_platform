#ifndef WEB_PLATFORM_STYLES_CSS_H
#define WEB_PLATFORM_STYLES_CSS_H

#include <Arduino.h>

// Additional CSS styles for specific components not in base theme
const char WEB_PLATFORM_STYLES_CSS[] PROGMEM = R"rawliteral(
/* TickerTape Additional Styles - Extends /assets/style.css */

/* Network scanning and management */
.network-list {
  max-height: 300px;
  overflow-y: auto;
  border: 1px solid rgba(255,255,255,0.1);
  border-radius: 8px;
  background: rgba(0,0,0,0.2);
  margin-top: 15px;
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

/* Additional button styles */
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

/* Token management */
.token-table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 1rem;
}

.token-table th, .token-table td {
  padding: 0.5rem;
  text-align: left;
  border-bottom: 1px solid rgba(255,255,255,0.2);
}

.token-display {
  background: rgba(255,255,255,0.1);
  padding: 10px;
  border-radius: 5px;
  font-family: monospace;
  word-break: break-all;
  margin: 10px 0;
  display: none;
  color: #fff;
  border: 1px solid rgba(255,255,255,0.2);
}

/* WiFi setup specific */
.wifi-setup {
  max-width: 500px;
  margin: 20px auto;
}

.network-scanner {
  margin-bottom: 20px;
}

/* Success page elements */
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
}

/* Security notices */
.security-notice {
  background: rgba(76, 175, 80, 0.1);
  border: 1px solid rgba(76, 175, 80, 0.3);
  border-radius: 8px;
  padding: 16px;
  margin-bottom: 20px;
}

.security-notice h4 {
  color: #4CAF50;
  margin: 0 0 8px 0;
  font-size: 1.1em;
}

/* Danger zones */
.danger-zone {
  border: 1px solid rgba(255, 87, 34, 0.5);
  background: rgba(255, 87, 34, 0.1);
}

.danger-zone h3 {
  color: #ff5722;
}

/* Additional responsive adjustments */
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
  
  .countdown {
    font-size: 2em;
  }
}
)rawliteral";

#endif // WEB_PLATFORM_STYLES_CSS_H