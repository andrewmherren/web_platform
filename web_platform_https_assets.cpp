#include "web_platform.h"

// HTTPS static asset handling
// This file adds support for serving static assets over HTTPS

#if defined(ESP32)

void WebPlatform::registerHttpsStaticAssets() {
  if (!httpsServerHandle) {
    Serial.println("WebPlatform: No HTTPS server for static assets");
    return;
  }

  Serial.println(
      "WebPlatform: Phase 1 migration - static assets are now unified routes");

  // In Phase 1 of the Route Handler Migration, we've replaced static assets
  // with unified routes This method is kept as a stub for backward
  // compatibility

  int assetsRegistered = 0;

  // The following code is disabled as part of the migration
  /*
  for (const auto &route : assetRoutes) {
    // Store path permanently for ESP-IDF
    httpsRoutePaths.push_back(route.path);
    const char *pathPtr = httpsRoutePaths.back().c_str();

    httpd_uri_t asset_config = {.uri = pathPtr,
                                .method = HTTP_GET,
                                .handler = httpsGenericHandler,
                                .user_ctx = nullptr};

    esp_err_t ret =
        httpd_register_uri_handler(httpsServerHandle, &asset_config);
    if (ret != ESP_OK) {
      Serial.printf("  Failed to register asset %s: %d\n", route.path.c_str(),
                    ret);
    } else {
      Serial.printf("  Registered asset: %s\n", route.path.c_str());
      assetsRegistered++;
    }
  }

  // Special case for CSS files and aliases
  const char *cssAliases[] = {"/assets/style.css"};

  for (const char *cssPath : cssAliases) {
    // Register CSS asset path
    httpsRoutePaths.push_back(cssPath);
    const char *pathPtr = httpsRoutePaths.back().c_str();

    httpd_uri_t style_alias = {.uri = pathPtr,
                             .method = HTTP_GET,
                             .handler = httpsGenericHandler,
                             .user_ctx = nullptr};

    esp_err_t ret =
        httpd_register_uri_handler(httpsServerHandle, &style_alias);
    if (ret != ESP_OK) {
      Serial.printf("  Failed to register CSS alias %s: %d\n", cssPath, ret);
    } else {
      Serial.printf("  Registered CSS alias: %s\n", cssPath);
      assetsRegistered++;
    }
  }
  */

  Serial.printf("WebPlatform: Phase 1 static asset migration complete\n");
}

#endif // ESP32