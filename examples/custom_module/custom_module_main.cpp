/**
 * Custom Module Example - main.cpp
 *
 * This demonstrates how to use the Environmental Sensor Module in your main
 * application.
 *
 * To use this example:
 * 1. Copy environmental_sensor_module.h and environmental_sensor_module.cpp to
 * your lib/ directory
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
  Serial.println("Starting Custom Environmental Sensor Example...");

  // Set up navigation menu
  std::vector<NavigationItem> navItems = {
      NavigationItem("Dashboard", "/"),
      NavigationItem("Environmental", "/sensors/"),
      Authenticated(NavigationItem("Account", "/account")),
      NavigationItem("System Info", "/status")};
  IWebModule::setNavigationMenu(navItems);

  // Initialize WebPlatform
  webPlatform.begin("SensorDevice");

  if (webPlatform.isConnected()) {
    Serial.println(
        "WiFi connected! Registering environmental sensor module...");

    // Initialize the custom module
    sensorModule.begin();

    // Register the custom module at /sensors/
    webPlatform.registerModule("/sensors", &sensorModule);

    Serial.print("Environmental sensor ready at: ");
    Serial.println(webPlatform.getBaseUrl() + "/sensors/");
    Serial.println("Dashboard available at: " + webPlatform.getBaseUrl());
    Serial.println("Default login: admin / admin");
  } else {
    Serial.println("WiFi configuration mode - connect to setup network");
  }
}

void loop() {
  webPlatform.handle();

  if (webPlatform.isConnected()) {
    sensorModule.handle();
  }

  delay(100);
}