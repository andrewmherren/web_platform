#include "docs/system_api_docs.h"
#include "interface/openapi_types.h"
#include "storage/auth_storage.h"
#include "utilities/json_response_builder.h"
#include "web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::registerConfigPortalRoutes() {
  // Static assets - no authentication required for captive portal
  registerWebRoute("/assets/favicon.svg",
                   std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerWebRoute("/assets/favicon.ico",
                   std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerWebRoute("/assets/style.css",
                   std::bind(&WebPlatform::styleCSSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerWebRoute("/assets/web-platform-style.css",
                   std::bind(&WebPlatform::webPlatformCSSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerWebRoute("/assets/web-platform-utils.js",
                   std::bind(&WebPlatform::webPlatformJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerWebRoute("/assets/wifi.js",
                   std::bind(&WebPlatform::wifiJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // Register static assets first
  registerWebRoute("/assets/config-portal-success.js",
                   std::bind(&WebPlatform::configPortalSuccessJSAssetHandler,
                             this, std::placeholders::_1,
                             std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // Initial setup route (if admin password not set)
  registerWebRoute("/setup",
                   std::bind(&WebPlatform::initialSetupPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // Captive portal catch-all routes for common detection URLs
  registerWebRoute("/",
                   std::bind(&WebPlatform::configPortalPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // Register at multiple paths to ensure captive portal works
  registerWebRoute("/portal",
                   std::bind(&WebPlatform::configPortalPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerApiRoute("/wifi",
                   std::bind(&WebPlatform::wifiConfigHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_POST,
                   API_DOC_BLOCK(SystemApiDocs::createConfigureWifi()));

  // API endpoints - no authentication required in captive portal mode
  registerApiRoute("/status",
                   std::bind(&WebPlatform::statusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createGetStatus()));

  registerApiRoute("/scan",
                   std::bind(&WebPlatform::scanApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createScanWifi()));

  registerApiRoute("/reset",
                   std::bind(&WebPlatform::resetApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_POST,
                   API_DOC_BLOCK(SystemApiDocs::createResetDevice()));

  // User creation route for initial setup (reuses existing handler with
  // wrapper)
  registerApiRoute(
      "/user",
      [this](WebRequest &req, WebResponse &res) {
        // Only allow user creation if no users exist (initial setup)
        if (AuthStorage::hasUsers()) {
          JsonResponseBuilder::createErrorResponse(
              res, "User creation not allowed - users already exist", 403);
          return;
        }
        // Forward to the normal user creation handler
        this->createUserApiHandler(req, res);
      },
      {AuthType::PAGE_TOKEN}, WebModule::WM_POST,
      API_DOC("Create first user account",
              "Creates the first user account with admin privileges during "
              "initial setup. Only works when no users exist."));
}