#include "../../include/interface/web_response.h"
#include "../../include/interface/webserver_typedefs.h"
#include "../../include/web_platform.h"

#if defined(ESP32)
#include <WebServer.h>
#include <esp_http_server.h>
// Forward declare WebServerClass - the actual implementation is in
// web_platform.h
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
// Forward declare WebServerClass - the actual implementation is in
// web_platform.h
#endif

WebResponse::WebResponse()
    : statusCode(200), mimeType("text/html"), headersSent(false),
      responseSent(false) {}

void WebResponse::setStatus(int code) {
  if (headersSent)
    return;
  statusCode = code;
}

void WebResponse::setContent(const String &content, const String &mimeType) {
  if (responseSent)
    return;
  this->content = content;
  this->mimeType = mimeType;
}

void WebResponse::setHeader(const String &name, const String &value) {
  if (headersSent)
    return;
  headers[name] = value;
}

void WebResponse::redirect(const String &url, int code) {
  if (headersSent)
    return;

  setStatus(code);
  setHeader("Location", url);
  setContent("Redirecting...", "text/plain");
}

String WebResponse::getHeader(const String &name) const {
  auto it = headers.find(name);
  return (it != headers.end()) ? it->second : String();
}

// Send response to Arduino WebServer
void WebResponse::sendTo(WebServerClass *server) {
  if (!server || responseSent)
    return;

  // Send all custom headers
  for (const auto &header : headers) {
    server->sendHeader(header.first, header.second);
  }

  markHeadersSent();

  // Send response
  server->send(statusCode, mimeType, content);

  markResponseSent();
}
// Send response to ESP-IDF HTTPS server
#if defined(ESP32)
esp_err_t WebResponse::sendTo(httpd_req *req) {
  if (!req || responseSent)
    return ESP_FAIL;

  // Set status
  char status_str[8];
  snprintf(status_str, sizeof(status_str), "%d", statusCode);
  httpd_resp_set_status(req, status_str);

  // Set content type
  httpd_resp_set_type(req, mimeType.c_str());

  // Set custom headers
  for (const auto &header : headers) {
    httpd_resp_set_hdr(req, header.first.c_str(), header.second.c_str());
  }

  markHeadersSent();

  // Send response body
  esp_err_t ret = httpd_resp_send(req, content.c_str(), content.length());

  if (ret == ESP_OK) {
    markResponseSent();
  }

  return ret;
}
#endif