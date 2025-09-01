#ifndef WEB_UI_STYLES_H
#define WEB_UI_STYLES_H

#include <Arduino.h>

// Default CSS styles for web interface - based on LOCAL_WEB_UI_STYLEGUIDE.md
const char WEB_UI_DEFAULT_CSS[] PROGMEM = R"css(
/* Web UI Default Styles */
/* Based on LOCAL_WEB_UI_STYLEGUIDE.md glass morphism design */

/* Reset and base styles */
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  margin: 0;
  padding: 20px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  color: white;
}

/* Container - Main layout wrapper */
.container {
  max-width: 800px;
  margin: 0 auto;
  background: rgba(255, 255, 255, 0.1);
  padding: 30px;
  border-radius: 15px;
  backdrop-filter: blur(10px);
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
}

/* Typography */
h1 {
  text-align: center;
  margin-bottom: 30px;
  font-size: 2.5em;
  text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
}

h2 {
  margin-bottom: 20px;
  font-size: 2em;
  text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.3);
}

h3 {
  margin: 0 0 10px 0;
  color: #fff;
  font-size: 1.2em;
}

p {
  margin: 5px 0;
  opacity: 0.9;
}

/* Grid system for responsive layouts */
.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
  margin-bottom: 30px;
}

/* Cards - Primary content containers */
.status-card {
  background: rgba(255, 255, 255, 0.15);
  padding: 20px;
  border-radius: 10px;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.status-card h3 {
  margin: 0 0 10px 0;
  color: #fff;
  font-size: 1.2em;
}

/* Status indicator classes */
.success { 
  color: #4CAF50; 
  font-weight: bold; 
}

.info { 
  color: #2196F3; 
  font-weight: bold; 
}

.warning { 
  color: #FF9800; 
  font-weight: bold; 
}

.error { 
  color: #f44336; 
  font-weight: bold; 
}

/* Form elements */
.form-group {
  margin-bottom: 15px;
}

label {
  display: block;
  margin-bottom: 8px;
  font-weight: bold;
  color: #fff;
}

.form-control {
  width: 100%;
  padding: 12px;
  border: 1px solid rgba(255, 255, 255, 0.3);
  border-radius: 8px;
  box-sizing: border-box;
  background: rgba(255, 255, 255, 0.1);
  color: white;
  backdrop-filter: blur(5px);
}

.form-control:focus {
  outline: none;
  border-color: rgba(255, 255, 255, 0.5);
  background: rgba(255, 255, 255, 0.15);
}

.form-control option {
  background: #764ba2;
  color: white;
}

/* Button system */
.button-group {
  display: flex;
  gap: 15px;
  margin-top: 20px;
  justify-content: center;
  flex-wrap: wrap;
}

.btn {
  padding: 12px 24px;
  border: none;
  border-radius: 25px;
  cursor: pointer;
  font-weight: bold;
  transition: all 0.3s ease;
  border: 1px solid rgba(255, 255, 255, 0.3);
  text-decoration: none;
  display: inline-block;
  text-align: center;
}

.btn:hover:not(:disabled) {
  transform: translateY(-2px);
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-primary {
  background: rgba(76, 175, 80, 0.8);
  color: white;
}

.btn-primary:hover:not(:disabled) {
  background: rgba(76, 175, 80, 1);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.2);
  color: white;
}

.btn-secondary:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.3);
}

.btn-warning {
  background: rgba(255, 152, 0, 0.8);
  color: white;
}

.btn-warning:hover:not(:disabled) {
  background: rgba(255, 152, 0, 1);
}

.btn-danger {
  background: rgba(244, 67, 54, 0.8);
  color: white;
}

.btn-danger:hover:not(:disabled) {
  background: rgba(244, 67, 54, 1);
}

/* Navigation links */
.nav-links {
  display: flex;
  justify-content: center;
  gap: 20px;
  margin-top: 30px;
  flex-wrap: wrap;
}

.nav-links a {
  background: rgba(255, 255, 255, 0.2);
  color: white;
  text-decoration: none;
  padding: 12px 24px;
  border-radius: 25px;
  border: 1px solid rgba(255, 255, 255, 0.3);
  transition: all 0.3s ease;
}

.nav-links a:hover {
  background: rgba(255, 255, 255, 0.3);
  transform: translateY(-2px);
}

/* Status messages */
.status-message {
  padding: 15px;
  border-radius: 10px;
  margin-top: 15px;
  font-weight: bold;
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.status-message.error {
  background: rgba(244, 67, 54, 0.2);
  border-color: rgba(244, 67, 54, 0.5);
  color: #ffcdd2;
}

.status-message.success {
  background: rgba(76, 175, 80, 0.2);
  border-color: rgba(76, 175, 80, 0.5);
  color: #c8e6c9;
}

.status-message.info {
  background: rgba(33, 150, 243, 0.2);
  border-color: rgba(33, 150, 243, 0.5);
  color: #bbdefb;
}

.status-message.warning {
  background: rgba(255, 152, 0, 0.2);
  border-color: rgba(255, 152, 0, 0.5);
  color: #ffe0b2;
}

/* Utility classes */
.hidden {
  display: none !important;
}

.text-center {
  text-align: center;
}

.text-left {
  text-align: left;
}

.text-right {
  text-align: right;
}

.mb-0 { margin-bottom: 0; }
.mb-1 { margin-bottom: 10px; }
.mb-2 { margin-bottom: 20px; }
.mb-3 { margin-bottom: 30px; }

.mt-0 { margin-top: 0; }
.mt-1 { margin-top: 10px; }
.mt-2 { margin-top: 20px; }
.mt-3 { margin-top: 30px; }

/* Footer */
.footer {
  text-align: center;
  margin-top: 40px;
  opacity: 0.7;
  font-size: 0.9em;
}

/* Responsive design */
@media (max-width: 600px) {
  .container {
    padding: 20px;
    margin: 10px;
  }
  
  h1 {
    font-size: 2em;
  }
  
  .status-grid {
    grid-template-columns: 1fr;
    gap: 15px;
  }
  
  .nav-links {
    flex-direction: column;
    align-items: center;
  }
  
  .button-group {
    flex-direction: column;
    align-items: center;
  }
  
  .btn {
    width: 100%;
    max-width: 300px;
  }
}

/* Loading and animation states */
@keyframes pulse {
  0% { opacity: 1; }
  50% { opacity: 0.5; }
  100% { opacity: 1; }
}

.loading {
  animation: pulse 1.5s ease-in-out infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.spinning {
  animation: spin 1s linear infinite;
}

/* Special card types */
.pdo-container {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
  margin-bottom: 20px;
}

.pdo-card {
  border: 2px solid rgba(255, 255, 255, 0.2);
  border-radius: 10px;
  padding: 15px;
  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(5px);
}

.pdo-card.active {
  border-color: rgba(76, 175, 80, 0.8);
  background: rgba(76, 175, 80, 0.2);
}

.pdo-card.fixed {
  border-color: rgba(255, 152, 0, 0.8);
  background: rgba(255, 152, 0, 0.2);
}

.pdo-header {
  font-weight: bold;
  margin-bottom: 10px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  color: #fff;
}

.pdo-badge {
  background: rgba(33, 150, 243, 0.8);
  color: white;
  padding: 4px 12px;
  border-radius: 15px;
  font-size: 0.75em;
  font-weight: bold;
}

.pdo-badge.fixed {
  background: rgba(255, 152, 0, 0.8);
}

.pdo-badge.active {
  background: rgba(76, 175, 80, 0.8);
}

.pdo-details {
  font-size: 0.9em;
  line-height: 1.4;
  color: rgba(255, 255, 255, 0.9);
}

/* Refresh button modifier */
.refresh-button {
  margin-left: 10px;
  padding: 8px 16px;
  font-size: 0.8em;
}

/* Error page specific styles */
.error-page {
  text-align: center;
  padding: 40px 20px;
}

.error-page h1 {
  font-size: 3em;
  margin-bottom: 20px;
  color: #fff;
  text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
}

.error-description {
  font-size: 1.2em;
  margin-bottom: 30px;
  color: rgba(255, 255, 255, 0.9);
  line-height: 1.5;
}

.error-actions {
  display: flex;
  gap: 20px;
  justify-content: center;
  flex-wrap: wrap;
  margin-top: 30px;
}

.error-actions .btn {
  min-width: 150px;
}

@media (max-width: 600px) {
  .error-page h1 {
    font-size: 2.5em;
  }
  
  .error-description {
    font-size: 1em;
  }
  
  .error-actions {
    flex-direction: column;
    align-items: center;
  }
  
  .error-actions .btn {
    width: 100%;
    max-width: 300px;
  }
}
)css";

#endif // WEB_UI_STYLES_H