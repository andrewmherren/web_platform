#include "storage/auth_storage.h"
#include "utilities/json_response_builder.h"
#include "web_platform.h"
#include <functional>

// RESTful API Handlers - User Management

void WebPlatform::getUsersApiHandler(WebRequest &req, WebResponse &res) {

  // Check if user is admin
  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);
  if (!currentUser.isValid() || !currentUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Admin access required", 403);
    return;
  }

  std::vector<AuthUser> users = AuthStorage::getAllUsers();

  // Use JsonResponseBuilder with automatic sizing based on user count
  size_t estimatedSize = 256 + (users.size() * 128);
  JsonResponseBuilder::createDynamicResponse(
      res,
      [&](JsonObject &root) {
        root["success"] = true;
        JsonArray usersArray = root.createNestedArray("users");

        for (const auto &user : users) {
          JsonObject userObj = usersArray.createNestedObject();
          userObj["id"] = user.id;
          userObj["username"] = user.username;
          userObj["createdAt"] = user.createdAt;
        }
      },
      estimatedSize);
}

void WebPlatform::createUserApiHandler(WebRequest &req, WebResponse &res) {
  const AuthContext &auth = req.getAuthContext();
  bool isInitialSetup = !AuthStorage::hasUsers();

  // For normal operation (users exist), require admin privileges
  if (!isInitialSetup) {
    // Check if current user is admin (check by actual admin flag, not hardcoded
    // username)
    AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);
    if (!currentUser.isValid() || !currentUser.isAdmin) {
      JsonResponseBuilder::createErrorResponse(res, "Admin access required",
                                               403);
      return;
    }
  }
  // For initial setup (no users exist), anyone with page token can create the
  // first admin user

  // Support both form data and JSON input
  String username = req.getParam("username");
  String password = req.getParam("password");

  // If form data is empty, try JSON
  if (username.isEmpty()) {
    username = req.getJsonParam("username");
  }
  if (password.isEmpty()) {
    password = req.getJsonParam("password");
  }

  if (username.isEmpty() || password.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(
        res, "Username and password required", 400);
    return;
  }

  if (password.length() < 4) {
    JsonResponseBuilder::createErrorResponse(
        res, "Password must be at least 4 characters", 400);
    return;
  }

  // Check if user already exists
  AuthUser existingUser = AuthStorage::findUserByUsername(username);
  if (existingUser.isValid()) {
    JsonResponseBuilder::createErrorResponse(res, "User already exists", 409);
    return;
  }

  // Pass isInitialSetup flag to createUser so first user gets admin privileges
  String userId = AuthStorage::createUser(username, password, isInitialSetup);
  if (userId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "Failed to create user", 500);
    return;
  }

  // Success response with different message for initial setup
  String message = isInitialSetup
                       ? "First user account created with admin privileges"
                       : "User created";
  JsonResponseBuilder::createResponse<256>(res, [&](JsonObject &root) {
    root["success"] = true;
    root["message"] = message;
    root["id"] = userId;
  });
  res.setStatus(201);
}

void WebPlatform::getUserByIdApiHandler(WebRequest &req, WebResponse &res) {

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "User ID required", 400);
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only access their own data unless they are admin
  if (userId != currentUser.id && !currentUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Admin access required", 403);
    return;
  }

  AuthUser user = AuthStorage::findUserById(userId);
  if (!user.isValid()) {
    JsonResponseBuilder::createErrorResponse(res, "User not found", 404);
    return;
  }

  size_t estimatedSize = 384;
  JsonResponseBuilder::createDynamicResponse(
      res,
      [&](JsonObject &root) {
        root["success"] = true;
        JsonObject userObj = root.createNestedObject("user");
        userObj["id"] = user.id;
        userObj["username"] = user.username;
        userObj["createdAt"] = user.createdAt;
      },
      estimatedSize);
}

void WebPlatform::updateUserByIdApiHandler(WebRequest &req, WebResponse &res) {
  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "User ID required", 400);
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only update their own data unless they are admin
  if (userId != currentUser.id && !currentUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Admin access required", 403);
    return;
  }

  String password = req.getJsonParam("password");
  if (password.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "Password is required", 400);
    return;
  }

  if (password.length() < 4) {
    JsonResponseBuilder::createErrorResponse(
        res, "Password must be at least 4 characters", 400);
    return;
  }

  bool success = AuthStorage::updateUserPassword(userId, password);

  if (success) {
    JsonResponseBuilder::createResponse<256>(res, [&](JsonObject &root) {
      root["success"] = true;
      root["message"] = "User updated";
    });
    res.setContent("{\"success\":true,\"message\":\"User updated\"}");
  } else {
    JsonResponseBuilder::createErrorResponse(res, "Failed to update user", 500);
  }
}

void WebPlatform::deleteUserByIdApiHandler(WebRequest &req, WebResponse &res) {
  // Check if user is admin
  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);
  if (!currentUser.isValid() || !currentUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Admin access required", 403);
    return;
  }

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "User ID required", 400);
    return;
  }

  // Don't allow deleting admin users (any user with admin privileges)
  AuthUser targetUser = AuthStorage::findUserById(userId);
  if (targetUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Cannot delete admin user",
                                             403);
    return;
  }

  bool success = AuthStorage::deleteUser(userId);

  if (success) {
    JsonResponseBuilder::createResponse<256>(res, [&](JsonObject &root) {
      root["success"] = true;
      root["message"] = "User deleted";
    });
  } else {
    JsonResponseBuilder::createErrorResponse(res, "Failed to delete user", 500);
  }
}

// Current User Convenience Handlers

void WebPlatform::getCurrentUserApiHandler(WebRequest &req, WebResponse &res) {

  const AuthContext &auth = req.getAuthContext();
  AuthUser user = AuthStorage::findUserByUsername(auth.username);

  if (!user.isValid()) {
    JsonResponseBuilder::createErrorResponse(res, "User nt found", 404);
    return;
  }

  size_t estimatedSize = 384;
  JsonResponseBuilder::createDynamicResponse(
      res,
      [&](JsonObject &root) {
        root["success"] = true;
        JsonObject userObj = root.createNestedObject("user");
        userObj["id"] = user.id;
        userObj["username"] = user.username;
        userObj["createdAt"] = user.createdAt;
      },
      estimatedSize);
}

void WebPlatform::updateCurrentUserApiHandler(WebRequest &req,
                                              WebResponse &res) {
  const AuthContext &auth = req.getAuthContext();
  AuthUser user = AuthStorage::findUserByUsername(auth.username);
  String password = req.getJsonParam("password");

  if (password.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "Password is required", 404);
    return;
  }

  if (password.length() < 4) {
    JsonResponseBuilder::createErrorResponse(
        res, "Password must be at least 4 characters", 404);
    return;
  }

  bool success = AuthStorage::updateUserPassword(user.id, password);

  if (success) {
    JsonResponseBuilder::createResponse<256>(res, [&](JsonObject &root) {
      root["success"] = true;
      root["message"] = "User updated";
    });
  } else {
    JsonResponseBuilder::createErrorResponse(res, "Failed to update user", 500);
  }
}

// Token Management Handlers

void WebPlatform::getUserTokensApiHandler(WebRequest &req, WebResponse &res) {

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "User ID required", 400);
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only access their own tokens unless they are admin
  if (userId != currentUser.id && !currentUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Admin access required", 403);
    return;
  }

  std::vector<AuthApiToken> tokens = AuthStorage::getUserApiTokens(userId);

  // Use JsonResponseBuilder with dynamic sizing based on token count
  size_t estimatedSize = 256 + (tokens.size() * 256);
  JsonResponseBuilder::createDynamicResponse(
      res,
      [&](JsonObject &root) {
        root["success"] = true;
        JsonArray tokensArray = root.createNestedArray("tokens");

        for (const auto &token : tokens) {
          JsonObject tokenObj = tokensArray.createNestedObject();
          tokenObj["id"] = token.id;
          tokenObj["token"] = token.token;
          tokenObj["name"] = token.name;
          tokenObj["createdAt"] = token.createdAt;
          tokenObj["expiresAt"] = token.expiresAt;
        }
      },
      estimatedSize);
}

void WebPlatform::createUserTokenApiHandler(WebRequest &req, WebResponse &res) {
  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "User ID required", 400);
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only create tokens for themselves unless they are admin
  if (userId != currentUser.id && !currentUser.isAdmin) {
    JsonResponseBuilder::createErrorResponse(res, "Admin access required", 403);
    return;
  }

  String tokenName = req.getJsonParam("name");
  if (tokenName.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "Token name is required",
                                             400);
    return;
  }

  String token = AuthStorage::createApiToken(userId, tokenName);
  if (token.isEmpty()) {
    JsonResponseBuilder::createErrorResponse(res, "Failed to create token",
                                             500);
    return;
  }

  JsonResponseBuilder::createResponse<256>(res, [&](JsonObject &root) {
    root["success"] = true;
    root["message"] = "Token created";
    root["token"] = token;
  });
}

// System Status API Endpoints

void WebPlatform::getSystemStatusApiHandler(WebRequest &req, WebResponse &res) {

  // Memory information
  uint32_t freeHeap = ESP.getFreeHeap();
  int freeHeapPercent = 0;

  uint32_t totalHeap = ESP.getHeapSize();
  if (totalHeap > 0) {
    freeHeapPercent = (int)((float)freeHeap / totalHeap * 100.0);
  }

  freeHeapPercent = min(100, max(0, freeHeapPercent));

  String heapColor = "good";
  if (freeHeapPercent < 20) {
    heapColor = "danger";
  } else if (freeHeapPercent < 40) {
    heapColor = "warning";
  }

  // Storage information
  uint32_t flashSize = ESP.getFlashChipSize() / (1024 * 1024);
  uint32_t sketchSize = ESP.getSketchSize();
  uint32_t usedSpace = sketchSize / (1024 * 1024);
  uint32_t availableSpace =
      (flashSize > usedSpace) ? (flashSize - usedSpace) : 0;
  int usedSpacePercent =
      (flashSize > 0) ? (int)((float)usedSpace / flashSize * 100.0) : 0;
  usedSpacePercent = min(100, max(0, usedSpacePercent));

  String spaceColor = "good";
  if (usedSpacePercent > 80) {
    spaceColor = "danger";
  } else if (usedSpacePercent > 60) {
    spaceColor = "warning";
  }

  // Build JSON response with builder
  JsonResponseBuilder::createResponse<1024>(res, [&](JsonObject &root) {
    root["success"] = true;

    JsonObject status = root.createNestedObject("status");
    status["uptime"] = millis() / 1000;

    JsonObject memory = status.createNestedObject("memory");
    memory["freeHeap"] = freeHeap;
    memory["freeHeapPercent"] = freeHeapPercent;
    memory["color"] = heapColor;

    JsonObject storage = status.createNestedObject("storage");
    storage["flashSize"] = flashSize;
    storage["usedSpace"] = usedSpace;
    storage["availableSpace"] = availableSpace;
    storage["usedSpacePercent"] = usedSpacePercent;
    storage["color"] = spaceColor;

    JsonObject platform = status.createNestedObject("platform");
    platform["mode"] =
        (currentMode == CONNECTED) ? "Connected" : "Config Portal";
    platform["httpsEnabled"] = httpsEnabled;
    platform["serverPort"] = serverPort;
    platform["hostname"] = getHostname();
    platform["moduleCount"] = registeredModules.size();
    platform["routeCount"] = getRouteCount();
    platform["platformVersion"] = getPlatformVersion();
    platform["systemVersion"] = getSystemVersion();
  });
}

void WebPlatform::getNetworkStatusApiHandler(WebRequest &req,
                                             WebResponse &res) {
  // Use JsonResponseBuilder for simple response
  JsonResponseBuilder::createResponse<512>(res, [&](JsonObject &root) {
    root["success"] = true;

    JsonObject network = root.createNestedObject("network");
    network["ssid"] = WiFi.SSID();
    network["ipAddress"] = WiFi.localIP().toString();
    network["macAddress"] = WiFi.macAddress();
    network["signalStrength"] = WiFi.RSSI();
  });
}

void WebPlatform::getModulesApiHandler(WebRequest &req, WebResponse &res) {
  // Size based on number of modules
  size_t estimatedSize = 256 + (registeredModules.size() * 256);
  JsonResponseBuilder::createDynamicResponse(
      res,
      [&](JsonObject &root) {
        root["success"] = true;

        JsonArray modules = root.createNestedArray("modules");
        for (const auto &regModule : registeredModules) {
          JsonObject module = modules.createNestedObject();
          module["name"] = regModule.module->getModuleName();
          module["version"] = regModule.module->getModuleVersion();
          module["description"] = regModule.module->getModuleDescription();
          module["basePath"] = regModule.basePath;
        }
      },
      estimatedSize);
}

void WebPlatform::getOpenAPISpecHandler(WebRequest &req, WebResponse &res) {
  // Serve pre-generated OpenAPI spec (generated once during initialization)
  streamPreGeneratedOpenAPISpec(res);
}

void WebPlatform::getMakerAPISpecHandler(WebRequest &req, WebResponse &res) {
  // Serve pre-generated Maker OpenAPI spec (generated once during
  // initialization)
  streamPreGeneratedMakerAPISpec(res);
}