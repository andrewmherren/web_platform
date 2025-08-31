
#include "../../include/web_platform.h"
#include "../../assets/config_portal_html.h"
#include "../../assets/config_portal_js.h"
#include "../../assets/config_portal_success_html.h"
#include <ArduinoJson.h>

void WebPlatform::configPortalJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setContent(FPSTR(CONFIG_PORTAL_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::configPortalPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/");

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

  res.setContent(IWebModule::injectNavigationMenu(html), "text/html");
};

void WebPlatform::configPortalSavePageHandler(WebRequest &req, WebResponse &res) {
  Serial.println("WebPlatform: Received WiFi save request");

  auto params = req.getAllParams();
  String ssid = params["ssid"];
  String password = params["password"];

  Serial.printf("SSID: %s, Password length: %d chars (redacted for security)\n",
                ssid.c_str(), password.length());

  if (ssid.length() > 0) {
    saveWiFiCredentials(ssid, password);

    // Create success page with device restart countdown
    String html = FPSTR(CONFIG_PORTAL_SUCCESS_HTML);
    html.replace("{{DEVICE_NAME}}", deviceName);
    html.replace("{{NETWORK_SSID}}", ssid);

    // Reset and re-save credentials
    EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
    EEPROM.commit();
    saveWiFiCredentials(ssid, password);

    // Verify credentials
    String checkSsid, checkPass;
    bool credentialsValid = loadWiFiCredentials(checkSsid, checkPass);
    Serial.printf("WebPlatform: Credential verification %s - SSID match: %s\n",
                  credentialsValid ? "passed" : "failed",
                  checkSsid == ssid ? "yes" : "no");

    res.setContent(html, "text/html");

    // Schedule restart after response is sent
    Serial.println("WebPlatform: WiFi credentials saved - restarting in "
                   "3 seconds...");
    delay(1000);
    Serial.println("WebPlatform: Restarting in 2 seconds...");
    delay(1000);
    Serial.println("WebPlatform: Restarting in 1 second...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("WebPlatform: No SSID provided, returning to config portal");
    configPortalPageHandler(req, res);
  }
}