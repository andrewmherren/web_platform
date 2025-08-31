#include "../web_platform.h"
#include <web_module_interface.h>

void WebPlatform::registerConnectedModeRoutes() {
  Serial.println(
      "WebPlatform: Registering core platform routes with unified system");

  // Home page
  this->registerRoute("/",
                      std::bind(&WebPlatform::rootPageHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::NONE}, WebModule::WM_GET);

  // System status
  this->registerRoute("/status",
                      std::bind(&WebPlatform::statusPageHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::NONE}, WebModule::WM_GET);

  // WiFi management page
  this->registerRoute("/wifi",
                      std::bind(&WebPlatform::wifiPageHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::NONE}, WebModule::WM_GET);

  this->registerRoute("/api/scan",
                      std::bind(&WebPlatform::scanApiHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  this->registerRoute("/api/status",
                      std::bind(&WebPlatform::statusApiHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  this->registerRoute("/api/connect",
                      std::bind(&WebPlatform::connectApiHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::PAGE_TOKEN}, WebModule::WM_POST);

  this->registerRoute("/api/reset",
                      std::bind(&WebPlatform::resetApiHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      {AuthType::PAGE_TOKEN}, WebModule::WM_POST);
}