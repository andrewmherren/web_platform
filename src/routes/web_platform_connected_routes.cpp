#include "docs/system_api_docs.h"
#include "web_platform.h"
#include <interface/openapi_types.h>
#include <interface/web_module_interface.h>

void WebPlatform::registerConnectedModeRoutes() {
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

  registerWebRoute("/assets/system-status.js",
                   std::bind(&WebPlatform::systemStatusJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  registerWebRoute("/assets/home-page.js",
                   std::bind(&WebPlatform::homePageJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // Home page
  registerWebRoute("/",
                   std::bind(&WebPlatform::rootPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // System status
  registerWebRoute("/status",
                   std::bind(&WebPlatform::statusPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::NONE}, WebModule::WM_GET);

  // WiFi management page
  registerWebRoute("/wifi",
                   std::bind(&WebPlatform::wifiPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

#if OPENAPI_ENABLED
  // OpenAPI specification endpoints - cached & fresh versions
  registerWebRoute("/openapi.json",
                   std::bind(&WebPlatform::getOpenAPISpecHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::NONE}}, // No auth required for API docs
                   WebModule::WM_GET);
#endif

#if MAKERAPI_ENABLED
  // OpenAPI specification endpoints - cached & fresh versions
  registerWebRoute("/maker/openapi.json",
                   std::bind(&WebPlatform::getMakerAPISpecHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::NONE}}, // No auth required for API docs
                   WebModule::WM_GET);
#endif

  registerApiRoute("/scan",
                   std::bind(&WebPlatform::scanApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createScanWifi()));

  registerApiRoute("/status",
                   std::bind(&WebPlatform::statusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createGetStatus()));

  registerApiRoute("/reset",
                   std::bind(&WebPlatform::resetApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_POST,
                   API_DOC_BLOCK(SystemApiDocs::createResetDevice()));

  registerApiRoute("/wifi",
                   std::bind(&WebPlatform::wifiConfigHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_POST,
                   API_DOC_BLOCK(SystemApiDocs::createConfigureWifi()));

  // Register RESTful API routes for system status data
  registerApiRoute("/system",
                   std::bind(&WebPlatform::getSystemStatusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createGetSystemStatus()));

  registerApiRoute("/network",
                   std::bind(&WebPlatform::getNetworkStatusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createGetNetworkStatus()));

  registerApiRoute("/modules",
                   std::bind(&WebPlatform::getModulesApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET,
                   API_DOC_BLOCK(SystemApiDocs::createGetModules()));
}