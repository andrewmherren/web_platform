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

  registerRoute("/assets/web-platform-utils.js",
                std::bind(&WebPlatform::webPlatformJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/assets/config-portal.js",
                std::bind(&WebPlatform::configPortalJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  // Register static assets first
  registerRoute("/assets/config-portal.js",
                std::bind(&WebPlatform::configPortalJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  // Register at multiple paths to ensure captive portal works
  registerRoute("/",
                std::bind(&WebPlatform::configPortalPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  registerRoute("/index.html",
                std::bind(&WebPlatform::configPortalPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_GET);

  // Save WiFi credentials - no auth required for initial setup
  registerRoute("/save",
                std::bind(&WebPlatform::configPortalSavePageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::NONE}, WebModule::WM_POST);
  // TODO: /save should be an api route with page_token protection

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