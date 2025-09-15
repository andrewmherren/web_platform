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
      responseSent(false), progmemData(nullptr), isProgmemContent(false) {}

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
  this->isProgmemContent = false;
  this->progmemData = nullptr;
}

void WebResponse::setProgmemContent(const char *progmemData,
                                    const String &mimeType) {
  if (responseSent)
    return;
  this->progmemData = progmemData;
  this->mimeType = mimeType;
  this->isProgmemContent = true;
  this->content = ""; // Clear regular content
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

  // Send response - use streaming for PROGMEM content
  if (isProgmemContent && progmemData != nullptr) {
    sendProgmemChunked(progmemData, server);
  } else {
    server->send(statusCode, mimeType, content);
  }

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

  // Send response body - use streaming for PROGMEM content
  esp_err_t ret;
  if (isProgmemContent && progmemData != nullptr) {
    ret = sendProgmemChunked(progmemData, req);
  } else {
    ret = httpd_resp_send(req, content.c_str(), content.length());
  }

  if (ret == ESP_OK) {
    markResponseSent();
  }

  return ret;
}
#endif

// PROGMEM streaming implementation for Arduino WebServer
void WebResponse::sendProgmemChunked(const char *data, WebServerClass *server) {
  if (!data || !server)
    return;

  size_t len = strlen_P(data);
  const size_t CHUNK_SIZE = 1024;

  // Set content length unknown to enable chunked transfer
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(statusCode, mimeType, "");

  // Send data in chunks
  for (size_t i = 0; i < len; i += CHUNK_SIZE) {
    char buffer[CHUNK_SIZE + 1];
    size_t chunk_len = min(CHUNK_SIZE, len - i);
    memcpy_P(buffer, data + i, chunk_len);
    buffer[chunk_len] = 0;
    server->sendContent(buffer);
  }

  // End chunked transfer
  server->sendContent("");
}

#if defined(ESP32)
// PROGMEM streaming implementation for ESP-IDF HTTPS server
esp_err_t WebResponse::sendProgmemChunked(const char *data, httpd_req *req) {
  if (!data || !req)
    return ESP_FAIL;

  size_t len = strlen_P(data);
  const size_t CHUNK_SIZE = 1024;

  // Send data in chunks
  for (size_t i = 0; i < len; i += CHUNK_SIZE) {
    char buffer[CHUNK_SIZE + 1];
    size_t chunk_len = min(CHUNK_SIZE, len - i);
    memcpy_P(buffer, data + i, chunk_len);
    buffer[chunk_len] = 0;

    esp_err_t ret = httpd_resp_send_chunk(req, buffer, chunk_len);
    if (ret != ESP_OK)
      return ret;
  }

  // End chunked transfer
  return httpd_resp_send_chunk(req, NULL, 0);
}
#endif
