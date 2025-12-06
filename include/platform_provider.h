// Platform provider for production and tests. Accepts any IWebPlatform
// implementation to support native tests (MockWebPlatform) and ESP32 (WebPlatform).
#pragma once
#include <web_platform_interface.h>

// Production platform provider implementation
class WebPlatformProvider : public IWebPlatformProvider {
public:
  explicit WebPlatformProvider(IWebPlatform *platform) : _platform(platform) {}

  IWebPlatform &getPlatform() override { return *_platform; }

private:
  IWebPlatform *_platform;
};

// Convenience setup function
void setupProductionPlatformProvider();