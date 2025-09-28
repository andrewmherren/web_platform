#include "docs/system_api_docs.h"

// Define module-specific tags
const std::vector<String> SystemApiDocs::SYSTEM_TAGS = {"System"};
const std::vector<String> SystemApiDocs::WIFI_TAGS = {"WiFi Management"};
const std::vector<String> SystemApiDocs::NETWORK_TAGS = {"Network"};

// System Status Documentation

OpenAPIDocumentation SystemApiDocs::createGetStatus() {
  return OpenAPIFactory::create("Get device status",
                                "Returns the current status of the device "
                                "including WiFi connection "
                                "details and basic system information.",
                                "getDeviceStatus", SYSTEM_TAGS)
      .withResponseExample(R"({
            "success": true,
            "status": {
            "wifi": {
                "connected": true,
                "ssid": "MyNetwork",
                "ip": "192.168.1.100",
                "rssi": -45
            },
            "system": {
                "uptime": 3600,
                "freeMemory": 45000,
                "version": "1.0.0"
            }
            }
        })")
      .withResponseSchema(R"({
            "type": "object",
            "properties": {
            "success": {"type": "boolean"},
            "status": {
                "type": "object",
                "properties": {
                "wifi": {
                    "type": "object",
                    "properties": {
                    "connected": {"type": "boolean"},
                    "ssid": {"type": "string"},
                    "ip": {"type": "string"},
                    "rssi": {"type": "number"}
                    }
                },
                "system": {
                    "type": "object",
                    "properties": {
                    "uptime": {"type": "number", "description": "Uptime in seconds"},
                    "freeMemory": {"type": "number", "description": "Free memory in bytes"},
                    "version": {"type": "string"}
                    }
                }
                }
            }
            }
        })");
}

OpenAPIDocumentation SystemApiDocs::createGetSystemStatus() {
  return OpenAPIFactory::create("Get system status",
                                "Returns detailed system information including "
                                "uptime, memory usage, and firmware details.",
                                "getSystemStatus", SYSTEM_TAGS)
      .withResponseExample(R"({
        "success": true,
        "system": {
          "uptime": 7200,
          "freeHeap": 45000,
          "totalHeap": 320000,
          "chipModel": "ESP32-S3",
          "cpuFreq": 240,
          "flashSize": 8388608,
          "sketchSize": 1500000,
          "freeSketchSpace": 6000000,
          "sdkVersion": "4.4.2"
        }
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Detailed system information"));
}

OpenAPIDocumentation SystemApiDocs::createGetNetworkStatus() {
  return OpenAPIFactory::create("Get network status",
                                "Returns network status including IP address, "
                                "signal strength, and connection details.",
                                "getNetworkStatus", NETWORK_TAGS)
      .withResponseExample(R"({
        "success": true,
        "network": {
          "connected": true,
          "ssid": "MyNetwork",
          "bssid": "AA:BB:CC:DD:EE:FF",
          "channel": 6,
          "rssi": -45,
          "quality": 90,
          "ip": "192.168.1.100",
          "subnet": "255.255.255.0",
          "gateway": "192.168.1.1",
          "dns": "8.8.8.8",
          "hostname": "device.local",
          "macAddress": "24:6F:28:12:34:56"
        }
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Network connection details"));
}

OpenAPIDocumentation SystemApiDocs::createGetModules() {
  return OpenAPIFactory::create("Get registered modules",
                                "Returns information about all registered web "
                                "modules and their routes.",
                                "getRegisteredModules", SYSTEM_TAGS)
      .withResponseExample(R"({
        "success": true,
        "modules": [
          {
            "name": "USBPDController",
            "version": "2.1.0",
            "description": "USB-C Power Delivery Controller",
            "basePath": "/usb_pd",
            "routeCount": 6
          }
        ]
      })")
      .withResponseSchema(
          OpenAPIFactory::createListResponse("Registered web modules"));
}

OpenAPIDocumentation SystemApiDocs::createGetOpenAPISpec() {
  return OpenAPIFactory::create(
             "Get Fresh OpenAPI Specification",
             "Returns a freshly generated OpenAPI 3.0 "
             "specification for all registered API routes. "
             "This will also update the cached specification. "
             "Supports filtering by authentication type.",
             "getOpenAPISpecification", SYSTEM_TAGS)
      .withParameters(R"([
        {
          "name": "filter",
          "in": "query",
          "required": false,
          "schema": {
            "type": "string",
            "enum": ["token", "session", "none"],
            "description": "Filter routes by authentication type"
          }
        }
      ])")
      .withResponseExample(R"({
        "openapi": "3.0.0",
        "info": {
          "title": "WebPlatform API",
          "version": "1.0.0",
          "description": "API for embedded WebPlatform device"
        },
        "servers": [
          {
            "url": "https://device.local/api",
            "description": "Device API Server"
          }
        ],
        "paths": {},
        "components": {
          "securitySchemes": {}
        }
      })")
      .withResponseSchema(R"({
        "type": "object",
        "description": "OpenAPI 3.0 specification document"
      })");
}

OpenAPIDocumentation SystemApiDocs::createResetDevice() {
  return OpenAPIFactory::create(
             "Reset device",
             "Resets the device WiFi configuration and restarts in "
             "configuration "
             "portal mode. This will disconnect all current sessions.",
             "resetDevice", SYSTEM_TAGS)
      .withResponseExample(R"({
          "success": true,
          "message": "Device will reset and restart in configuration mode"
        })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Reset confirmation"));
}

// WiFi Management Documentation

OpenAPIDocumentation SystemApiDocs::createScanWifi() {
  return OpenAPIFactory::create("Scan WiFi networks",
                                "Scans for available WiFi networks and returns "
                                "the results with signal "
                                "strength and security information.",
                                "scanWifiNetworks", WIFI_TAGS)
      .withResponseExample(R"({
        "success": true,
        "networks": [
          {
            "ssid": "MyNetwork",
            "rssi": -45,
            "channel": 6,
            "encryption": "WPA2",
            "hidden": false
          },
          {
            "ssid": "GuestNetwork",
            "rssi": -67,
            "channel": 11,
            "encryption": "Open",
            "hidden": false
          }
        ]
      })")
      .withResponseSchema(R"({
        "type": "object",
        "properties": {
          "success": {"type": "boolean"},
          "networks": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "ssid": {"type": "string"},
                "rssi": {"type": "number", "description": "Signal strength in dBm"},
                "channel": {"type": "number"},
                "encryption": {"type": "string"},
                "hidden": {"type": "boolean"}
              }
            }
          }
        }
      })");
}

OpenAPIDocumentation SystemApiDocs::createConfigureWifi() {
  return OpenAPIFactory::create(
             "Configure WiFi",
             "Updates the device's WiFi configuration with new credentials. "
             "The "
             "device will attempt to connect using the provided credentials.",
             "configureWifi", WIFI_TAGS)
      .withRequestExample(R"({
        "ssid": "MyNetwork",
        "password": "mypassword123"
      })")
      .withRequestBody(R"({
        "required": true,
        "content": {
          "application/json": {
            "schema": {
              "type": "object",
              "description": "WiFi configuration request body",
              "required": ["ssid", "password"],
              "properties": {
                "ssid": {
                  "type": "string",
                  "minLength": 1,
                  "maxLength": 32,
                  "description": 'WiFi network name (SSID)'
                },
                "password": {
                  "type": "string",
                  "minLength": 8,
                  "maxLength": 63,
                  "description": "WiFi network password"
                }
              }
            }
          }
        }
      })")
      .withResponseExample(R"({
        "success": true,
        "message": "WiFi configuration updated. Attempting to connect...",
        "status": "connecting"
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("WiFi configuration result"));
}
