/**
 * Custom Module Example - main.cpp
 *
 * This demonstrates how to use the Environmental Sensor Module in your main
 * application.
 *
 * To use this example:
 * 1. Copy environmental_sensor_module.h and environmental_sensor_module.cpp to
 * your lib/your_module directory
 * 2. Copy this code to your src/main.cpp
 * 3. Build and upload to your ESP32
 *
 * The module will be available at: http://yourdevice.local/sensors/
 */

#include "environmental_sensor_module.h"
#include <Arduino.h>
#include <web_platform.h>

void setup() {
  Serial.begin(115200);
  DEBUG_PRINTLN("Starting Custom Environmental Sensor Example...");

  // Set up navigation menu
  std::vector<NavigationItem> navItems = {
      NavigationItem("Dashboard", "/"),
      NavigationItem("Environmental", "/sensors/"),
      Authenticated(NavigationItem("Account", "/account")),
      NavigationItem("System Info", "/status")};
  IWebModule::setNavigationMenu(navItems);

  // register modules
  webPlatform.registerModule("/sensors", &sensorModule);
  // Initialize WebPlatform
  webPlatform.setSystemVersion("1.0.0");
  webPlatform.begin("SensorDevice");

  if (webPlatform.isConnected()) {
    DEBUG_PRINTLN("WiFi connected! Registering environmental sensor module...");

    DEBUG_PRINT("Environmental sensor ready at: ");
    DEBUG_PRINTLN(webPlatform.getBaseUrl() + "/sensors/");
    DEBUG_PRINTLN("Dashboard available at: " + webPlatform.getBaseUrl());
    DEBUG_PRINTLN("Default login: admin / admin");
  } else {
    DEBUG_PRINTLN("WiFi configuration mode - connect to setup network");
  }
}

void loop() {
  webPlatform.handle();

  delay(100);
}