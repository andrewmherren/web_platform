#include "../../assets/config_portal_html.h"
#include "../../assets/config_portal_success_html.h"
#include "../../assets/config_portal_success_js.h"
#include "../../assets/initial_setup_html.h"
#include "storage/auth_storage.h"
#include "utilities/json_response_builder.h"
#include "web_platform.h"
#include <ArduinoJson.h>

void WebPlatform::configPortalPageHandler(WebRequest &req, WebResponse &res) {
  // Check if initial setup is needed first
  if (AuthStorage::requiresInitialSetup()) {
    // Redirect to initial setup page
    res.redirect("/setup");
    return;
  }

  res.setProgmemContent(CONFIG_PORTAL_HTML, "text/html");
};

void WebPlatform::configPortalSuccessJSAssetHandler(WebRequest &req,
                                                    WebResponse &res) {
  res.setProgmemContent(CONFIG_PORTAL_SUCCESS_JS, "application/javascript");
  res.setHeader("Cache-Control", "public, max-age=3600");
}

void WebPlatform::initialSetupPageHandler(WebRequest &req, WebResponse &res) {
  // Check if initial setup is actually needed
  if (!AuthStorage::requiresInitialSetup()) {
    // Initial setup is not needed, redirect to portal/login
    res.redirect("/portal");
    return;
  }

  res.setProgmemContent(INITIAL_SETUP_HTML, "text/html");
}

void WebPlatform::initialSetupHandler(WebRequest &req, WebResponse &res) {
  // Check if initial setup is actually needed
  if (!AuthStorage::requiresInitialSetup()) {
    JsonResponseBuilder::createErrorResponse(res, "Initial setup not required",
                                             400);
    return;
  }

  String password = req.getParam("password");
  String confirmPassword = req.getParam("confirmPassword");

  // Validate password
  if (password.length() == 0) {
    JsonResponseBuilder::createErrorResponse(res, "Password is required", 400);
    return;
  }

  if (password.length() < 6) {
    JsonResponseBuilder::createErrorResponse(
        res, "Password must be at least 6 characters long", 400);
    return;
  }

  if (password != confirmPassword) {
    JsonResponseBuilder::createErrorResponse(res, "Passwords do not match",
                                             400);
    return;
  }

  // Set the initial admin password
  if (AuthStorage::setInitialAdminPassword(password)) {
    // Create a session for the admin user
    AuthUser admin = AuthStorage::findUserByUsername("admin");
    if (admin.isValid()) {
      String sessionId = AuthStorage::createSession(admin.id);
      if (!sessionId.isEmpty()) {
        // Set session cookie
        res.setHeader("Set-Cookie",
                      "session=" + sessionId +
                          "; Path=/; Max-Age=86400; SameSite=Strict; HttpOnly");

        JsonResponseBuilder::createSuccessResponse(
            res, "Initial setup completed successfully");
      } else {
        JsonResponseBuilder::createErrorResponse(
            res, "Failed to create session", 500);
      }
    } else {
      JsonResponseBuilder::createErrorResponse(
          res, "Admin user not found after setup", 500);
    }
  } else {
    JsonResponseBuilder::createErrorResponse(
        res, "Failed to set initial password", 500);
  }
}
