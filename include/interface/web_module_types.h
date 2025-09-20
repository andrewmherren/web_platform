#ifndef WEB_MODULE_TYPES_H
#define WEB_MODULE_TYPES_H

#include <WebServer.h>

// Use a completely different namespace to avoid conflicts with ESP32 built-in
// HTTP enums
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
inline String wmMethodToString(WebModule::Method method) {
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

inline HTTPMethod wmMethodToHttpMethod(WebModule::Method method) {
  return (method == WebModule::WM_GET)      ? HTTP_GET
         : (method == WebModule::WM_POST)   ? HTTP_POST
         : (method == WebModule::WM_PUT)    ? HTTP_PUT
         : (method == WebModule::WM_PATCH)  ? HTTP_PATCH
         : (method == WebModule::WM_DELETE) ? HTTP_DELETE
                                            : HTTP_GET;
}

inline WebModule::Method httpMethodToWMMethod(HTTPMethod method) {
  if (method == HTTP_GET)
    return WebModule::WM_GET;
  if (method == HTTP_POST)
    return WebModule::WM_POST;
  if (method == HTTP_PUT)
    return WebModule::WM_PUT;
  if (method == HTTP_DELETE)
    return WebModule::WM_DELETE;
  if (method == HTTP_PATCH)
    return WebModule::WM_PATCH;
  return WebModule::WM_GET; // Default fallback
}

#endif // WEB_MODULE_TYPES_H