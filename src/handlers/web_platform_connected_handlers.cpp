#include "../../assets/connected_home_html.h"
#include "../../assets/home_page_js.h"
#include "../../assets/system_status_html.h"
#include "../../assets/system_status_js.h"
#include "../../assets/wifi_management_html.h"
#include "../../assets/wifi_management_js.h"
#include "../../include/interface/web_module_interface.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

void WebPlatform::wifiManagementJSAssetHandler(WebRequest &req,
                                               WebResponse &res) {
  res.setContent(FPSTR(WIFI_MANAGEMENT_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::systemStatusJSAssetHandler(WebRequest &req,
                                             WebResponse &res) {
  res.setContent(FPSTR(SYSTEM_STATUS_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::homePageJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setContent(FPSTR(HOME_PAGE_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

// Define min and max functions if not already defined (for ESP8266
// compatibility)
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

void WebPlatform::rootPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/");
  res.setContent(FPSTR(CONNECTED_HOME_HTML), "text/html");
}

void WebPlatform::statusPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/status");
  res.setContent(FPSTR(SYSTEM_STATUS_HTML), "text/html");
}

void WebPlatform::wifiPageHandler(WebRequest &req, WebResponse &res) {
  IWebModule::setCurrentPath("/wifi");
  res.setContent(FPSTR(WIFI_MANAGEMENT_HTML), "text/html");
}