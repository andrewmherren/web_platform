#ifndef MOCK_STATIC_ASSET_H
#define MOCK_STATIC_ASSET_H

#include <Arduino.h>
#include <web_module_interface.h>

// This structure replaces the StaticAsset structure that was previously in
// IWebModule It is used only for backward compatibility during migration to the
// route-based system
struct StaticAsset {
  String path;
  String content;
  String mimeType;
  bool useProgmem;

  StaticAsset() : path(""), content(""), mimeType(""), useProgmem(false) {}

  StaticAsset(const String &p, const String &c, const String &m, bool up)
      : path(p), content(c), mimeType(m), useProgmem(up) {}
};

// Mock implementation of the old static asset API
// These functions return empty results since static assets are now handled as
// routes
inline std::vector<WebRoute> getStaticAssetRoutes() {
  return std::vector<WebRoute>();
}

inline StaticAsset getStaticAsset(const String &path) { return StaticAsset(); }

#endif // MOCK_STATIC_ASSET_H