#include "interface/web_module_interface.h"
#include "web_platform.h"

#if OPENAPI_ENABLED
#include "docs/system_api_docs.h"
#endif

void WebPlatform::registerConnectedModeRoutes() {
  registerWebRoute("/assets/favicon.svg",
                   std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/favicon.ico",
                   std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/style.css",
                   std::bind(&WebPlatform::styleCSSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/web-platform-style.css",
                   std::bind(&WebPlatform::webPlatformCSSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/web-platform-utils.js",
                   std::bind(&WebPlatform::webPlatformJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/wifi.js",
                   std::bind(&WebPlatform::wifiJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/system-status.js",
                   std::bind(&WebPlatform::systemStatusJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/assets/home-page.js",
                   std::bind(&WebPlatform::homePageJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Home page
  registerWebRoute("/",
                   std::bind(&WebPlatform::rootPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // System status
  registerWebRoute("/status",
                   std::bind(&WebPlatform::statusPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // WiFi management page
  registerWebRoute("/wifi",
                   std::bind(&WebPlatform::wifiPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerApiRoute("/scan",
                   std::bind(&WebPlatform::scanApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET
#if OPENAPI_ENABLED
                  , SystemApiDocs::createScanWifi()
#endif
                  );

  registerApiRoute("/status",
                   std::bind(&WebPlatform::statusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET
#if OPENAPI_ENABLED
                  , SystemApiDocs::createGetStatus()
#endif
                  );

  registerApiRoute("/reset",
                   std::bind(&WebPlatform::resetApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_POST
#if OPENAPI_ENABLED
                  , SystemApiDocs::createResetDevice()
#endif
                  );

  registerApiRoute("/wifi",
                   std::bind(&WebPlatform::wifiConfigHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_POST
#if OPENAPI_ENABLED
                  , SystemApiDocs::createConfigureWifi()
#endif
                  );

  // Register RESTful API routes for system status data
  registerApiRoute("/system",
                   std::bind(&WebPlatform::getSystemStatusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET
#if OPENAPI_ENABLED
                  , SystemApiDocs::createGetSystemStatus()
#endif
                  );

  registerApiRoute("/network",
                   std::bind(&WebPlatform::getNetworkStatusApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET
#if OPENAPI_ENABLED
                  , SystemApiDocs::createGetNetworkStatus()
#endif
                  );

  registerApiRoute("/modules",
                   std::bind(&WebPlatform::getModulesApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET
#if OPENAPI_ENABLED
                  , SystemApiDocs::createGetModules()
#endif
                  );

  #if OPENAPI_ENABLED
  // OpenAPI specification endpoints - cached & fresh versions
  registerApiRoute("/openapi.json",
                   std::bind(&WebPlatform::getOpenAPISpecHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::NONE}}, // No auth required for API docs
                   WebModule::WM_GET
#if OPENAPI_ENABLED
                  , SystemApiDocs::createGetOpenAPISpec()
#endif
                  );
  #endif
}