
#include "../../assets/config_portal_html.h"
#include "../../assets/config_portal_js.h"
#include "../../assets/config_portal_success_html.h"
#include "../../include/web_platform.h"
#include <ArduinoJson.h>


void WebPlatform::configPortalJSAssetHandler(WebRequest &req,
                                             WebResponse &res) {
  res.setContent(FPSTR(CONFIG_PORTAL_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::configPortalPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/");
  res.setContent(FPSTR(CONFIG_PORTAL_HTML), "text/html");
};

void WebPlatform::configPortalSavePageHandler(WebRequest &req,
                                              WebResponse &res) {
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