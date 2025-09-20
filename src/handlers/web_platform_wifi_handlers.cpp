#include "../../include/interface/web_module_interface.h"
#include "../../include/utilities/json_response_builder.h"
#include "../../include/web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::scanApiHandler(WebRequest &req, WebResponse &res) {
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
                                WiFi.encryptionType(i) != WIFI_AUTH_OPEN});
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

  // Serialize with memory safety
  String response;
  size_t jsonSize = measureJson(doc);
  response.reserve(jsonSize + 10); // Reserve with small buffer
  serializeJson(doc, response);

  res.setContent(response, "application/json");
  // Document automatically cleaned up when going out of scope
}

void WebPlatform::statusApiHandler(WebRequest &req, WebResponse &res) {
  // Use JsonResponseBuilder for automatic memory management
  JsonResponseBuilder::createResponse<512>(res, [&](JsonObject &json) {
    json["connected"] = (WiFi.status() == WL_CONNECTED);
    json["ssid"] = WiFi.SSID();
    json["ip"] = WiFi.localIP().toString();
    json["rssi"] = WiFi.RSSI();
    json["mode"] = (currentMode == CONFIG_PORTAL) ? "config" : "connected";
    json["https_enabled"] = httpsEnabled;
    json["device_name"] = deviceName;
  });
}

void WebPlatform::resetApiHandler(WebRequest &req, WebResponse &res) {
  // Clear WiFi credentials
  EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
  EEPROM.commit();

  // Schedule restart
  delay(1000);
  ESP.restart();

  res.setContent(R"({"status": "restarting"})", "application/json");
}

void WebPlatform::wifiConfigHandler(WebRequest &req, WebResponse &res) {
  Serial.println("WebPlatform: Received WiFi save API request");

  String ssid = req.getJsonParam("ssid");
  String password = req.getJsonParam("password");

  Serial.printf("SSID: %s, Password length: %d chars (redacted for security)\n",
                ssid.c_str(), password.length());

  if (ssid.length() > 0) {
    // Reset and save credentials
    EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
    EEPROM.commit();
    saveWiFiCredentials(ssid, password);

    // Verify credentials
    String checkSsid, checkPass;
    bool credentialsValid = loadWiFiCredentials(checkSsid, checkPass);
    Serial.printf("WebPlatform: Credential verification %s - SSID match: %s\n",
                  credentialsValid ? "passed" : "failed",
                  checkSsid == ssid ? "yes" : "no");

    // Return success response using JsonResponseBuilder
    JsonResponseBuilder::createResponse<256>(res, [&](JsonObject &json) {
      json["success"] = true;
      json["message"] = "WiFi credentials saved successfully";
      json["ssid"] = ssid;
      json["restart_required"] = true;
    });

    // Schedule restart after response is sent
    Serial.println(
        "WebPlatform: WiFi credentials saved - restarting in 3 seconds...");
    delay(1000);
    Serial.println("WebPlatform: Restarting in 2 seconds...");
    delay(1000);
    Serial.println("WebPlatform: Restarting in 1 second...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("WebPlatform: No SSID provided in API request");

    JsonResponseBuilder::createErrorResponse(res, "SSID is required", 400);
  }
}
