#include "web_platform.h"

// WebPlatform WiFi management implementation
// This file contains the WiFi-specific functionality of the WebPlatform class

void WebPlatform::setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSIDBuffer, apPassword);

  Serial.printf("WebPlatform: Access Point started: %s\n", apSSIDBuffer);
  Serial.printf("WebPlatform: AP IP address: %s\n",
                WiFi.softAPIP().toString().c_str());
}

void WebPlatform::setupmDNS() {
  if (MDNS.begin(deviceName)) {
    MDNS.addService("http", "tcp", serverPort);
    Serial.printf("WebPlatform: mDNS started: %s.local\n", deviceName);
  } else {
    Serial.println("WebPlatform: mDNS failed to start");
  }
}

bool WebPlatform::loadWiFiCredentials(String &ssid, String &password) {
  // Check if credentials exist
  if (EEPROM.read(WIFI_CONFIG_FLAG_ADDR) != 1) {
    Serial.println("WebPlatform: No WiFi credentials found in EEPROM");
    return false;
  }

  // Read SSID
  ssid = "";
  for (int i = 0; i < 32; i++) {
    char c = EEPROM.read(WIFI_SSID_ADDR + i);
    if (c == 0)
      break;
    ssid += c;
  }

  // Read password
  password = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(WIFI_PASS_ADDR + i);
    if (c == 0)
      break;
    password += c;
  }

  if (ssid.length() > 0) {
    Serial.printf("WebPlatform: Loaded stored WiFi credentials - SSID: %s, "
                  "Password length: %d chars\n",
                  ssid.c_str(), password.length());
    return true;
  } else {
    Serial.println("WebPlatform: Invalid or empty WiFi credentials in EEPROM");
    return false;
  }
}

void WebPlatform::saveWiFiCredentials(const String &ssid,
                                      const String &password) {
  // Clear previous data
  for (int i = 0; i < 96; i++) {
    EEPROM.write(WIFI_SSID_ADDR + i, 0);
  }

  // Write SSID
  for (int i = 0; i < ssid.length() && i < 31; i++) {
    EEPROM.write(WIFI_SSID_ADDR + i, ssid[i]);
  }

  // Write password
  for (int i = 0; i < password.length() && i < 63; i++) {
    EEPROM.write(WIFI_PASS_ADDR + i, password[i]);
  }

  // Set configuration flag - do this last in case of failure midway
  EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 1);
  bool success = EEPROM.commit();

  // Double-check that the write was successful
  uint8_t flagCheck = EEPROM.read(WIFI_CONFIG_FLAG_ADDR);
  if (flagCheck != 1) {
    Serial.println(
        "WebPlatform: ERROR - Failed to write WiFi config flag to EEPROM!");
    success = false;
  }

  Serial.printf("WebPlatform: WiFi credentials saved for SSID: %s, Password "
                "length: %d chars, EEPROM commit %s\n",
                ssid.c_str(), password.length(),
                success ? "successful" : "failed");
}

bool WebPlatform::connectToStoredWiFi() {
  String ssid, password;
  if (!loadWiFiCredentials(ssid, password)) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait up to 10 seconds for connection
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  return WiFi.status() == WL_CONNECTED;
}

void WebPlatform::updateConnectionState() {
  if (currentMode == CONNECTED && WiFi.status() != WL_CONNECTED) {
    Serial.println(
        "WebPlatform: WiFi connection lost, switching to config portal");
    connectionState = WIFI_CONNECTION_FAILED;
    // Note: In a full implementation, this might trigger a restart
    // For now, we'll maintain current behavior
  }
}

void WebPlatform::determinePlatformMode() {
  String ssid, password;

  if (loadWiFiCredentials(ssid, password) && ssid.length() > 0) {
    Serial.println(
        "WebPlatform: Found stored WiFi credentials, attempting connection...");

    if (connectToStoredWiFi()) {
      currentMode = CONNECTED;
      connectionState = WIFI_CONNECTED;
      setupmDNS();
      Serial.printf("WebPlatform: Connected to WiFi: %s\n",
                    WiFi.SSID().c_str());
      Serial.printf("WebPlatform: IP address: %s\n",
                    WiFi.localIP().toString().c_str());
    } else {
      Serial.println("WebPlatform: Failed to connect to stored WiFi, starting "
                     "config portal");
      currentMode = CONFIG_PORTAL;
      connectionState = WIFI_CONFIG_PORTAL;
      setupAccessPoint();
    }
  } else {
    Serial.println(
        "WebPlatform: No WiFi credentials found, starting config portal");
    currentMode = CONFIG_PORTAL;
    connectionState = WIFI_CONFIG_PORTAL;
    setupAccessPoint();
  }
}

void WebPlatform::initializeEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  Serial.printf("WebPlatform: EEPROM initialized with size %d bytes\n",
                EEPROM_SIZE);

  // Debug: Check the flag status
  uint8_t configFlag = EEPROM.read(WIFI_CONFIG_FLAG_ADDR);
  Serial.printf("WebPlatform: EEPROM config flag status: %d (1=configured, "
                "0=unconfigured)\n",
                configFlag);
}

String WebPlatform::extractPostParameter(const String &postBody,
                                         const String &paramName) {
  String searchStr = paramName + "=";
  int startIndex = postBody.indexOf(searchStr);
  if (startIndex == -1)
    return "";

  startIndex += searchStr.length();
  int endIndex = postBody.indexOf("&", startIndex);
  if (endIndex == -1)
    endIndex = postBody.length();

  String value = postBody.substring(startIndex, endIndex);
  // URL decode would go here in a full implementation
  return value;
}

void WebPlatform::resetWiFiCredentials() {
  EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
  EEPROM.commit();
  Serial.println("WebPlatform: WiFi credentials reset");
}

void WebPlatform::startConfigPortal() {
  currentMode = CONFIG_PORTAL;
  connectionState = WIFI_CONFIG_PORTAL;
  setupAccessPoint();
  Serial.println("WebPlatform: Config portal started");
}

String WebPlatform::getBaseUrl() const {
  if (currentMode == CONFIG_PORTAL) {
    return "http://" + WiFi.softAPIP().toString() + ":" + String(serverPort);
  } else {
    String protocol = httpsEnabled ? "https" : "http";
    return protocol + "://" + WiFi.localIP().toString() + ":" +
           String(serverPort);
  }
}
