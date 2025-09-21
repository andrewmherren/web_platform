#include "docs/system_api_docs.h"
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

  // Register at multiple paths to ensure captive portal works
  registerWebRoute("/portal",
                   std::bind(&WebPlatform::configPortalPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerApiRoute("/wifi",
                   std::bind(&WebPlatform::wifiConfigHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_POST
#if OPENAPI_ENABLED
                  ,SystemApiDocs::createConfigureWifi()
#endif
                  );

  // API endpoints - no authentication required in captive portal mode
  registerApiRoute("/status",
                   std::bind(&WebPlatform::statusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_GET
#if OPENAPI_ENABLED
                  ,SystemApiDocs::createGetStatus()
#endif
                  );

  registerApiRoute("/scan",
                   std::bind(&WebPlatform::scanApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_GET
#if OPENAPI_ENABLED
                  ,SystemApiDocs::createScanWifi()
#endif
                  );

  registerApiRoute("/reset",
                   std::bind(&WebPlatform::resetApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::PAGE_TOKEN}, WebModule::WM_POST
#if OPENAPI_ENABLED
                  ,SystemApiDocs::createResetDevice()
#endif
                  );
}