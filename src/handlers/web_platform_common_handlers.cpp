#include "../../assets/favicon_ico.h"
#include "../../assets/web_platform_styles_css.h"
#include "../../assets/web_platform_utils_js.h"
#include "../../assets/web_ui_styles.h"
#include "../../assets/wifi_js.h"
#include "../../include/web_platform.h"

void WebPlatform::webPlatformCSSAssetHandler(WebRequest &req,
                                             WebResponse &res) {
  res.setProgmemContent(WEB_PLATFORM_STYLES_CSS, "text/css");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::webPlatformJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(WEB_PLATFORM_UTILS_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::styleCSSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(WEB_UI_DEFAULT_CSS, "text/css");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::webPlatformFaviconHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(WEB_PLATFORM_FAVICON, "image/svg+xml");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::wifiJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(WIFI_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}