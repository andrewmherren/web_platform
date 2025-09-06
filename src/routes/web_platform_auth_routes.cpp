#include "../../assets/account_page_html.h"
#include "../../assets/login_page_html.h"
#include "../../include/auth/auth_constants.h"
#include "../../include/interface/auth_types.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"
#include <functional>

// Register authentication-related routes
void WebPlatform::registerAuthRoutes() { // Login page - accessible without auth
                                         // (both GET and POST handlers)
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
  registerRoute(
      "/account",
      std::bind(&WebPlatform::accountPageHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {AuthType::SESSION}); // RESTful API endpoints for user management

  // List all users (admin only)
  registerRoute("/api/users",
                std::bind(&WebPlatform::getUsersApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_GET);

  // Create new user (admin only)
  registerRoute("/api/users",
                std::bind(&WebPlatform::createUserApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_POST);

  // Get specific user by ID
  registerRoute("/api/users/{id}",
                std::bind(&WebPlatform::getUserByIdApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_GET);

  // Update specific user by ID
  registerRoute("/api/users/{id}",
                std::bind(&WebPlatform::updateUserByIdApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_PUT);

  // Delete specific user by ID (admin only)
  registerRoute("/api/users/{id}",
                std::bind(&WebPlatform::deleteUserByIdApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_DELETE);

  // Current user convenience endpoints

  // Get current user
  registerRoute("/api/user",
                std::bind(&WebPlatform::getCurrentUserApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_GET); // Update current user
  registerRoute("/api/user",
                std::bind(&WebPlatform::updateCurrentUserApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_PUT);

  // Token management endpoints

  // Get user's tokens
  registerRoute("/api/users/{id}/tokens",
                std::bind(&WebPlatform::getUserTokensApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_GET);

  // Create token for user
  registerRoute("/api/users/{id}/tokens",
                std::bind(&WebPlatform::createUserTokenApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_POST); // Delete specific token
  registerRoute("/api/tokens/{id}",
                std::bind(&WebPlatform::deleteTokenApiHandler, this,
                          std::placeholders::_1, std::placeholders::_2),
                {{AuthType::PAGE_TOKEN, AuthType::SESSION, AuthType::TOKEN}},
                WebModule::WM_DELETE);
}