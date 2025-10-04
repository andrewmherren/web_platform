#include "../../assets/config_portal_html.h"
#include "../../assets/config_portal_success_html.h"
#include "../../assets/config_portal_success_js.h"
#include "../../assets/initial_setup_html.h"
#include "storage/auth_storage.h"
#include "utilities/debug_macros.h"
#include "utilities/json_response_builder.h"
#include "web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::configPortalPageHandler(WebRequest &req, WebResponse &res) {
  // Check if initial setup is needed first
  if (AuthStorage::requiresInitialSetup()) {
    // Redirect to initial setup page
    res.redirect("/setup");
    return;
  }

  res.setProgmemContent(CONFIG_PORTAL_HTML, "text/html");
};

void WebPlatform::configPortalSuccessJSAssetHandler(WebRequest &req,
                                                    WebResponse &res) {
  res.setProgmemContent(CONFIG_PORTAL_SUCCESS_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::initialSetupPageHandler(WebRequest &req, WebResponse &res) {
  // Check if initial setup is actually needed
  if (!AuthStorage::requiresInitialSetup()) {
    // Initial setup is not needed, redirect to portal/login
    res.redirect("/portal");
    return;
  }

  res.setProgmemContent(INITIAL_SETUP_HTML, "text/html");
}

// initialSetupHandler removed - now using createUserApiHandler with lambda
// wrapper
