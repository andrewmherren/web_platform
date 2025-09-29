#ifndef ENVIRONMENTAL_SENSOR_DOCS_H
#define ENVIRONMENTAL_SENSOR_DOCS_H

#include <Arduino.h>
#include <vector>
#include <web_platform.h>

class EnvironmentalSensorDocs {
public:
  static const std::vector<String> SENSOR_TAGS;

  // Documentation factory methods
  static OpenAPIDocumentation createGetCurrentReadings();
  static OpenAPIDocumentation createUpdateSensorConfig();
  static OpenAPIDocumentation createGetDetailedSensorData();
  static OpenAPIDocumentation createControlSensor();
};

#endif // ENVIRONMENTAL_SENSOR_DOCS_H