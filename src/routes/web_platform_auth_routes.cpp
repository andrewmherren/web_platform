#include "../../assets/account_page_html.h"
#include "../../assets/login_page_html.h"
#include "../../include/auth/auth_constants.h"
#include "../../include/auth/auth_storage.h"
#include "../../include/web_platform.h"
#include "../../include/interface/auth_types.h"
#include <functional>

// Register authentication-related routes
void WebPlatform::registerAuthRoutes() {

  registerRoute(
      "/assets/style.css",
      std::bind(&WebPlatform::styleCSSAssetHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
      {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // Login page - accessible without auth
  registerRoute("/login",
                std::bind(&WebPlatform::loginPageHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::LOCAL_ONLY});

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

  // API endpoints for account management
  registerRoute("/api/account/password",
                std::bind(&WebPlatform::updatePasswordApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION}, WebModule::WM_POST);

  // API endpoints for token management
  registerRoute("/api/tokens/create",
                std::bind(&WebPlatform::createTokenApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION}, WebModule::WM_POST);

  registerRoute("/api/tokens/delete",
                std::bind(&WebPlatform::deleteTokenApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {AuthType::SESSION}, WebModule::WM_POST);
}