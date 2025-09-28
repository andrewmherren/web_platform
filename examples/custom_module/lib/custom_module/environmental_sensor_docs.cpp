#include "environmental_sensor_docs.h"

const std::vector<String> EnvironmentalSensorDocs::SENSOR_TAGS = {
    "Environmental Sensor"};

OpenAPIDocumentation EnvironmentalSensorDocs::createGetCurrentReadings() {
  return OpenAPIFactory::create("Get current sensor readings",
                                "Returns current temperature and humidity "
                                "readings from the environmental sensor",
                                "getCurrentReadings", SENSOR_TAGS)
      .withResponseExample(R"({
        "temperature": 23.5,
        "humidity": 45.2,
        "timestamp": 1684567890,
        "seconds_since_reading": 15,
        "location": "Office",
        "status": "active"
      })")
      .withResponseSchema(R"({
        "type": "object",
        "properties": {
          "temperature": {"type": "number", "description": "Temperature in Celsius"},
          "humidity": {"type": "number", "description": "Humidity percentage"},
          "timestamp": {"type": "integer", "description": "Unix timestamp in seconds"},
          "seconds_since_reading": {"type": "integer", "description": "Seconds since last sensor reading"},
          "location": {"type": "string", "description": "Sensor location"},
          "status": {"type": "string", "enum": ["active", "disabled"], "description": "Sensor status"}
        },
        "required": ["temperature", "humidity", "timestamp", "seconds_since_reading", "location", "status"]
      })");
}

OpenAPIDocumentation EnvironmentalSensorDocs::createUpdateSensorConfig() {
  return OpenAPIFactory::create("Update sensor configuration",
                                "Updates sensor settings including location, "
                                "thresholds, and enable/disable state",
                                "updateSensorConfig", SENSOR_TAGS)
      .withParameters(R"([
        {
          "name": "location",
          "in": "formData",
          "required": true,
          "type": "string",
          "description": "Sensor location name"
        },
        {
          "name": "enabled",
          "in": "formData",
          "required": false,
          "type": "string",
          "enum": ["on"],
          "description": 'Enable sensor (checkbox - present when checked)'
        },
        {
          "name": "temp-threshold",
          "in": "formData",
          "required": true,
          "type": "number",
          "minimum": 0,
          "description": "Temperature alert threshold in Celsius"
        },
        {
          "name": "humidity-threshold",
          "in": "formData",
          "required": true,
          "type": "number",
          "minimum": 0,
          "description": "Humidity alert threshold percentage"
        },
        {
          "name": "alerts-enabled",
          "in": "formData",
          "required": false,
          "type": "string",
          "enum": ["on"],
          "description": 'Enable threshold alerts (checkbox - present when checked)'
        }
      ])")
      .withRequestExample(
          R"(location=Office&enabled=on&temp-threshold=30.0&humidity-threshold=60.0&alerts-enabled=on)")
      .withResponseExample(OpenAPIFactory::createSuccessResponse(
          "Configuration updated successfully"))
      .withResponseSchema(OpenAPIFactory::createSuccessResponse());
}

OpenAPIDocumentation EnvironmentalSensorDocs::createGetDetailedSensorData() {
  return OpenAPIFactory::create(
             "Get detailed sensor data",
             "Returns comprehensive sensor information including readings, "
             "configuration, and status alerts",
             "getDetailedSensorData", SENSOR_TAGS)
      .withResponseExample(R"({
        "sensor_info": {
          "name": "Environmental Sensor",
          "version": "1.2.0",
          "location": "Office",
          "enabled": true
        },
        "current_readings": {
          "temperature": 23.5,
          "humidity": 45.2,
          "timestamp": 1684567890,
          "last_update": 15
        },
        "configuration": {
          "temp_threshold": 30.0,
          "humidity_threshold": 60.0,
          "alerts_enabled": true
        },
        "status": {
          "temp_alert": false,
          "humidity_alert": false,
          "operational": true
        }
      })")
      .withResponseSchema(R"({
        "type": "object",
        "properties": {
          "sensor_info": {
            "type": "object",
            "properties": {
              "name": {"type": "string", "description": "Module name"},
              "version": {"type": "string", "description": "Module version"},
              "location": {"type": "string", "description": "Sensor location"},
              "enabled": {"type": "boolean", "description": "Whether sensor is enabled"}
            }
          },
          "current_readings": {
            "type": "object",
            "properties": {
              "temperature": {"type": "number", "description": "Temperature in Celsius"},
              "humidity": {"type": "number", "description": "Humidity percentage"},
              "timestamp": {"type": "integer", "description": "Reading timestamp in seconds"},
              "last_update": {"type": "integer", "description": "Seconds since last reading"}
            }
          },
          "configuration": {
            "type": "object",
            "properties": {
              "temp_threshold": {"type": "number", "description": "Temperature alert threshold"},
              "humidity_threshold": {"type": "number", "description": "Humidity alert threshold"},
              "alerts_enabled": {"type": "boolean", "description": "Whether alerts are enabled"}
            }
          },
          "status": {
            "type": "object",
            "properties": {
              "temp_alert": {"type": "boolean", "description": "Temperature threshold exceeded"},
              "humidity_alert": {"type": "boolean", "description": "Humidity threshold exceeded"},
              "operational": {"type": "boolean", "description": "Sensor operational status"}
            }
          }
        }
      })");
}

OpenAPIDocumentation EnvironmentalSensorDocs::createControlSensor() {
  return OpenAPIFactory::create("Control sensor operations",
                                "Send control commands to the sensor (refresh, "
                                "enable, disable, reset-alerts)",
                                "controlSensor", SENSOR_TAGS)
      .withParameters(R"([
    {
      "name": "command",
      "in": "formData",
      "required": true,
      "type": "string",
      "enum": ["refresh", "enable", "disable", "reset-alerts"],
      "description": "Control command to execute"
    }
  ])")
      .withRequestExample(R"(command=refresh)")
      .withResponseExample(
          OpenAPIFactory::createSuccessResponse("Sensor readings refreshed"))
      .withResponseSchema(OpenAPIFactory::createSuccessResponse());
}