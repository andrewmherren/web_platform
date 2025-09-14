#include "../../assets/account_page_html.h"
#include "../../assets/login_page_html.h"
#include "../../include/auth/auth_constants.h"
#include "../../include/interface/auth_types.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"
#include <functional>

// In general, auth api routes should use Token and Session auth types because
// only those two identify the actual user preforming interactions and we dont
// wan to allow anonymous interaction with auth related resources (ie users,
// tokens, etc)

// Register authentication-related routes
void WebPlatform::registerAuthRoutes() { // Login page - accessible without auth
                                         // (both GET and POST handlers)
  registerWebRoute("/assets/account-page.js",
                   std::bind(&WebPlatform::accountPageJSAssetHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  registerWebRoute("/login",
                   std::bind(&WebPlatform::loginPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_GET);

  // POST handler for login form submission
  registerApiRoute("/login",
                   std::bind(&WebPlatform::loginApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY}, WebModule::WM_POST);

  // Logout endpoint
  registerWebRoute("/logout",
                   std::bind(&WebPlatform::logoutPageHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {AuthType::LOCAL_ONLY});

  // User account page - requires session authentication
  registerWebRoute(
      "/account",
      std::bind(&WebPlatform::accountPageHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {AuthType::SESSION}); // RESTful API endpoints for user management

  // List all users (admin only)
  registerApiRoute(
      "/users",
      std::bind(&WebPlatform::getUsersApiHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET,
      OpenAPIDocumentation("List all users",
                           "Retrieves all user accounts (admin only)",
                           "listUsers", {"User Management"}));

  // Create new user (admin only)
  registerApiRoute(
      "/users",
      std::bind(&WebPlatform::createUserApiHandler, this, std::placeholders::_1,
                std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_POST,
      OpenAPIDocumentation("Create a new user",
                           "Creates a new user account (admin only)",
                           "createUser", {"User Management"}));

  // Get specific user by ID
  registerApiRoute("/users/{id}",
                   std::bind(&WebPlatform::getUserByIdApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET,
                   OpenAPIDocumentation("Get user by ID",
                                        "Retrieves a specific user by their ID",
                                        "getUserById", {"User Management"}));

  // Update specific user by ID
  registerApiRoute("/users/{id}",
                   std::bind(&WebPlatform::updateUserByIdApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_PUT,
                   OpenAPIDocumentation("Update user by ID",
                                        "Updates a specific user by their ID",
                                        "updateUserById", {"User Management"}));

  // Delete specific user by ID (admin only)
  registerApiRoute(
      "/users/{id}",
      std::bind(&WebPlatform::deleteUserByIdApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_DELETE,
      OpenAPIDocumentation("Delete user by ID",
                           "Removes a specific user account by ID (admin only)",
                           "deleteUserById", {"User Management"}));

  // Current user convenience endpoints

  // Get current user
  registerApiRoute(
      "/user",
      std::bind(&WebPlatform::getCurrentUserApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET,
      OpenAPIDocumentation(
          "Get current user",
          "Retrieves information about the currently authenticated user",
          "getCurrentUser", {"User Management"}));

  // Update current user
  registerApiRoute(
      "/user",
      std::bind(&WebPlatform::updateCurrentUserApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_PUT,
      OpenAPIDocumentation(
          "Update current user",
          "Updates information for the currently authenticated user",
          "updateCurrentUser", {"User Management"}));

  // Token management endpoints

  // Get user's tokens
  registerApiRoute(
      "/users/{id}/tokens",
      std::bind(&WebPlatform::getUserTokensApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET,
      OpenAPIDocumentation("Get user tokens",
                           "Retrieves all API tokens for a specific user",
                           "getUserTokens", {"Token Management"}));

  // Create token for user
  registerApiRoute(
      "/users/{id}/tokens",
      std::bind(&WebPlatform::createUserTokenApiHandler, this,
                std::placeholders::_1, std::placeholders::_2),
      {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_POST,
      OpenAPIDocumentation("Create user token",
                           "Creates a new API token for a specific user",
                           "createUserToken", {"Token Management"}));

  // Delete specific token
  registerApiRoute("/tokens/{id}",
                   std::bind(&WebPlatform::deleteTokenApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_DELETE,
                   OpenAPIDocumentation("Delete token",
                                        "Removes a specific API token by ID",
                                        "deleteToken", {"Token Management"}));
}