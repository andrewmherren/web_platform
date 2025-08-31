#ifndef WEB_MODULE_TYPES_H
#define WEB_MODULE_TYPES_H

// Use a completely different namespace to avoid conflicts with ESP32/ESP8266
// built-in HTTP enums
namespace WebModule {

// HTTP Methods enum - prefixed to avoid conflicts
enum Method {
  WM_GET = 0,
  WM_POST = 1,
  WM_PUT = 2,
  WM_DELETE = 3,
  WM_PATCH = 4
};

} // namespace WebModule

// Utility functions for HTTP method conversion
inline String httpMethodToString(WebModule::Method method) {
  switch (method) {
  case WebModule::WM_GET:
    return "GET";
  case WebModule::WM_POST:
    return "POST";
  case WebModule::WM_PUT:
    return "PUT";
  case WebModule::WM_DELETE:
    return "DELETE";
  case WebModule::WM_PATCH:
    return "PATCH";
  default:
    return "UNKNOWN";
  }
}

inline WebModule::Method stringToHttpMethod(const String &method) {
  if (method == "GET")
    return WebModule::WM_GET;
  if (method == "POST")
    return WebModule::WM_POST;
  if (method == "PUT")
    return WebModule::WM_PUT;
  if (method == "DELETE")
    return WebModule::WM_DELETE;
  if (method == "PATCH")
    return WebModule::WM_PATCH;
  return WebModule::WM_GET; // Default fallback
}

#endif // WEB_MODULE_TYPES_H