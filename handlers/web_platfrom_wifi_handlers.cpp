#include "../web_platform.h"
#include "../assets/wifi_management_js.h"
#include <web_module_interface.h>
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

  res.setContent(response, "application/json");
}

void WebPlatform::statusApiHandler(WebRequest &req, WebResponse &res) {
  
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

  res.setContent(response, "application/json");
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

void WebPlatform::wifiManagementJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setContent(FPSTR(WIFI_MANAGEMENT_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}