#ifndef WEB_REQUEST_H
#define WEB_REQUEST_H

// This include is already in Arduino.h, but added here to help linters
#if defined(__CLANG_LINT__)
#include <WString.h>
#endif

#include "auth_types.h"
#include "web_module_types.h"
#include <Arduino.h>
#include <map>

// Common HTTP headers that should be collected by web servers
extern const char* COMMON_HTTP_HEADERS[];
extern const size_t COMMON_HTTP_HEADERS_COUNT;

// Forward declarations to avoid circular dependencies
#if defined(ESP32) || defined(ESP8266)
class WebServerClass;
#endif

#if defined(ESP32)
struct httpd_req;
#endif

/**
 * WebRequest - Unified request abstraction for HTTP/HTTPS handlers
 *
 * Part of the web_module_interface to provide a consistent interface
 * for accessing request data across Arduino WebServer and ESP-IDF
 * HTTP server implementations without modules needing to know about
 * WebPlatform internals.
 */
class WebRequest {
private:
  String path;
  WebModule::Method method;
  String body;
  String clientIp;
  std::map<String, String> params;
  std::map<String, String> headers;
  std::map<String, String> jsonParams;
  AuthContext authContext; // Authentication information

public:
  // Constructor for HTTP server (Arduino WebServer)
#if defined(ESP32) || defined(ESP8266)
  WebRequest(WebServerClass *server);
#endif

  // Constructor for HTTPS server (ESP-IDF)
#if defined(ESP32)
  WebRequest(httpd_req *req);
#endif

  // Request information
  String getPath() const { return path; }
  WebModule::Method getMethod() const { return method; }
  String getBody() const { return body; }
  String getClientIp() const { return clientIp; }

  // URL parameters (query string and POST form data)
  String getParam(const String &name) const;
  bool hasParam(const String &name) const;
  std::map<String, String> getAllParams() const { return params; }

  // Headers
  String getHeader(const String &name) const;
  bool hasHeader(const String &name) const;

  // Convenience methods
  String getQueryString() const;
  String getContentType() const;
  size_t getContentLength() const;

  // JSON parameter access
  String getJsonParam(const String &name) const;
  bool hasJsonParam(const String &name) const;

  // Authentication context
  const AuthContext &getAuthContext() const { return authContext; }
  void setAuthContext(const AuthContext &context) { authContext = context; }

private:
  void parseQueryParams(const String &query);
  void parseFormData(const String &formData);
  void parseJsonData(const String &jsonData);
  void parseRequestBody(const String &body, const String &contentType);
  void parseHeaders();
#if defined(ESP32)
  void parseClientIp(httpd_req *req);
#endif
};

#endif // WEB_REQUEST_H