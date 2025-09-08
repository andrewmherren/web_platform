#include "../../include/web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::registerConfigPortalRoutes() {
  // Static assets - no authentication required for captive portal
  registerRoute("/assets/favicon.svg",
                std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/assets/favicon.ico",
                std::bind(&WebPlatform::webPlatformFaviconHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/assets/style.css",
                std::bind(&WebPlatform::styleCSSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/assets/web-platform-style.css",
                std::bind(&WebPlatform::webPlatformCSSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/assets/web-platform-utils.js",
                std::bind(&WebPlatform::webPlatformJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/assets/wifi.js",
                std::bind(&WebPlatform::wifiJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  // Register static assets first
  registerRoute("/assets/config-portal-success.js",
                std::bind(&WebPlatform::configPortalSuccessJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  // Register at multiple paths to ensure captive portal works
  registerRoute("/portal",
                std::bind(&WebPlatform::configPortalPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/api/wifi",
                std::bind(&WebPlatform::wifiConfigHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_POST);

  // API endpoints - no authentication required in captive portal mode
  registerRoute("/api/status",
                std::bind(&WebPlatform::statusApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  registerRoute("/api/scan",
                std::bind(&WebPlatform::scanApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  registerRoute("/api/reset",
                std::bind(&WebPlatform::resetApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::PAGE_TOKEN}, WebModule::WM_POST);
}