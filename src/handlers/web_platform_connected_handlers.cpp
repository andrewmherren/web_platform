#include "../../assets/connected_home_html.h"
#include "../../assets/system_status_html.h"
#include "../../assets/wifi_management_html.h"
#include "../../include/auth/auth_storage.h"
#include "../../include/web_platform.h"
#include "../../include/interface/web_module_interface.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// Define min and max functions if not already defined (for ESP8266 compatibility)
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

void WebPlatform::rootPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/");

  String html = FPSTR(CONNECTED_HOME_HTML);

  // Replace template variables with actual values
  html.replace("{{WIFI_SSID}}", WiFi.SSID());
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
  html.replace("{{UPTIME}}", String(millis() / 1000));
  html.replace("{{SERVER_PROTOCOL}}",
               String(httpsEnabled ? "HTTPS (Secure)" : "HTTP"));
  html.replace("{{SERVER_PORT}}", String(serverPort));
  html.replace("{{HOSTNAME}}", getHostname());
  html.replace("{{FREE_MEMORY}}", String(ESP.getFreeHeap() / 1024));

  html = g_platformService->prepareHtml(html, req);

  // Build module list HTML
  String moduleListHtml = "";
  if (registeredModules.size() > 0) {
    for (const auto &module : registeredModules) {
      moduleListHtml += R"(
            <div class="module-item">
                <div>
                    <strong>)" +
                        module.module->getModuleName() + R"(</strong> 
                    <small>v)" +
                        module.module->getModuleVersion() + R"(</small>
                </div>
                <div>
                    <a href=")" +
                        module.basePath +
                        R"(" class="btn btn-secondary">Open</a>
                </div>
            </div>)";
    }
  } else {
    moduleListHtml = "<p>No modules registered.</p>";
  }
  html.replace("{{MODULE_LIST}}", moduleListHtml);

  res.setContent(IWebModule::injectNavigationMenu(html), "text/html");
}

void WebPlatform::statusPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/status");
  String html = FPSTR(SYSTEM_STATUS_HTML);// Replace template variables with actual values
  html.replace("{{UPTIME}}", String(millis() / 1000));// Memory information with gauge - cross-platform compatible
  uint32_t freeHeap = ESP.getFreeHeap();
  int freeHeapPercent = 0;
  
#if defined(ESP32)
  // ESP32 has getHeapSize() function
  uint32_t totalHeap = ESP.getHeapSize();
  if (totalHeap > 0) {
    freeHeapPercent = (int)((float)freeHeap / totalHeap * 100.0);
  }
#else
  // ESP8266 doesn't have direct totalHeap - use approximation
  // Typical ESP8266 has ~80KB of RAM total
  uint32_t estimatedTotalHeap = 80 * 1024; // 80KB approximation for ESP8266
  freeHeapPercent = (int)((float)freeHeap / estimatedTotalHeap * 100.0);
#endif
  
  // Cap percentage to 0-100 range to ensure proper display
  freeHeapPercent = min(100, max(0, freeHeapPercent));
  
  html.replace("{{FREE_HEAP}}", String(freeHeap));
  html.replace("{{FREE_HEAP_PERCENT}}", String(freeHeapPercent));
  
  // Determine color based on available memory (higher % is better for free memory)
  String heapColor = "good";
  if (freeHeapPercent < 20) {
    heapColor = "danger";
  } else if (freeHeapPercent < 40) {
    heapColor = "warning";
  }
  html.replace("{{FREE_HEAP_COLOR}}", heapColor);// Storage information with gauge - cross-platform compatible
  uint32_t flashSize = ESP.getFlashChipSize() / (1024 * 1024); // MB
  uint32_t sketchSize = 0;
  
#if defined(ESP32)
  sketchSize = ESP.getSketchSize(); // ESP32 function
#else
  sketchSize = ESP.getSketchSize(); // Also available on ESP8266
#endif

  uint32_t usedSpace = sketchSize / (1024 * 1024); // Convert to MB
  // Ensure we don't calculate negative available space
  uint32_t availableSpace = (flashSize > usedSpace) ? (flashSize - usedSpace) : 0;
  int usedSpacePercent = (flashSize > 0) ? (int)((float)usedSpace / flashSize * 100.0) : 0;
  
  // Cap percentage to 0-100 range to ensure proper display
  usedSpacePercent = min(100, max(0, usedSpacePercent));
  
  html.replace("{{FLASH_SIZE}}", String(flashSize));
  html.replace("{{USED_SPACE}}", String(usedSpace));
  html.replace("{{AVAILABLE_SPACE}}", String(availableSpace));
  html.replace("{{USED_SPACE_PERCENT}}", String(usedSpacePercent));
  
  // Determine color based on used space (lower % is better for used space)
  String spaceColor = "good";
  if (usedSpacePercent > 80) {
    spaceColor = "danger";
  } else if (usedSpacePercent > 60) {
    spaceColor = "warning";
  }
  html.replace("{{USED_SPACE_COLOR}}", spaceColor);
  
  html.replace(
      "{{PLATFORM_MODE}}",
      String(currentMode == CONNECTED ? "Connected" : "Config Portal"));
  html.replace("{{WIFI_SSID}}", WiFi.SSID());
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{HOSTNAME}}", getHostname());
  html.replace("{{MAC_ADDRESS}}", WiFi.macAddress());
  html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
  html.replace("{{SERVER_PORT}}", String(serverPort));
  html.replace("{{HTTPS_STATUS}}",
               String(httpsEnabled ? "Enabled" : "Disabled"));
  html.replace("{{MODULE_COUNT}}", String(registeredModules.size()));
  html.replace("{{ROUTE_COUNT}}", String(getRouteCount()));

  html = g_platformService->prepareHtml(html, req);

  // Build module table HTML
  String moduleTableHtml = "";
  for (const auto &module : registeredModules) {
    moduleTableHtml += R"(
                <tr>
                    <td>)" +
                       module.module->getModuleName() + R"(</td>
                    <td>)" +
                       module.module->getModuleVersion() + R"(</td>
                    <td>)" +
                       module.basePath + R"(</td>
                </tr>)";
  }
  html.replace("{{MODULE_TABLE}}", moduleTableHtml);

  res.setContent(IWebModule::injectNavigationMenu(html), "text/html");
}

void WebPlatform::wifiPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/wifi");

  String html = FPSTR(WIFI_MANAGEMENT_HTML);

  // Replace template variables with actual values
  html.replace("{{CURRENT_SSID}}", WiFi.SSID());
  html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{MAC_ADDRESS}}", WiFi.macAddress());

  html = g_platformService->prepareHtml(html, req);

  res.setContent(IWebModule::injectNavigationMenu(html), "text/html");
}

void WebPlatform::connectApiHandler(WebRequest &req, WebResponse &res) {
  String ssid = req.getParam("ssid");
  String password = req.getParam("password");

  if (ssid.length() > 0) {
    saveWiFiCredentials(ssid, password);

    res.setContent("{\"status\": \"restarting\", \"message\": \"Connecting "
                   "to new network...\"}",
                   "application/json");

    // Schedule restart after response is sent
    // Note: This is a simplified approach - in a real implementation you
    // might want to use a timer
    delay(100); // Give time for response to be sent
    ESP.restart();
  } else {
    res.setStatus(400);
    res.setContent(
        "{\"status\": \"error\", \"message\": \"Invalid SSID provided\"}",
        "application/json");
  }
};