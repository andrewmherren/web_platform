#include "../../include/interface/web_module_interface.h"
#include "../../include/web_platform.h"

void WebPlatform::registerConnectedModeRoutes() {
  registerRoute("/assets/favicon.svg",
                std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/assets/favicon.ico",
                std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/assets/style.css",
                std::bind(&WebPlatform::styleCSSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/assets/web-platform-style.css",
                std::bind(&WebPlatform::webPlatformCSSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/assets/web-platform-utils.js",
                std::bind(&WebPlatform::webPlatformJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);
  registerRoute("/assets/wifi-management.js",
                std::bind(&WebPlatform::wifiManagementJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/assets/system-status.js",
                std::bind(&WebPlatform::systemStatusJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/assets/home-page.js",
                std::bind(&WebPlatform::homePageJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Home page
  registerRoute("/",
                std::bind(&WebPlatform::rootPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // System status
  registerRoute("/status",
                std::bind(&WebPlatform::statusPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // WiFi management page
  registerRoute("/wifi",
                std::bind(&WebPlatform::wifiPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/api/scan",
                std::bind(&WebPlatform::scanApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  registerRoute("/api/status",
                std::bind(&WebPlatform::statusApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  registerRoute("/api/connect",
                std::bind(&WebPlatform::connectApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_POST);
  registerRoute("/api/reset",
                std::bind(&WebPlatform::resetApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_POST);

  // Register RESTful API routes for system status data
  registerRoute("/api/system",
                std::bind(&WebPlatform::getSystemStatusApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::SESSION}}, WebModule::WM_GET);
  registerRoute("/api/network",
                std::bind(&WebPlatform::getNetworkStatusApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::SESSION}}, WebModule::WM_GET);
  registerRoute("/api/modules",
                std::bind(&WebPlatform::getModulesApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::SESSION}}, WebModule::WM_GET);
}