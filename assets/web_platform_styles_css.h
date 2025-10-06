#ifndef WEB_PLATFORM_STYLES_CSS_H
#define WEB_PLATFORM_STYLES_CSS_H

#include <Arduino.h>

// Web Platform specific styles - functional mechanics only
// Visual/themeable styles moved to main stylesheet
const char WEB_PLATFORM_STYLES_CSS[] PROGMEM = R"rawliteral(
/* Web Platform Specific Styles - Functional Only */

/* Auth UI Layout (non-themeable) */
.auth-body {
  padding: 10px;
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
  overflow: hidden;
}

.auth-header {
  padding: 30px 30px 20px;
  text-align: center;
}

.auth-title {
  font-size: 1.8em;
  margin: 0 0 10px 0;
}

.auth-subtitle {
  margin: 0;
  font-size: 0.9em;
  line-height: 1.4;
}

.auth-form {
  padding: 25px 30px;
}

.auth-submit {
  width: 100%;
  padding: 14px;
  margin-top: 25px;
  font-size: 1.1em;
}

.auth-footer {
  padding: 20px 30px 30px;
  text-align: center;
}

.auth-note {
  font-size: 0.85em;
  margin: 0;
  line-height: 1.4;
}

/* Password Toggle - Structure Only */
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
  font-size: 12px;
  padding: 6px 10px;
  font-weight: 600;
  transition: all 0.3s ease;
  min-width: 50px;
  text-align: center;
}

.password-toggle:hover {
  transform: scale(1.05);
}

.password-toggle:active {
  transform: scale(0.95);
}

/* Network Scanner - Structure Only */
.network-scanner {
  margin-bottom: 25px;
}

.scan-button {
  width: 100%;
  margin-bottom: 15px;
  position: relative;
}

.scan-button.scanning {
  animation: pulse 1.5s ease-in-out infinite;
}

.network-list {
  max-height: 400px;
  overflow-y: auto;
  margin-top: 15px;
  padding: 4px;
}

.network-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  margin: 8px;
  cursor: pointer;
  transition: all 0.3s ease;
  min-height: 60px;
}

.network-item:hover {
  transform: translateY(-2px);
}

.network-item.selected {
  transform: translateY(-2px);
}

.network-name {
  font-weight: bold;
  font-size: 16px;
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
}

.security-icon {
  font-size: 16px;
}

.signal-strength {
  font-size: 12px;
  font-weight: bold;
}

/* WiFi Setup Container - Structure Only */
.wifi-setup {
  padding: 25px;
  margin: 20px auto;
}

.wifi-setup h3 {
  margin-bottom: 20px;
  font-size: 1.3em;
  display: flex;
  align-items: center;
  gap: 10px;
}

.wifi-setup h3::before {
  content: "📡";
  font-size: 1.2em;
}

/* Loading states - Structure Only */
.network-list .loading,
.network-list .error {
  padding: 30px 20px;
  text-align: center;
  font-style: italic;
}

.network-list .loading::before {
  content: "";
  display: block;
  width: 24px;
  height: 24px;
  margin: 0 auto 10px;
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