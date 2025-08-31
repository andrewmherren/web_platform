#include "../../include/web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::registerConfigPortalRoutes() {
  Serial.println("WebPlatform: Registering config portal routes using unified "
                 "system"); // Main configuration page - register at both / and

  registerRoute(
      "/assets/style.css",
      std::bind(&WebPlatform::styleCSSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
      {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Register static assets first
  registerRoute("/assets/config-portal.js",
                std::bind(&WebPlatform::configPortalJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Register at multiple paths to ensure captive portal works
  registerRoute("/",
                std::bind(&WebPlatform::configPortalPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/index.html",
                std::bind(&WebPlatform::configPortalPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Save WiFi credentials
  registerRoute("/save",
                std::bind(&WebPlatform::configPortalSavePageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_POST);

  // API endpoints
  registerRoute(
      "/api/status",
      std::bind(&WebPlatform::statusApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
      {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  registerRoute(
      "/api/scan",
      std::bind(&WebPlatform::scanApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
      {AuthType::PAGE_TOKEN}, WebModule::WM_GET);

  registerRoute(
      "/api/reset",
      std::bind(&WebPlatform::resetApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
      {AuthType::PAGE_TOKEN}, WebModule::WM_POST);
}