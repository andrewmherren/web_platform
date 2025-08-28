#include "assets/config_portal_css.h"
#include "assets/config_portal_html.h"
#include "assets/config_portal_js.h"
#include "assets/config_portal_success_html.h"
#include "web_platform.h"
#include <ArduinoJson.h>

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// WebPlatform config portal implementation
// This file contains functions specific to the WiFi configuration portal mode

String WebPlatform::handleConfigPortalRoot() {
  // Enhanced WiFi configuration page with security notice
  String html = FPSTR(CONFIG_PORTAL_HTML);

  // Replace placeholder with device name
  html.replace("{{DEVICE_NAME}}", deviceName);

  // Add appropriate security notice based on protocol
  String securityNotice;
  if (httpsEnabled) {
    securityNotice = R"(
    <div class="security-notice https">
        <h4><span class="security-icon-large">🔒</span> Secure Connection</h4>
        <p>This connection is secured with HTTPS encryption. Your WiFi password will be transmitted securely.</p>
    </div>)";
  } else {
    securityNotice = R"(
    <div class="security-notice">
        <h4><span class="security-icon-large">ℹ️</span> Connection Notice</h4>
        <p>This is a direct device connection. Only enter WiFi credentials on your trusted private network.</p>
    </div>)";
  }
  html.replace("{{SECURITY_NOTICE}}", securityNotice);

  // Register CSS and JS
  this->registerRoute(
      "/assets/config-portal.css",
      [](WebRequest &req, WebResponse &res) {
        res.setContent(FPSTR(CONFIG_PORTAL_CSS), "text/css");
        res.setHeader("Cache-Control", "public, max-age=3600");
      },
      {AuthType::NONE}, WebModule::WM_GET);

  this->registerRoute(
      "/assets/config-portal.js",
      [](WebRequest &req, WebResponse &res) {
        res.setContent(FPSTR(CONFIG_PORTAL_JS), "application/javascript");
        res.setHeader("Cache-Control", "public, max-age=3600");
      },
      {AuthType::NONE}, WebModule::WM_GET);

  return IWebModule::injectNavigationMenu(html);
}

// This function is no longer used - replaced with direct parameter handling
String WebPlatform::handleConfigPortalSave(const String &postBody) {
  // Deprecated - kept for compatibility
  return handleConfigPortalRoot();
}

String WebPlatform::handleWiFiStatusAPI() {
  DynamicJsonDocument doc(512);
  doc["connected"] = (WiFi.status() == WL_CONNECTED);
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["mode"] = (currentMode == CONFIG_PORTAL) ? "config" : "connected";
  doc["https_enabled"] = httpsEnabled;
  doc["device_name"] = deviceName;

  String response;
  serializeJson(doc, response);
  return response;
}

String WebPlatform::handleWiFiScanAPI() {
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.createNestedArray("networks");

  Serial.println("WebPlatform: Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  Serial.printf("WebPlatform: Found %d networks\n", n);

  // Sort networks by signal strength (RSSI)
  struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    bool encryption;
  };

  std::vector<WiFiNetwork> sortedNetworks;
  for (int i = 0; i < n; i++) {
    // Skip empty SSIDs and duplicates
    if (WiFi.SSID(i).length() == 0)
      continue;

    bool isDuplicate = false;
    for (const auto &network : sortedNetworks) {
      if (network.ssid == WiFi.SSID(i)) {
        isDuplicate = true;
        break;
      }
    }

    if (!isDuplicate) {
      sortedNetworks.push_back({WiFi.SSID(i), WiFi.RSSI(i),
#if defined(ESP8266)
                                WiFi.encryptionType(i) != ENC_TYPE_NONE
#else
                                WiFi.encryptionType(i) != WIFI_AUTH_OPEN
#endif
      });
    }
  }

  // Sort by signal strength (descending)
  std::sort(sortedNetworks.begin(), sortedNetworks.end(),
            [](const WiFiNetwork &a, const WiFiNetwork &b) {
              return a.rssi > b.rssi;
            });

  // Add to JSON response
  for (const auto &network : sortedNetworks) {
    JsonObject netObj = networks.createNestedObject();
    netObj["ssid"] = network.ssid;
    netObj["rssi"] = network.rssi;
    netObj["encryption"] = network.encryption;
  }

  String response;
  serializeJson(doc, response);
  return response;
}

String WebPlatform::handleWiFiResetAPI() {
  // Clear WiFi credentials
  EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
  EEPROM.commit();

  // Schedule restart
  delay(1000);
  ESP.restart();

  return R"({"status": "restarting"})";
}

void WebPlatform::setupCaptivePortal() {
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("WebPlatform: Captive portal DNS started");
}

bool WebPlatform::isCaptivePortalRequest(const String &host) {
  // Simple captive portal detection
  return (host.indexOf("captive") != -1 || host.indexOf("generate") != -1 ||
          host.indexOf("connectivitycheck") != -1);
}

void WebPlatform::setupConfigPortalMode() {
  Serial.println("WebPlatform: Setting up config portal routes");

  registerConfigPortalRoutes();
  setupCaptivePortal();

  // Register static assets for the config portal
  this->registerRoute(
      "/assets/config-portal.css",
      [](WebRequest &req, WebResponse &res) {
        res.setContent(FPSTR(CONFIG_PORTAL_CSS), "text/css");
        res.setHeader("Cache-Control", "public, max-age=3600");
      },
      {AuthType::NONE}, WebModule::WM_GET);

  this->registerRoute(
      "/assets/config-portal.js",
      [](WebRequest &req, WebResponse &res) {
        res.setContent(FPSTR(CONFIG_PORTAL_JS), "application/javascript");
        res.setHeader("Cache-Control", "public, max-age=3600");
      },
      {AuthType::NONE}, WebModule::WM_GET);
}

void WebPlatform::registerConfigPortalRoutes() {
  if (!server) {
    Serial.println("WebPlatform: No HTTP server to register config portal "
                   "routes on (HTTPS-only mode)");
    return;
  }

  // Main configuration page
  server->on("/", HTTP_GET, [this]() {
    IWebModule::setCurrentPath("/");
    String response = handleConfigPortalRoot();
    server->send(200, "text/html", response);
  });

  // Save WiFi credentials
  server->on("/save", HTTP_POST, [this]() {
    Serial.println("WebPlatform: Received WiFi save request");
    Serial.printf("Args count: %d\n", server->args());
    for (int i = 0; i < server->args(); i++) {
      Serial.printf("Arg %d: %s = %s\n", i, server->argName(i).c_str(),
                    server->arg(i).c_str());
    }

    String ssid = server->arg("ssid");
    String password = server->arg("password");
    Serial.printf(
        "SSID: %s, Password length: %d chars (redacted for security)\n",
        ssid.c_str(), password.length());

    if (ssid.length() > 0) {
      saveWiFiCredentials(ssid, password);

      // Create success page with device restart countdown
      String html = FPSTR(CONFIG_PORTAL_SUCCESS_HTML);

      // Replace placeholder with device name and network SSID
      html.replace("{{DEVICE_NAME}}", deviceName);
      html.replace("{{NETWORK_SSID}}", ssid);

      // Reset the saved flag to ensure credentials are re-loaded after restart
      EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
      EEPROM.commit();

      // Now save credentials again to make sure they're properly stored
      saveWiFiCredentials(ssid, password);

      // Verify credentials were properly saved
      String checkSsid, checkPass;
      bool credentialsValid = loadWiFiCredentials(checkSsid, checkPass);
      Serial.printf(
          "WebPlatform: Credential verification %s - SSID match: %s\n",
          credentialsValid ? "passed" : "failed",
          checkSsid == ssid ? "yes" : "no");

      server->send(200, "text/html", html);

      // Schedule restart after sending response
      Serial.println(
          "WebPlatform: WiFi credentials saved - restarting in 3 seconds...");
      delay(1000);
      Serial.println("WebPlatform: Restarting in 2 seconds...");
      delay(1000);
      Serial.println("WebPlatform: Restarting in 1 second...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.println(
          "WebPlatform: No SSID provided, returning to config portal");
      String response = handleConfigPortalRoot();
      server->send(200, "text/html", response);
    }
  });

  // API endpoints
  server->on("/api/status", HTTP_GET, [this]() {
    String response = handleWiFiStatusAPI();
    server->send(200, "application/json", response);
  });

  server->on("/api/scan", HTTP_GET, [this]() {
    String response = handleWiFiScanAPI();
    server->send(200, "application/json", response);
  });

  server->on("/api/reset", HTTP_POST, [this]() {
    String response = handleWiFiResetAPI();
    server->send(200, "application/json", response);
  });
}