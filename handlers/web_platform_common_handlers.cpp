#include "../web_platform.h"
#include "../assets/web_platform_styles_css.h"
#include "../assets/web_platform_utils_js.h"

void WebPlatform::webPlatformCSSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setContent(FPSTR(WEB_PLATFORM_STYLES_CSS), "text/css");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::webPlatformJSAssetHandler(WebRequest &req, WebResponse &res) {
  res.setContent(FPSTR(WEB_PLATFORM_UTILS_JS), "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}