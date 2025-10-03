/**
 * Environmental Sensor Module - Header File
 *
 * This is a complete, self-contained example showing how to create a custom
 * web module using current WebPlatform patterns. Simply copy these files
 * to your project and include in main.cpp
 *
 * Features demonstrated:
 * - Complete IWebModule interface implementation
 * - Mixed authentication requirements (public/protected endpoints)
 * - Real-time data updates with JavaScript
 * - Configuration management with forms and AuthUtils.js
 * - RESTful API design with OpenAPI documentation
 * - CSRF protection using current patterns
 * - Module state management and lifecycle methods
 * - Build flag awareness (WEB_PLATFORM_OPENAPI)
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <web_platform.h>

class EnvironmentalSensorModule : public IWebModule {
private:
  // Simulated sensor data
  float temperature = 23.5;
  float humidity = 45.2;
  bool sensorEnabled = true;
  String sensorLocation = "Office";
  unsigned long lastReading = 0;

  // Configuration
  float tempThreshold = 30.0;
  float humidityThreshold = 60.0;
  bool alertsEnabled = true;

public:
  // Constructor
  EnvironmentalSensorModule();

  // Module lifecycle methods
  using IWebModule::begin;
  void begin() override;
  void handle() override;

  // Public getters for current data
  float getTemperature() const;
  float getHumidity() const;
  bool isEnabled() const;
  String getLocation() const;

  // Configuration setters (optional - for programmatic control)
  void setLocation(const String &location);
  void setTempThreshold(float threshold);
  void setHumidityThreshold(float threshold);
  void setAlertsEnabled(bool enabled);

  // Required IWebModule methods
  std::vector<RouteVariant> getHttpRoutes() override;
  std::vector<RouteVariant> getHttpsRoutes() override;
  String getModuleName() const override;
  String getModuleVersion() const override;
  String getModuleDescription() const override;

private:
  // Internal methods
  void updateSensorReadings();

  // Route handlers
  void mainPageHandler(WebRequest &req, WebResponse &res);
  void getCurrentDataHandler(WebRequest &req, WebResponse &res);
  void configPageHandler(WebRequest &req, WebResponse &res);
  void updateConfigHandler(WebRequest &req, WebResponse &res);
  void getDataAPIHandler(WebRequest &req, WebResponse &res);
  void controlAPIHandler(WebRequest &req, WebResponse &res);

  // PROGMEM route optimization helpers
  WebModule::UnifiedRouteHandler getHandlerForRoute(int routeIndex);
  OpenAPIDocumentation getDocumentationForRoute(int routeIndex);
};

// Global instance of the module (declared here, defined in .cpp)
// NOSONAR: This module instance must be mutable for state management
extern EnvironmentalSensorModule sensorModule;