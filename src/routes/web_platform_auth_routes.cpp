#include "../../assets/account_page_html.h"
#include "../../assets/login_page_html.h"
#include "../../include/auth/auth_constants.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"
#include "../../include/interface/auth_types.h"
#include <functional>

// Register authentication-related routes
void WebPlatform::registerAuthRoutes() {// Login page - accessible without auth (both GET and POST handlers)
  registerRoute("/assets/account-page.js",
                std::bind(&WebPlatform::accountPageJSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerRoute("/login",
                std::bind(&WebPlatform::loginPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_GET);
  
  // POST handler for login form submission
  registerRoute("/login",
                std::bind(&WebPlatform::loginPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY}, WebModule::WM_POST);

  // Logout endpoint
  registerRoute("/logout",
                std::bind(&WebPlatform::logoutPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY});

  // User account page - requires session authentication
  registerRoute("/account",
                std::bind(&WebPlatform::accountPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION});
  
  // API endpoints for user management
  registerRoute("/api/user",
                std::bind(&WebPlatform::updateUserApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION}, WebModule::WM_PUT);

  // API endpoints for token management
  registerRoute("/api/token",
                std::bind(&WebPlatform::createTokenApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION}, WebModule::WM_POST);

  registerRoute("/api/token/{tokenId}",
                std::bind(&WebPlatform::deleteTokenApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION}, WebModule::WM_DELETE);
}