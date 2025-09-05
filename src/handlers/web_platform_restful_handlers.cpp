#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"
#include <functional>

// RESTful API Handlers - User Management

void WebPlatform::getUsersApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  // Check if user is admin (for now, assume username "admin" is admin)
  const AuthContext &auth = req.getAuthContext();
  if (auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Admin access required\"}");
    return;
  }

  std::vector<AuthUser> users = AuthStorage::getAllUsers();

  String json = "{\"success\":true,\"users\":[";
  for (size_t i = 0; i < users.size(); i++) {
    if (i > 0)
      json += ",";
    json += "{";
    json += "\"id\":\"" + users[i].id + "\",";
    json += "\"username\":\"" + users[i].username + "\",";
    json += "\"createdAt\":" + String(users[i].createdAt);
    json += "}";
  }
  json += "]}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}

void WebPlatform::createUserApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_POST) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  // Check if user is admin
  const AuthContext &auth = req.getAuthContext();
  if (auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Admin access required\"}");
    return;
  }

  String username = req.getJsonParam("username");
  String password = req.getJsonParam("password");

  if (username.isEmpty() || password.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent(
        "{\"success\":false,\"message\":\"Username and password required\"}");
    return;
  }

  if (password.length() < 4) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password must be at least "
                   "4 characters\"}");
    return;
  }

  // Check if user already exists
  AuthUser existingUser = AuthStorage::findUserByUsername(username);
  if (existingUser.isValid()) {
    res.setStatus(409);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User already exists\"}");
    return;
  }

  String userId = AuthStorage::createUser(username, password);
  if (userId.isEmpty()) {
    res.setStatus(500);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Failed to create user\"}");
    return;
  }

  res.setStatus(201);
  res.setHeader("Content-Type", "application/json");
  res.setContent("{\"success\":true,\"message\":\"User created\",\"id\":\"" +
                 userId + "\"}");
}

void WebPlatform::getUserByIdApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User ID required\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only access their own data unless they are admin
  if (userId != currentUser.id && auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Access denied\"}");
    return;
  }

  AuthUser user = AuthStorage::findUserById(userId);
  if (!user.isValid()) {
    res.setStatus(404);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User not found\"}");
    return;
  }

  String json = "{\"success\":true,\"user\":{";
  json += "\"id\":\"" + user.id + "\",";
  json += "\"username\":\"" + user.username + "\",";
  json += "\"createdAt\":" + String(user.createdAt);
  json += "}}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}

void WebPlatform::updateUserByIdApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_PUT) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User ID required\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only update their own data unless they are admin
  if (userId != currentUser.id && auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Access denied\"}");
    return;
  }

  String password = req.getJsonParam("password");
  if (password.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password is required\"}");
    return;
  }

  if (password.length() < 4) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password must be at least "
                   "4 characters\"}");
    return;
  }

  bool success = AuthStorage::updateUserPassword(userId, password);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"User updated\"}");
  } else {
    res.setStatus(500);
    res.setContent("{\"success\":false,\"message\":\"Failed to update user\"}");
  }
}

void WebPlatform::deleteUserByIdApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_DELETE) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  // Check if user is admin
  const AuthContext &auth = req.getAuthContext();
  if (auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Admin access required\"}");
    return;
  }

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User ID required\"}");
    return;
  }

  // Don't allow deleting the admin user
  AuthUser targetUser = AuthStorage::findUserById(userId);
  if (targetUser.username == "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent(
        "{\"success\":false,\"message\":\"Cannot delete admin user\"}");
    return;
  }

  bool success = AuthStorage::deleteUser(userId);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"User deleted\"}");
  } else {
    res.setStatus(500);
    res.setContent("{\"success\":false,\"message\":\"Failed to delete user\"}");
  }
}

// Current User Convenience Handlers

void WebPlatform::getCurrentUserApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser user = AuthStorage::findUserByUsername(auth.username);

  if (!user.isValid()) {
    res.setStatus(404);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User not found\"}");
    return;
  }

  String json = "{\"success\":true,\"user\":{";
  json += "\"id\":\"" + user.id + "\",";
  json += "\"username\":\"" + user.username + "\",";
  json += "\"createdAt\":" + String(user.createdAt);
  json += "}}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}

void WebPlatform::updateCurrentUserApiHandler(WebRequest &req,
                                              WebResponse &res) {
  if (req.getMethod() != WebModule::WM_PUT) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser user = AuthStorage::findUserByUsername(auth.username);
  String password = req.getJsonParam("password");

  if (password.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password is required\"}");
    return;
  }

  if (password.length() < 4) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Password must be at least "
                   "4 characters\"}");
    return;
  }

  bool success = AuthStorage::updateUserPassword(user.id, password);

  res.setHeader("Content-Type", "application/json");
  if (success) {
    res.setContent("{\"success\":true,\"message\":\"User updated\"}");
  } else {
    res.setStatus(500);
    res.setContent("{\"success\":false,\"message\":\"Failed to update user\"}");
  }
}

// Token Management Handlers

void WebPlatform::getUserTokensApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User ID required\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only access their own tokens unless they are admin
  if (userId != currentUser.id && auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Access denied\"}");
    return;
  }

  std::vector<AuthApiToken> tokens = AuthStorage::getUserApiTokens(userId);

  String json = "{\"success\":true,\"tokens\":[";
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i > 0)
      json += ",";
    json += "{";
    json += "\"token\":\"" + tokens[i].token + "\",";
    json += "\"name\":\"" + tokens[i].name + "\",";
    json += "\"createdAt\":" + String(tokens[i].createdAt) + ",";
    json += "\"expiresAt\":" + String(tokens[i].expiresAt);
    json += "}";
  }
  json += "]}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}

void WebPlatform::createUserTokenApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_POST) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String userId = req.getRouteParameter("id");
  if (userId.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"User ID required\"}");
    return;
  }

  const AuthContext &auth = req.getAuthContext();
  AuthUser currentUser = AuthStorage::findUserByUsername(auth.username);

  // Users can only create tokens for themselves unless they are admin
  if (userId != currentUser.id && auth.username != "admin") {
    res.setStatus(403);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Access denied\"}");
    return;
  }

  String tokenName = req.getJsonParam("name");
  if (tokenName.isEmpty()) {
    res.setStatus(400);
    res.setHeader("Content-Type", "application/json");
    res.setContent(
        "{\"success\":false,\"message\":\"Token name is required\"}");
    return;
  }

  String token = AuthStorage::createApiToken(userId, tokenName);
  if (token.isEmpty()) {
    res.setStatus(500);
    res.setHeader("Content-Type", "application/json");
    res.setContent(
        "{\"success\":false,\"message\":\"Failed to create token\"}");
    return;
  }

  res.setStatus(201);
  res.setHeader("Content-Type", "application/json");
  res.setContent("{\"success\":true,\"token\":\"" + token + "\"}");
}

// System Status API Endpoints

void WebPlatform::getSystemStatusApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  // Memory information
  uint32_t freeHeap = ESP.getFreeHeap();
  int freeHeapPercent = 0;

#if defined(ESP32)
  uint32_t totalHeap = ESP.getHeapSize();
  if (totalHeap > 0) {
    freeHeapPercent = (int)((float)freeHeap / totalHeap * 100.0);
  }
#else
  uint32_t estimatedTotalHeap = 80 * 1024;
  freeHeapPercent = (int)((float)freeHeap / estimatedTotalHeap * 100.0);
#endif

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

  // Build JSON response
  String json = "{\"success\":true,\"status\":{";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"memory\":{";
  json += "\"freeHeap\":" + String(freeHeap) + ",";
  json += "\"freeHeapPercent\":" + String(freeHeapPercent) + ",";
  json += "\"color\":\"" + heapColor + "\"";
  json += "},";
  json += "\"storage\":{";
  json += "\"flashSize\":" + String(flashSize) + ",";
  json += "\"usedSpace\":" + String(usedSpace) + ",";
  json += "\"availableSpace\":" + String(availableSpace) + ",";
  json += "\"usedSpacePercent\":" + String(usedSpacePercent) + ",";
  json += "\"color\":\"" + spaceColor + "\"";
  json += "},";
  json += "\"platform\":{";
  json += "\"mode\":\"" +
          String(currentMode == CONNECTED ? "Connected" : "Config Portal") +
          "\",";
  json += "\"httpsEnabled\":" + String(httpsEnabled ? "true" : "false") + ",";
  json += "\"serverPort\":" + String(serverPort) + ",";
  json += "\"hostname\":\"" + getHostname() + "\",";
  json += "\"moduleCount\":" + String(registeredModules.size()) + ",";
  json += "\"routeCount\":" + String(getRouteCount());
  json += "}";
  json += "}}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}

void WebPlatform::getNetworkStatusApiHandler(WebRequest &req,
                                             WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String json = "{\"success\":true,\"network\":{";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"ipAddress\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"macAddress\":\"" + WiFi.macAddress() + "\",";
  json += "\"signalStrength\":" + String(WiFi.RSSI());
  json += "}}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}

void WebPlatform::getModulesApiHandler(WebRequest &req, WebResponse &res) {
  if (req.getMethod() != WebModule::WM_GET) {
    res.setStatus(405);
    res.setHeader("Content-Type", "application/json");
    res.setContent("{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String json = "{\"success\":true,\"modules\":[";
  for (size_t i = 0; i < registeredModules.size(); i++) {
    if (i > 0)
      json += ",";
    json += "{";
    json +=
        "\"name\":\"" + registeredModules[i].module->getModuleName() + "\",";
    json += "\"version\":\"" + registeredModules[i].module->getModuleVersion() +
            "\",";
    json += "\"basePath\":\"" + registeredModules[i].basePath + "\"";
    json += "}";
  }
  json += "]}";

  res.setHeader("Content-Type", "application/json");
  res.setContent(json);
}
