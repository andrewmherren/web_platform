#ifndef ENVIRONMENTAL_SENSOR_DOCS_H
#define ENVIRONMENTAL_SENSOR_DOCS_H

#include "interface/openapi_documentation.h"
#include "interface/openapi_factory.h"
#include <vector>

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