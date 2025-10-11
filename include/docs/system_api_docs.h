#ifndef SYSTEM_API_DOCS_H
#define SYSTEM_API_DOCS_H
#include <interface/openapi_factory.h>
#include <vector>

/**
 * Documentation factory for system and WiFi-related API endpoints
 * Following the recommended pattern of separating documentation from
 * implementation
 */
class SystemApiDocs {
public:
  // Module-specific tags for grouping endpoints
  static const std::vector<String> SYSTEM_TAGS;
  static const std::vector<String> WIFI_TAGS;
  static const std::vector<String> NETWORK_TAGS;

  // System Status Documentation
  static OpenAPIDocumentation createGetStatus();
  static OpenAPIDocumentation createGetSystemStatus();
  static OpenAPIDocumentation createGetNetworkStatus();
  static OpenAPIDocumentation createGetModules();
  static OpenAPIDocumentation createGetOpenAPISpec();
  static OpenAPIDocumentation createResetDevice();

  // WiFi Management Documentation
  static OpenAPIDocumentation createScanWifi();
  static OpenAPIDocumentation createConfigureWifi();
};

#endif