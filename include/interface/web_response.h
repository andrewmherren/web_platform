#ifndef WEB_RESPONSE_H
#define WEB_RESPONSE_H

#include <Arduino.h>
#include <map>

// Forward declarations to avoid circular dependencies
class WebServerClass;

#if defined(ESP32)
struct httpd_req;
typedef int esp_err_t;
#endif

/**
 * WebResponse - Unified response abstraction for HTTP/HTTPS handlers
 *
 * Part of the web_module_interface to provide a consistent interface
 * for sending responses across Arduino WebServer and ESP-IDF HTTP
 * server implementations without modules needing to know about
 * WebPlatform internals.
 */
class WebResponse {
private:
  int statusCode;
  String content;
  String mimeType;
  std::map<String, String> headers;
  bool headersSent;
  bool responseSent;

public:
  WebResponse();

  // Response configuration
  void setStatus(int code);
  void setContent(const String &content, const String &mimeType = "text/html");
  void setHeader(const String &name, const String &value);
  void redirect(const String &url, int code = 302);

  // Send response (called internally by WebPlatform)
  void sendTo(WebServerClass *server);

#if defined(ESP32)
  esp_err_t sendTo(httpd_req *req);
#endif

  // Status queries
  bool isHeadersSent() const { return headersSent; }
  bool isResponseSent() const { return responseSent; }
  int getStatus() const { return statusCode; }
  String getContent() const { return content; }
  String getMimeType() const { return mimeType; }

  // Header access
  String getHeader(const String &name) const;
  std::map<String, String> getAllHeaders() const { return headers; }

private:
  void markHeadersSent() { headersSent = true; }
  void markResponseSent() { responseSent = true; }

  // Allow WebPlatform to call private methods
  friend class WebPlatform;
};

#endif // WEB_RESPONSE_H