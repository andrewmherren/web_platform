#pragma once
#include "web_platform.h"
#include <web_platform_interface.h>

// Production platform provider implementation
class WebPlatformProvider : public IWebPlatformProvider {
public:
  WebPlatformProvider(WebPlatform *platform) : _platform(platform) {}

  IWebPlatform &getPlatform() override { return *_platform; }

private:
  WebPlatform *_platform;
};

// Convenience setup function
void setupProductionPlatformProvider();