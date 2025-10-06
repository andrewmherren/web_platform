#ifndef WEB_PLATFORM_STYLES_CSS_H
#define WEB_PLATFORM_STYLES_CSS_H

#include <Arduino.h>

// Web Platform specific styles - minimal version
// Only includes styles actually used by initial_setup_html and config_portal_html
const char WEB_PLATFORM_STYLES_CSS[] PROGMEM = R"rawliteral(
/* Web Platform Specific Styles - Minimal Version */

/* Authentication UI - Used by initial_setup_html */
.auth-body {
  padding: 10px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
}

.auth-container {
  width: 100%;
  max-width: 400px;
  margin: 0 auto;
}

.auth-card {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 15px;
  backdrop-filter: blur(10px);
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
  border: 1px solid rgba(255, 255, 255, 0.2);
  overflow: hidden;
}

.auth-header {
  padding: 30px 30px 20px;
  text-align: center;
  background: rgba(255, 255, 255, 0.05);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.auth-title {
  font-size: 1.8em;
  margin: 0 0 10px 0;
  color: #fff;
  font-weight: 600;
}

.auth-subtitle {
  color: rgba(255, 255, 255, 0.8);
  margin: 0;
  font-size: 0.9em;
  line-height: 1.4;
}

.auth-form {
  padding: 25px 30px;
}

.form-label {
  color: #fff;
  font-weight: 500;
  margin-bottom: 6px;
  display: block;
}

.form-help {
  font-size: 0.8em;
  color: rgba(255, 255, 255, 0.6);
  margin-top: 5px;
  line-height: 1.3;
}

.auth-submit {
  width: 100%;
  padding: 14px;
  margin-top: 25px;
  font-size: 1.1em;
  font-weight: 600;
}

.auth-footer {
  padding: 20px 30px 30px;
  text-align: center;
  background: rgba(0, 0, 0, 0.1);
  border-top: 1px solid rgba(255, 255, 255, 0.1);
}

.auth-note {
  color: rgba(255, 255, 255, 0.7);
  font-size: 0.85em;
  margin: 0;
  line-height: 1.4;
}

/* Password Toggle - Used by both pages */
.password-field {
  position: relative;
  display: flex;
  align-items: center;
}

.password-field .form-control {
  padding-right: 70px;
}

.password-toggle {
  position: absolute;
  right: 12px;
  background: rgba(33, 150, 243, 0.1);
  border: 1px solid rgba(33, 150, 243, 0.3);
  color: #2196F3;
  cursor: pointer;
  font-size: 12px;
  padding: 6px 10px;
  border-radius: 4px;
  font-weight: 600;
  transition: all 0.3s ease;
  min-width: 50px;
  text-align: center;
  backdrop-filter: blur(5px);
}

.password-toggle:hover {
  background: rgba(33, 150, 243, 0.2);
  border-color: rgba(33, 150, 243, 0.5);
  color: #1976D2;
  transform: scale(1.05);
}

.password-toggle:active {
  transform: scale(0.95);
}

/* Network Scanner - Used by config_portal_html */
.network-scanner {
  margin-bottom: 25px;
}

.scan-button {
  width: 100%;
  margin-bottom: 15px;
  font-weight: bold;
  position: relative;
}

.scan-button:disabled {
  opacity: 0.7;
  cursor: not-allowed;
}

.scan-button.scanning {
  background: rgba(255, 152, 0, 0.8);
  color: white;
  animation: pulse 1.5s ease-in-out infinite;
}

.network-list {
  max-height: 400px;
  overflow-y: auto;
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 8px;
  background: rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(5px);
  margin-top: 15px;
  padding: 4px;
}

.network-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  margin: 8px;
  background: rgba(255, 255, 255, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 10px;
  cursor: pointer;
  transition: all 0.3s ease;
  min-height: 60px;
  box-shadow: 0 3px 6px rgba(0, 0, 0, 0.15);
}

.network-item:hover {
  background: rgba(255, 255, 255, 0.15);
  border-color: rgba(255, 255, 255, 0.3);
  box-shadow: 0 5px 10px rgba(0, 0, 0, 0.2);
  transform: translateY(-2px);
}

.network-item.selected {
  background: rgba(76, 175, 80, 0.25);
  border-color: rgba(76, 175, 80, 0.7);
  box-shadow: 0 5px 15px rgba(76, 175, 80, 0.4);
  transform: translateY(-2px);
}

.network-name {
  font-weight: bold;
  font-size: 16px;
  color: #fff;
  margin-bottom: 4px;
  word-break: break-word;
  flex: 1;
  margin-right: 15px;
}

.network-info {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 4px;
  min-width: 80px;
}

.network-meta {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.7);
}

.security-icon {
  font-size: 16px;
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.3));
}

.signal-strength {
  font-size: 12px;
  font-weight: bold;
  color: #66D56A;
  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.3), 0 0 8px rgba(102, 213, 106, 0.3);
}

/* WiFi Setup Container - Used by config_portal_html */
.wifi-setup {
  background: rgba(255, 255, 255, 0.1);
  padding: 25px;
  border-radius: 12px;
  border: 1px solid rgba(255, 255, 255, 0.2);
  margin: 20px auto;
}

.wifi-setup h3 {
  margin-bottom: 20px;
  color: #fff;
  font-size: 1.3em;
  display: flex;
  align-items: center;
  gap: 10px;
}

.wifi-setup h3::before {
  content: "📡";
  font-size: 1.2em;
}

/* Loading states */
.network-list .loading,
.network-list .error {
  padding: 30px 20px;
  text-align: center;
  color: rgba(255, 255, 255, 0.7);
  font-style: italic;
}

.network-list .loading::before {
  content: "";
  display: block;
  width: 24px;
  height: 24px;
  margin: 0 auto 10px;
  border: 3px solid rgba(255, 255, 255, 0.3);
  border-top: 3px solid #2196F3;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

/* Mobile responsive adjustments */
@media (max-width: 480px) {
  .auth-body {
    padding: 15px;
    align-items: flex-start;
    padding-top: 30px;
  }
  
  .auth-header {
    padding: 25px 20px 15px;
  }
  
  .auth-title {
    font-size: 1.6em;
  }
  
  .auth-form {
    padding: 20px;
  }
  
  .password-field .form-control {
    padding-right: 80px;
  }
  
  .password-toggle {
    right: 10px;
    font-size: 13px;
    padding: 8px 10px;
    min-width: 60px;
  }
  
  .network-item {
    padding: 14px 16px;
    margin: 4px;
    min-height: 56px;
  }
  
  .network-item:hover {
    transform: none;
  }
  
  .wifi-setup {
    margin: 0;
    padding: 20px;
  }
}

@media (max-width: 768px) {
  .network-list {
    max-height: 300px;
  }
  
  .scan-button {
    padding: 16px;
    font-size: 16px;
    min-height: 50px;
  }
}
)rawliteral";

#endif // WEB_PLATFORM_STYLES_CSS_H