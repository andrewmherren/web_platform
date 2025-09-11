#ifndef MAKER_API_STYLES_CSS_H
#define MAKER_API_STYLES_CSS_H

#include <Arduino.h>

// API Documentation styles - designed to work alongside web_ui_styles.h
const char MAKER_API_STYLES_CSS[] PROGMEM = R"css(
/* API Documentation Styles */
/* Designed to complement web_ui_styles.h glass morphism theme */

.api-endpoints {
  margin-top: 15px;
}

.api-section {
  margin-bottom: 25px;
}

.api-section-title {
  color: #fff;
  font-size: 1.1em;
  font-weight: 600;
  margin-bottom: 12px;
  padding-bottom: 5px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.2);
  display: flex;
  align-items: center;
  gap: 8px;
}

.api-section-title::before {
  content: "- ";
  font-size: 1em;
  opacity: 0.8;
}

.api-endpoint {
  display: flex;
  align-items: center;
  margin-bottom: 10px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
  border: 1px solid rgba(255, 255, 255, 0.1);
  gap: 12px;
  flex-wrap: wrap;
  transition: all 0.3s ease;
}

.api-endpoint:hover {
  background: rgba(255, 255, 255, 0.08);
  border-color: rgba(255, 255, 255, 0.2);
  transform: translateX(2px);
}

.api-method {
  font-family: 'Courier New', 'Monaco', 'Menlo', monospace;
  font-size: 0.8em;
  font-weight: bold;
  padding: 4px 8px;
  border-radius: 4px;
  min-width: 50px;
  text-align: center;
  color: #fff;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

.api-method.get {
  background: linear-gradient(135deg, rgba(76, 175, 80, 0.9), rgba(56, 142, 60, 0.9));
}

.api-method.post {
  background: linear-gradient(135deg, rgba(33, 150, 243, 0.9), rgba(25, 118, 210, 0.9));
}

.api-method.put {
  background: linear-gradient(135deg, rgba(255, 152, 0, 0.9), rgba(245, 124, 0, 0.9));
}

.api-method.patch {
  background: linear-gradient(135deg, rgba(156, 39, 176, 0.9), rgba(123, 31, 162, 0.9));
}

.api-method.delete {
  background: linear-gradient(135deg, rgba(244, 67, 54, 0.9), rgba(211, 47, 47, 0.9));
}

.api-path {
  font-family: 'Courier New', 'Monaco', 'Menlo', monospace;
  font-size: 0.9em;
  color: #fff;
  background: rgba(0, 0, 0, 0.2);
  padding: 6px 10px;
  border-radius: 4px;
  flex: 1;
  min-width: 200px;
  font-weight: 500;
  letter-spacing: 0.5px;
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.api-description {
  color: rgba(255, 255, 255, 0.8);
  font-size: 0.9em;
  flex: 2;
  min-width: 150px;
  line-height: 1.4;
}

/* API Parameters and Response styling */
.api-details {
  margin-top: 8px;
  padding: 10px;
  background: rgba(0, 0, 0, 0.1);
  border-radius: 6px;
  border-left: 3px solid rgba(255, 255, 255, 0.3);
}

.api-params {
  font-size: 0.8em;
  color: rgba(255, 255, 255, 0.7);
  margin-top: 5px;
}

.api-params strong {
  color: rgba(255, 255, 255, 0.9);
}

.api-param-list {
  list-style: none;
  padding-left: 0;
  margin: 5px 0;
}

.api-param-list li {
  padding: 2px 0;
  font-family: 'Courier New', monospace;
  font-size: 0.85em;
}

.api-param-name {
  color: #4CAF50;
  font-weight: bold;
}

.api-param-type {
  color: #2196F3;
  font-style: italic;
}

.api-param-required {
  color: #FF5722;
  font-size: 0.7em;
  text-transform: uppercase;
  font-weight: bold;
}

/* Authentication indicator */
.api-auth-indicator {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  font-size: 0.7em;
  padding: 2px 6px;
  border-radius: 3px;
  margin-left: auto;
}

.api-auth-public {
  background: rgba(76, 175, 80, 0.3);
  color: #C8E6C9;
}

.api-auth-session {
  background: rgba(255, 152, 0, 0.3);
  color: #FFE0B2;
}

.api-auth-token {
  background: rgba(244, 67, 54, 0.3);
  color: #FFCDD2;
}

.api-auth-mixed {
  background: rgba(156, 39, 176, 0.3);
  color: #E1BEE7;
}

/* Collapsible API sections */
.api-section-collapsible .api-section-title {
  cursor: pointer;
  user-select: none;
}

.api-section-collapsible .api-section-title:hover {
  color: rgba(255, 255, 255, 0.8);
}

.api-section-collapsible .api-section-title::after {
  content: "▼";
  margin-left: auto;
  transition: transform 0.3s ease;
  font-size: 0.8em;
}

.api-section-collapsible.collapsed .api-section-title::after {
  transform: rotate(-90deg);
}

.api-section-collapsible.collapsed .api-endpoint {
  display: none;
}

/* API status badges */
.api-status-badge {
  display: inline-block;
  font-size: 0.7em;
  padding: 2px 6px;
  border-radius: 10px;
  font-weight: bold;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.api-status-stable {
  background: rgba(76, 175, 80, 0.3);
  color: #C8E6C9;
}

.api-status-beta {
  background: rgba(255, 152, 0, 0.3);
  color: #FFE0B2;
}

.api-status-deprecated {
  background: rgba(244, 67, 54, 0.3);
  color: #FFCDD2;
}

/* Responsive design for API documentation */
@media (max-width: 600px) {
  .api-endpoint {
    flex-direction: column;
    align-items: flex-start;
    gap: 8px;
  }
  
  .api-method {
    align-self: flex-start;
    min-width: 60px;
  }
  
  .api-path {
    width: 100%;
    min-width: unset;
    word-break: break-all;
  }
  
  .api-description {
    width: 100%;
    min-width: unset;
  }
  
  .api-auth-indicator {
    margin-left: 0;
    margin-top: 5px;
  }
  
  .api-section-title {
    font-size: 1em;
  }
  
  .api-details {
    margin-top: 10px;
    padding: 8px;
  }
}

/* Syntax highlighting for code examples */
.api-code-example {
  background: rgba(0, 0, 0, 0.3);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 6px;
  padding: 12px;
  margin: 10px 0;
  font-family: 'Courier New', monospace;
  font-size: 0.85em;
  color: #E8E8E8;
  overflow-x: auto;
  white-space: pre;
}

.api-code-example .keyword {
  color: #569CD6;
}

.api-code-example .string {
  color: #CE9178;
}

.api-code-example .number {
  color: #B5CEA8;
}

.api-code-example .comment {
  color: #6A9955;
  font-style: italic;
}

/* API section separators */
.api-section:not(:last-child)::after {
  content: "";
  display: block;
  height: 1px;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  margin: 20px 0;
}
)css";

#endif // MAKER_API_STYLES_H