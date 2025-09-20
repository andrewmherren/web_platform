#include "../../include/interface/web_response.h"
#include "../../include/interface/webserver_typedefs.h"
#include "../../include/web_platform.h"

#include <WebServer.h>
#include <esp_http_server.h>
// Forward declare WebServerClass - the actual implementation is in
// web_platform.h

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
  if (responseSent || !progmemData)
    return;

  // Add debug output
  Serial.print("Setting PROGMEM content, length: ");
  Serial.println(strlen_P(progmemData));

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

String WebResponse::getContent() const { return content; }

// Send response to Arduino WebServer
void WebResponse::sendTo(WebServerClass *server) {
  if (!server || responseSent)
    return;

  // Add debug output
  if (isProgmemContent && progmemData) {
    Serial.print("Sending PROGMEM content, length: ");
    Serial.println(strlen_P(progmemData));
  } else {
    Serial.print("Sending regular content, length: ");
    Serial.println(content.length());
  }

  // Send all custom headers
  for (const auto &header : headers) {
    server->sendHeader(header.first, header.second);
  }

  markHeadersSent();

  // Send response - use streaming for PROGMEM content
  if (isProgmemContent && progmemData != nullptr) {
    Serial.println("Using PROGMEM streaming");
    sendProgmemChunked(progmemData, server);
  } else {
    Serial.println("Using regular content send");
    server->send(statusCode, mimeType, content);
  }

  markResponseSent();
}

// Send response to ESP-IDF HTTPS server
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

  // Mark headers as sent
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

// JSON streaming implementation (future enhancement)
void WebResponse::streamJsonContent(const JsonDocument &doc,
                                    WebServerClass *server) {
  // This would stream JSON directly without creating a String
  // For now, fall back to String serialization
  String jsonString;
  serializeJson(doc, jsonString);
  server->send(statusCode, mimeType, jsonString);
}

esp_err_t WebResponse::streamJsonContent(const JsonDocument &doc,
                                         httpd_req *req) {
  // This would stream JSON directly without creating a String
  // For now, fall back to String serialization
  String jsonString;
  serializeJson(doc, jsonString);
  return httpd_resp_send(req, jsonString.c_str(), jsonString.length());
}

// PROGMEM streaming implementation for Arduino WebServer
void WebResponse::sendProgmemChunked(const char *data, WebServerClass *server) {
  if (!data || !server)
    return;

  size_t len = strlen_P(data);
  const size_t CHUNK_SIZE = 512;

  Serial.println("PROGMEM content length: " + String(len));

  if (len == 0) {
    Serial.println("WARNING: Zero-length PROGMEM content detected!");
    return;
  }

  // Pre-calculate content length and send it directly instead of chunked
  server->setContentLength(len);
  server->send(statusCode, mimeType, "");

  // Allocate buffer once and reuse
  char *buffer = (char *)malloc(CHUNK_SIZE + 1);
  if (!buffer) {
    Serial.println("ERROR: Failed to allocate buffer for PROGMEM streaming");
    return;
  }

  // Send data in chunks with explicit buffer management
  for (size_t i = 0; i < len; i += CHUNK_SIZE) {
    size_t chunk_len = min(CHUNK_SIZE, len - i);
    memcpy_P(buffer, data + i, chunk_len);
    buffer[chunk_len] = 0;
    server->sendContent(buffer);

    // Yield to prevent watchdog timeout on large files
    if (i % (CHUNK_SIZE * 10) == 0) {
      yield();
    }
  }

  // Clean up allocated memory
  free(buffer);

  Serial.println("PROGMEM streaming completed, buffer freed");
}

// PROGMEM streaming implementation for ESP-IDF HTTPS server
esp_err_t WebResponse::sendProgmemChunked(const char *data, httpd_req *req) {
  if (!data || !req)
    return ESP_FAIL;

  size_t len = strlen_P(data);
  const size_t CHUNK_SIZE = 512;

  // Allocate buffer once and reuse
  char *buffer = (char *)malloc(CHUNK_SIZE + 1);
  if (!buffer) {
    Serial.println(
        "ERROR: Failed to allocate buffer for PROGMEM HTTPS streaming");
    return ESP_ERR_NO_MEM;
  }

  // Send data in chunks with explicit buffer management
  esp_err_t ret = ESP_OK;
  for (size_t i = 0; i < len && ret == ESP_OK; i += CHUNK_SIZE) {
    size_t chunk_len = min(CHUNK_SIZE, len - i);
    memcpy_P(buffer, data + i, chunk_len);
    buffer[chunk_len] = 0;

    ret = httpd_resp_send_chunk(req, buffer, chunk_len);

    // Yield periodically to prevent watchdog timeout
    if (i % (CHUNK_SIZE * 10) == 0) {
      yield();
    }
  }

  // Clean up allocated memory
  free(buffer);

  if (ret == ESP_OK) {
    // End chunked transfer
    ret = httpd_resp_send_chunk(req, NULL, 0);
    Serial.println("PROGMEM HTTPS streaming completed, buffer freed");
  } else {
    Serial.println("ERROR: PROGMEM HTTPS streaming failed, buffer freed");
  }

  return ret;
}
