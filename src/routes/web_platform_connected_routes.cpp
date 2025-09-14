#include "../../include/interface/web_module_interface.h"
#include "../../include/web_platform.h"

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

  registerApiRoute(
      "/scan",
      std::bind(&WebPlatform::scanApiHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_GET,
      OpenAPIDocumentation(
          "Scan WiFi networks",
          "Scans for available WiFi networks and returns the results",
          "scanWifi", {"WiFi Management"}));

  registerApiRoute(
      "/status",
      std::bind(&WebPlatform::statusApiHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_GET,
      OpenAPIDocumentation("Get device status",
                           "Returns the current status of the device "
                           "including WiFi connection details",
                           "getStatus", {"System"}));

  registerApiRoute(
      "/reset",
      std::bind(&WebPlatform::resetApiHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_POST,
      OpenAPIDocumentation("Reset device",
                           "Resets the device WiFi configuration and "
                           "restarts in configuration portal mode",
                           "resetDevice", {"System"}));

  registerApiRoute(
      "/wifi",
      std::bind(&WebPlatform::wifiConfigHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_POST,
      OpenAPIDocumentation(
          "Configure WiFi",
          "Updates the device's WiFi configuration with new credentials",
          "configureWifi", {"WiFi Management"}));

  // Register RESTful API routes for system status data
  registerApiRoute(
      "/system",
      std::bind(&WebPlatform::getSystemStatusApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_GET,
      OpenAPIDocumentation("Get system status",
                           "Returns system information including uptime, "
                           "memory usage, and firmware details",
                           "getSystemStatus", {"System"}));

  registerApiRoute(
      "/network",
      std::bind(&WebPlatform::getNetworkStatusApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_GET,
      OpenAPIDocumentation("Get network status",
                           "Returns network status including IP address, "
                           "signal strength, and connection details",
                           "getNetworkStatus", {"Network"}));

  registerApiRoute(
      "/modules",
      std::bind(&WebPlatform::getModulesApiHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
      WebModule::WM_GET,
      OpenAPIDocumentation("Get registered modules",
                           "Returns information about all registered "
                           "web modules and their routes",
                           "getModules", {"System"}));

  // OpenAPI specification endpoint
  registerApiRoute("/openapi.json",
                   std::bind(&WebPlatform::getOpenAPISpecHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::PAGE_TOKEN, AuthType::TOKEN, AuthType::SESSION}},
                   WebModule::WM_GET,
                   OpenAPIDocumentation("Get OpenAPI specification",
                                        "Returns the OpenAPI 3.0 specification "
                                        "for all registered API routes",
                                        "getOpenAPISpec", {"System"}));
}