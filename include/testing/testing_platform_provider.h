#ifndef TESTING_PLATFORM_PROVIDER_H
#define TESTING_PLATFORM_PROVIDER_H

#include "mock_web_platform.h"
#include <Arduino.h>
#include <interface/web_platform_interface.h>
#include <memory>
#include <utility>
#include <vector>


/**
 * Single canonical mock platform
 */
class MockWebPlatform : public IWebPlatform {
private:
  String deviceName;
  bool connected = true;
  bool httpsEnabled = true;
  std::vector<std::pair<String, IWebModule *>> registeredModules;
  int routeCount = 0;

public:
  void begin(const String &name) override { deviceName = name; }

  void begin(const String &name, bool httpsOnly) override {
    deviceName = name;
    httpsEnabled = httpsOnly;
  }

  void handle() override {
    // Mock implementation - do nothing
  }

  bool isConnected() const override { return connected; }
  bool isHttpsEnabled() const override { return httpsEnabled; }
  String getBaseUrl() const override {
    return httpsEnabled ? "https://mock-device.local"
                        : "http://mock-device.local";
  }

  void registerModule(const String &basePath, IWebModule *module) override {
    registeredModules.push_back(std::make_pair(basePath, module));
    // Add routes from module to mock route count
    auto httpRoutes = module->getHttpRoutes();
    auto httpsRoutes = module->getHttpsRoutes();
    routeCount += httpRoutes.size() + httpsRoutes.size();
  }

  void registerWebRoute(const String &path,
                        WebModule::UnifiedRouteHandler handler,
                        const AuthRequirements &auth,
                        WebModule::Method method) override {
    routeCount++;
  }

  void registerApiRoute(const String &path,
                        WebModule::UnifiedRouteHandler handler,
                        const AuthRequirements &auth, WebModule::Method method,
                        const OpenAPIDocumentation &docs) override {
    routeCount++;
  }

  int getRouteCount() const override { return routeCount; }

  void disableRoute(const String &path, WebModule::Method method) override {
    if (routeCount > 0)
      routeCount--;
  }

  String getDeviceName() const override { return deviceName; }

  void setErrorPage(int statusCode, const String &html) override {
    // Mock implementation
  }

  void addGlobalRedirect(const String &fromPath,
                         const String &toPath) override {
    // Mock implementation
  }

  // Test utility methods
  void setConnected(bool conn) { connected = conn; }
  int getRegisteredModuleCount() const { return registeredModules.size(); }
  std::vector<std::pair<String, IWebModule *>> getRegisteredModules() const {
    return registeredModules;
  }
};

/**
 * Provider for tests to inject the mock platform
 */
class MockWebPlatformProvider : public IWebPlatformProvider {
private:
  std::unique_ptr<MockWebPlatform> mockPlatform;

public:
  MockWebPlatformProvider()
      : mockPlatform(std::make_unique<MockWebPlatform>()) {}

  IWebPlatform &getPlatform() override { return *mockPlatform; }

  MockWebPlatform &getMockPlatform() { return *mockPlatform; }
};

#endif // TESTING_PLATFORM_PROVIDER_H
