#include "../../assets/config_portal_html.h"
#include "../../assets/config_portal_success_html.h"
#include "../../assets/config_portal_success_js.h"
#include "utilities/json_response_builder.h"
#include "web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::configPortalPageHandler(WebRequest &req, WebResponse &res) {
  res.setProgmemContent(CONFIG_PORTAL_HTML, "text/html");
};

void WebPlatform::configPortalSuccessJSAssetHandler(WebRequest &req,
                                                    WebResponse &res) {
  res.setProgmemContent(CONFIG_PORTAL_SUCCESS_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}