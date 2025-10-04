#ifndef WEB_PLATFORM_STYLES_CSS_H
#define WEB_PLATFORM_STYLES_CSS_H

#include <Arduino.h>

// Web Platform specific styles: network scanning, token management,
// authentication UI
const char WEB_PLATFORM_STYLES_CSS[] PROGMEM = R"rawliteral(
/* Web Platform Specific Styles */

/* Network Scanner Styles */
.network-scanner {
  margin-bottom: 25px;
}

.scan-button {
  width: 100%;
  margin-bottom: 15px;
  font-weight: bold;
  position: relative;
}

.scan-button:hover:not(:disabled) {
  transform: translateY(-2px);
}

.scan-button:disabled {
  opacity: 0.7;
  cursor: not-allowed;
  transform: none;
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

.network-item:first-child {
  margin-top: 8px;
}

.network-item:last-child {
  margin-bottom: 8px;
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

.signal-bars {
  display: flex;
  gap: 2px;
  align-items: flex-end;
  margin-left: 4px;
}

.signal-bar {
  width: 3px;
  background: rgba(255, 255, 255, 0.3);
  border-radius: 1px;
}

.signal-bar.active {
  background: #66D56A;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3), 0 0 8px rgba(102, 213, 106, 0.3);
}

.signal-bar:nth-child(1) { height: 4px; }
.signal-bar:nth-child(2) { height: 8px; }
.signal-bar:nth-child(3) { height: 12px; }
.signal-bar:nth-child(4) { height: 16px; }

/* WiFi Setup specific styling */
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

/* Password field with toggle */
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

/* Loading and empty states */
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

.network-list .error::before {
  content: "⚠️";
  display: block;
  font-size: 24px;
  margin-bottom: 10px;
}

/* Token Management and Display Styles */
.token-container {
  margin-top: 15px;
}

.token-container label {
  font-weight: bold;
  margin-bottom: 8px;
  display: block;
}

.token-display-box {
  display: flex;
  gap: 10px;
  align-items: center;
}

.token-input {
  flex: 1;
  font-family: 'Courier New', monospace;
  font-size: 12px;
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid rgba(255, 255, 255, 0.3);
  color: #fff;
  padding: 10px;
  border-radius: 5px;
  word-break: break-all;
}

.token-input:focus {
  outline: none;
  border-color: rgba(255, 255, 255, 0.5);
}

.btn-copy {
  background: rgba(33, 150, 243, 0.8);
  color: white;
  border: none;
  padding: 10px 15px;
  border-radius: 5px;
  cursor: pointer;
  font-size: 12px;
  font-weight: bold;
  transition: all 0.3s ease;
  white-space: nowrap;
}

.btn-copy:hover {
  background: rgba(33, 150, 243, 1);
}

.btn-copy.btn-success {
  background: rgba(76, 175, 80, 0.8);
}

.token-warning {
  font-size: 0.9em;
  margin-top: 10px;
  color: #ffe0b2;
}

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

/* User Profile and Authentication UI */
.user-profile {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  padding: 20px;
  margin-bottom: 25px;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.profile-header {
  display: flex;
  align-items: center;
  gap: 15px;
  margin-bottom: 15px;
}

.profile-avatar {
  width: 60px;
  height: 60px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24px;
  color: white;
  font-weight: bold;
  text-transform: uppercase;
}

.profile-info {
  flex: 1;
}

.profile-name {
  font-size: 1.4em;
  font-weight: bold;
  margin: 0 0 5px 0;
}

.profile-role {
  color: rgba(255, 255, 255, 0.7);
  font-size: 0.9em;
  display: inline-block;
  background: rgba(33, 150, 243, 0.3);
  padding: 3px 10px;
  border-radius: 12px;
}

.profile-role.admin {
  background: rgba(244, 67, 54, 0.3);
}

.profile-details {
  margin-top: 15px;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
}

.profile-detail {
  margin-bottom: 10px;
}

.detail-label {
  font-size: 0.8em;
  color: rgba(255, 255, 255, 0.6);
  margin-bottom: 5px;
}

.detail-value {
  font-weight: bold;
}

/* API Tokens Section */
.tokens-section {
  margin-top: 30px;
}

.tokens-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.tokens-table {
  width: 100%;
  border-collapse: collapse;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
  overflow: hidden;
}

.tokens-table th {
  text-align: left;
  padding: 12px 15px;
  background: rgba(0, 0, 0, 0.2);
  color: white;
}

.tokens-table td {
  padding: 12px 15px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.tokens-table tr:last-child td {
  border-bottom: none;
}

.token-actions {
  display: flex;
  gap: 8px;
}

.btn-token {
  padding: 5px 10px;
  font-size: 0.8em;
  border-radius: 4px;
}

.empty-tokens {
  text-align: center;
  padding: 30px;
  color: rgba(255, 255, 255, 0.6);
  background: rgba(0, 0, 0, 0.1);
  border-radius: 8px;
}

.empty-tokens p {
  margin-bottom: 15px;
}

/* User Settings Form */
.settings-form {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  padding: 20px;
  margin-top: 25px;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.settings-form h3 {
  margin-bottom: 20px;
  font-size: 1.3em;
  display: flex;
  align-items: center;
  gap: 10px;
}

.settings-form h3::before {
  content: "⚙️";
  font-size: 1.2em;
}

.form-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
  margin-bottom: 15px;
}

/* Success page elements */
.success-icon {
  font-size: 4em;
  color: #4CAF50;
  margin-bottom: 20px;
  animation: pulse 2s infinite;
  text-align: center;
  display: block;
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
  transition: width 1s ease;
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

/* Authentication UI Styles */
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

/* Mobile responsive - Small screens (iPhone, etc.) */
@media (max-width: 480px) {
  .auth-body {
    padding: 15px;
    align-items: flex-start;
    padding-top: 30px;
  }
  
  .auth-container {
    max-width: none;
    width: 100%;
  }
  
  .auth-header {
    padding: 25px 20px 15px;
  }
  
  .auth-title {
    font-size: 1.6em;
  }
  
  .auth-subtitle {
    font-size: 0.85em;
  }
  
  .auth-form {
    padding: 20px;
  }
  
  .auth-footer {
    padding: 15px 20px 25px;
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
}

/* Mobile responsive - Medium screens */
@media (max-width: 768px) {
  body {
    padding: 15px;
  }
  
  .container {
    padding: 20px;
    margin: 0;
  }
  
  .wifi-setup {
    margin: 0;
    padding: 20px;
  }
  
  .wifi-setup h3 {
    font-size: 1.2em;
  }
  
  /* Enhanced network list for mobile */
  .network-scanner {
    margin-bottom: 20px;
  }
  
  .network-list {
    max-height: 300px;
    padding: 2px;
    margin-top: 12px;
  }
  
  .network-item {
    padding: 14px 16px;
    margin: 4px;
    min-height: 56px; /* Improved touch target */
    border-radius: 8px;
  }
  
  .network-item:hover {
    transform: none; /* Disable hover effects on mobile */
  }
  
  .network-item:active {
    background: rgba(255, 255, 255, 0.2);
    transform: scale(0.98);
  }
  
  .network-name {
    font-size: 15px;
    margin-bottom: 3px;
    margin-right: 12px;
  }
  
  .network-info {
    min-width: 75px;
  }
  
  .network-meta {
    font-size: 11px;
    gap: 6px;
  }
  
  .signal-bars {
    gap: 1.5px;
  }
  
  .security-icon {
    font-size: 15px;
  }
  
  /* Better scan button for mobile */
  .scan-button {
    padding: 16px;
    font-size: 16px;
    border-radius: 10px;
    min-height: 50px; /* Better touch target */
  }
  
  /* Enhanced form controls for mobile */
  .form-control {
    padding: 14px 12px;
    font-size: 16px; /* Prevent zoom on iOS */
    border-radius: 8px;
    min-height: 48px; /* Better touch target */
  }
  
  .password-field .form-control {
    padding-right: 70px;
  }
  
  .password-toggle {
    font-size: 13px;
    padding: 6px 10px;
    right: 10px;
    min-width: 55px;
    min-height: 36px; /* Better touch target */
  }
  
  /* Button improvements for mobile */
  .btn {
    padding: 14px 20px;
    font-size: 16px;
    min-height: 48px; /* Better touch target */
    border-radius: 8px;
  }
  
  .button-group {
    flex-direction: column;
    align-items: stretch;
    gap: 12px;
  }
  
  .button-group .btn {
    width: 100%;
  }
  
  /* Loading states improvements */
  .network-list .loading,
  .network-list .error {
    padding: 25px 15px;
    font-size: 14px;
  }
  
  .loading::before {
    width: 20px !important;
    height: 20px !important;
  }
}

/* Mobile responsive - Standard mobile screens (600px) */
@media (max-width: 600px) {
  .countdown {
    font-size: 2em;
  }
  
  .token-display-box {
    flex-direction: column;
    gap: 10px;
  }
  
  .btn-copy {
    width: 100%;
  }
  
  .form-row {
    grid-template-columns: 1fr;
  }
  
  .profile-header {
    flex-direction: column;
    text-align: center;
  }
  
  .profile-info {
    width: 100%;
  }
  
  .tokens-table {
    display: block;
    overflow-x: auto;
    font-size: 14px;
  }
  
  .tokens-table th,
  .tokens-table td {
    padding: 8px 10px;
  }
}

/* Large mobile screens and small tablets */
@media (max-width: 900px) and (min-width: 601px) {
  .container {
    max-width: 90%;
    padding: 25px;
  }
  
  .wifi-setup {
    padding: 22px;
  }
  
  .network-item {
    padding: 13px 18px;
    min-height: 55px;
  }
  
  .form-control {
    padding: 13px;
    font-size: 15px;
  }
}
)rawliteral";

#endif // WEB_PLATFORM_STYLES_CSS_H