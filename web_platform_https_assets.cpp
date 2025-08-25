#include "web_platform.h"

// HTTPS static asset handling
// This file adds support for serving static assets over HTTPS

#if defined(ESP32)

void WebPlatform::registerHttpsStaticAssets() {
  if (!httpsServerHandle) {
    Serial.println("WebPlatform: No HTTPS server for static assets");
    return;
  }

  Serial.println("WebPlatform: Registering HTTPS static assets...");
  
  // Get all static assets from IWebModule
  auto assetRoutes = IWebModule::getStaticAssetRoutes();
  int assetsRegistered = 0;
  
  for (const auto &route : assetRoutes) {
    // Store path permanently for ESP-IDF
    httpsRoutePaths.push_back(route.path);
    const char *pathPtr = httpsRoutePaths.back().c_str();
    
    httpd_uri_t asset_config = {
      .uri = pathPtr,
      .method = HTTP_GET,
      .handler = httpsGenericHandler,
      .user_ctx = nullptr
    };
    
    esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &asset_config);
    if (ret != ESP_OK) {
      Serial.printf("  Failed to register asset %s: %d\n", route.path.c_str(), ret);
    } else {
      Serial.printf("  Registered asset: %s\n", route.path.c_str());
      assetsRegistered++;
    }
  }
  
  // Special case for CSS files and aliases
  const char* cssAliases[] = {
    "/assets/style.css",
    "/assets/tickertape-theme.css"
  };
  
  for (const char* cssPath : cssAliases) {
    // Check if it's already registered
    bool alreadyRegistered = false;
    for (const auto &route : assetRoutes) {
      if (route.path == cssPath) {
        alreadyRegistered = true;
        break;
      }
    }
    
    if (!alreadyRegistered) {
      httpsRoutePaths.push_back(cssPath);
      const char *pathPtr = httpsRoutePaths.back().c_str();
      
      httpd_uri_t style_alias = {
        .uri = pathPtr,
        .method = HTTP_GET,
        .handler = httpsGenericHandler,
        .user_ctx = nullptr
      };
      
      esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &style_alias);
      if (ret != ESP_OK) {
        Serial.printf("  Failed to register CSS alias %s: %d\n", cssPath, ret);
      } else {
        Serial.printf("  Registered CSS alias: %s\n", cssPath);
        assetsRegistered++;
      }
    }
  }

  // Check and register additional JavaScript files from modules
  const char* jsFiles[] = {
    "/assets/tickertape-utils.js",
    "/assets/usb-pd-controller.js"  // USB PD controller specific JS
  };
  
  for (const char* jsPath : jsFiles) {
    // Check if it's already registered
    bool alreadyRegistered = false;
    for (const auto &route : assetRoutes) {
      if (route.path == jsPath) {
        alreadyRegistered = true;
        break;
      }
    }
    
    if (!alreadyRegistered) {
      httpsRoutePaths.push_back(jsPath);
      const char *pathPtr = httpsRoutePaths.back().c_str();
      
      httpd_uri_t js_route = {
        .uri = pathPtr,
        .method = HTTP_GET,
        .handler = httpsGenericHandler,
        .user_ctx = nullptr
      };
      
      esp_err_t ret = httpd_register_uri_handler(httpsServerHandle, &js_route);
      if (ret != ESP_OK) {
        Serial.printf("  Failed to register JS file %s: %d\n", jsPath, ret);
      } else {
        Serial.printf("  Registered JS file: %s\n", jsPath);
        assetsRegistered++;
      }
    }
  }
  
  Serial.printf("WebPlatform: Registered %d HTTPS static assets\n", assetsRegistered);
  
  // Debug - list all assets registered with the IWebModule system
  Serial.println("WebPlatform: Assets registered with IWebModule:");
  for (const auto &route : assetRoutes) {
    Serial.printf("  %s (%s)\n", route.path.c_str(), route.contentType.c_str());
  }
}

#endif // ESP32