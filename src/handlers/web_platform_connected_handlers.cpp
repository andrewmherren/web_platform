#include "../../assets/connected_home_html.h"
#include "../../assets/home_page_js.h"
#include "../../assets/system_status_html.h"
#include "../../assets/system_status_js.h"
#include "../../assets/wifi_management_html.h"
#include "../../include/interface/web_module_interface.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"

#include <WebServer.h>

void WebPlatform::systemStatusJSAssetHandler(WebRequest &req,
                                             WebResponse &res) {
  res.setProgmemContent(SYSTEM_STATUS_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::homePageJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(HOME_PAGE_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::rootPageHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(CONNECTED_HOME_HTML, "text/html");
}

void WebPlatform::statusPageHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(SYSTEM_STATUS_HTML, "text/html");
}

void WebPlatform::wifiPageHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(WIFI_MANAGEMENT_HTML, "text/html");
}