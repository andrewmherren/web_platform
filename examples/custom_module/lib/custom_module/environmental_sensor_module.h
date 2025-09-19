/**
 * Environmental Sensor Module - Header File
 *
 * This is a complete, self-contained example showing how to create a custom
 * web module. Simply copy these files to your project and include in main.cpp
 *
 * Features demonstrated:
 * - Complete IWebModule interface implementation
 * - Mixed authentication requirements (public/protected endpoints)
 * - Real-time data updates with JavaScript
 * - Configuration management with forms
 * - RESTful API design
 * - CSRF protection for forms
 * - Module state management
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <web_platform.h>

class EnvironmentalSensorModule : public IWebModule {
private:
  // Simulated sensor data
  float temperature;
  float humidity;
  bool sensorEnabled;
  String sensorLocation;
  unsigned long lastReading;

  // Configuration
  float tempThreshold;
  float humidityThreshold;
  bool alertsEnabled;

public:
  // Constructor
  EnvironmentalSensorModule();

  // Module lifecycle methods
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
};

// Global instance of the module (declared here, defined in .cpp)
extern EnvironmentalSensorModule sensorModule;