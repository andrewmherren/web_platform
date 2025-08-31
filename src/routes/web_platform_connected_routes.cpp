#include "../../include/web_platform.h"
#include "../../include/interface/web_module_interface.h"

void WebPlatform::registerConnectedModeRoutes() {
  Serial.println(
      "WebPlatform: Registering core platform routes with unified system");

  registerRoute(
      "/assets/style.css",
      std::bind(&WebPlatform::styleCSSAssetHandler, this,
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
}