#include "../../assets/account_page_html.h"
#include "../../assets/login_page_html.h"
#include "auth/auth_constants.h"
#include "interface/auth_types.h"
#include "storage/auth_storage.h"
#include "web_platform.h"
#include <functional>

#if OPENAPI_ENABLED
#include "docs/auth_api_docs.h"
#endif


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
  registerApiRoute("/users",
                   std::bind(&WebPlatform::getUsersApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET
#if OPENAPI_ENABLED
                   ,AuthApiDocs::createListUsers()
#endif
                    );

  // Create new user (admin only)
  registerApiRoute("/users",
                   std::bind(&WebPlatform::createUserApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_POST
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createCreateUser()
#endif
                );

  // Get specific user by ID
  registerApiRoute("/users/{id}",
                   std::bind(&WebPlatform::getUserByIdApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createGetUserById()
#endif
                );

  // Update specific user by ID
  registerApiRoute("/users/{id}",
                   std::bind(&WebPlatform::updateUserByIdApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_PUT
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createUpdateUserById()
#endif
                );

  // Delete specific user by ID (admin only)
  registerApiRoute("/users/{id}",
                   std::bind(&WebPlatform::deleteUserByIdApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_DELETE
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createDeleteUserById()
#endif
                );

  // Current user convenience endpoints

  // Get current user
  registerApiRoute("/user",
                   std::bind(&WebPlatform::getCurrentUserApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createGetCurrentUser()
#endif
                    );

  // Update current user
  registerApiRoute("/user",
                   std::bind(&WebPlatform::updateCurrentUserApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_PUT
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createUpdateCurrentUser()
#endif
                    );

  // Token management endpoints

  // Get user's tokens
  registerApiRoute("/users/{id}/tokens",
                   std::bind(&WebPlatform::getUserTokensApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_GET
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createGetUserTokens()
#endif
                    );

  // Create token for user
  registerApiRoute("/users/{id}/tokens",
                   std::bind(&WebPlatform::createUserTokenApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_POST
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createCreateUserToken()
#endif
                    );

  // Delete specific token
  registerApiRoute("/tokens/{id}",
                   std::bind(&WebPlatform::deleteTokenApiHandler, this,
                             std::placeholders::_1, std::placeholders::_2),
                   {{AuthType::TOKEN, AuthType::SESSION}}, WebModule::WM_DELETE
#if OPENAPI_ENABLED
                    ,AuthApiDocs::createDeleteToken()
#endif
                    );
}