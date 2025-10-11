#pragma once
#include "web_platform.h"
#include <interface/web_platform_interface.h>

// Production platform provider implementation
class ProductionPlatformProvider : public IWebPlatformProvider {
public:
  IWebPlatform &getPlatform() override {
    return webPlatform; // the global webPlatform object from web_platform
  }
};

// Static instance
static ProductionPlatformProvider productionProvider;

// Convenience setup function
static void setupProductionPlatformProvider() {
  IWebPlatformProvider::instance = &productionProvider;
}