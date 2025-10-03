#include "interface/web_response.h"
#include "interface/webserver_typedefs.h"
#include "storage/storage_manager.h"
#include "utilities/debug_macros.h"
#include "web_platform.h"

#include <WebServer.h>
#include <esp_http_server.h>
// Forward declare WebServerClass - the actual implementation is in
// web_platform.h

WebResponse::WebResponse()
    : statusCode(200), mimeType("text/html"), headersSent(false),
      responseSent(false), progmemData(nullptr), isProgmemContent(false),
      jsonDoc(nullptr), isJsonContent(false), isStorageStreamContent(false) {}

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
  this->isStorageStreamContent = false;
}

void WebResponse::setProgmemContent(const char *progmemData,
                                    const String &mimeType) {
  if (responseSent || !progmemData)
    return;

  this->progmemData = progmemData;
  this->mimeType = mimeType;
  this->isProgmemContent = true;
  this->isStorageStreamContent = false;
  this->content = ""; // Clear regular content
}

void WebResponse::setStorageStreamContent(const String &collection,
                                          const String &key,
                                          const String &mimeType,
                                          const String &driverName) {
  if (responseSent || collection.isEmpty() || key.isEmpty())
    return;

  this->storageCollection = collection;
  this->storageKey = key;
  this->storageDriverName = driverName.isEmpty() ? "littlefs" : driverName;
  this->mimeType = mimeType;
  this->isStorageStreamContent = true;
  this->isProgmemContent = false;
  this->progmemData = nullptr;
  this->content = ""; // Clear regular content
}

void WebResponse::setJsonContent(const JsonDocument &doc) {
  if (responseSent)
    return;

  setStatus(200);
  setHeader("Content-Type", "application/json");
  // Set a flag to indicate this should be streamed
  // We'll handle the actual streaming in sendTo methods
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

  // Send all custom headers
  for (const auto &header : headers) {
    server->sendHeader(header.first, header.second);
  }

  markHeadersSent();

  // Send response - use streaming for PROGMEM, JSON, or storage content
  if (isProgmemContent && progmemData != nullptr) {
    sendProgmemChunked(progmemData, server);
  } else if (isJsonContent && jsonDoc != nullptr) {
    streamJsonContent(*jsonDoc, server);
  } else if (isStorageStreamContent) {
    streamFromStorage(storageCollection, storageKey, server, storageDriverName);
  } else {
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

  // Send response body - use streaming for PROGMEM, JSON, or storage content
  esp_err_t ret;
  if (isProgmemContent && progmemData != nullptr) {
    ret = sendProgmemChunked(progmemData, req);
  } else if (isJsonContent && jsonDoc != nullptr) {
    ret = streamJsonContent(*jsonDoc, req);
  } else if (isStorageStreamContent) {
    ret = streamFromStorage(storageCollection, storageKey, req,
                            storageDriverName);
  } else {
    ret = httpd_resp_send(req, content.c_str(), content.length());
  }

  if (ret == ESP_OK) {
    markResponseSent();
  }

  return ret;
}

void WebResponse::streamJsonContent(const JsonDocument &doc,
                                    WebServerClass *server) {
  // Create a wrapper that implements Print interface
  class ServerPrint : public Print {
    WebServerClass *server;
    String buffer;

  public:
    ServerPrint(WebServerClass *s) : server(s) {
      buffer.reserve(512); // Small buffer for chunking
    }

    size_t write(uint8_t c) override {
      buffer += (char)c;
      if (buffer.length() >= 500) { // Flush when buffer gets full
        flush();
      }
      return 1;
    }

    void flush() {
      if (buffer.length() > 0) {
        // Send chunk (you'd need chunked transfer encoding)
        server->sendContent(buffer);
        buffer = "";
      }
    }
  };

  ServerPrint printer(server);
  server->setContentLength(CONTENT_LENGTH_UNKNOWN); // Enable chunked encoding
  server->send(statusCode, mimeType, "");

  serializeJson(doc, printer); // Streams directly!
  printer.flush();
}

esp_err_t WebResponse::streamJsonContent(const JsonDocument &doc,
                                         httpd_req *req) {
  class HttpdPrint : public Print {
    httpd_req *req;
    char buffer[512];
    size_t pos = 0;

  public:
    HttpdPrint(httpd_req *r) : req(r) {}

    size_t write(uint8_t c) override {
      buffer[pos++] = c;
      if (pos >= sizeof(buffer) - 1) {
        flush();
      }
      return 1;
    }

    void flush() {
      if (pos > 0) {
        httpd_resp_send_chunk(req, buffer, pos);
        pos = 0;
      }
    }
  };

  // Set chunked encoding
  httpd_resp_set_type(req, mimeType.c_str());
  httpd_resp_set_status(req, std::to_string(statusCode).c_str());

  HttpdPrint printer(req);
  serializeJson(doc, printer); // Streams directly!
  printer.flush();

  // End chunked response
  return httpd_resp_send_chunk(req, nullptr, 0);
}

// PROGMEM streaming implementation for Arduino WebServer
void WebResponse::sendProgmemChunked(const char *data, WebServerClass *server) {
  if (!data || !server)
    return;

  size_t len = strlen_P(data);
  const size_t CHUNK_SIZE = 512;

  DEBUG_PRINTLN("PROGMEM content length: " + String(len));

  if (len == 0) {
    WARN_PRINTLN("WARNING: Zero-length PROGMEM content detected!");
    return;
  }

  // Pre-calculate content length and send it directly instead of chunked
  server->setContentLength(len);
  server->send(statusCode, mimeType, "");

  // Allocate buffer once and reuse
  char *buffer = (char *)malloc(CHUNK_SIZE + 1);
  if (!buffer) {
    ERROR_PRINTLN("ERROR: Failed to allocate buffer for PROGMEM streaming");
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

  DEBUG_PRINTLN("PROGMEM streaming completed, buffer freed");
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
    ERROR_PRINTLN(
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
    DEBUG_PRINTLN("PROGMEM HTTPS streaming completed, buffer freed");
  } else {
    ERROR_PRINTLN("ERROR: PROGMEM HTTPS streaming failed, buffer freed");
  }

  return ret;
}

// Storage streaming implementation for Arduino WebServer
void WebResponse::streamFromStorage(const String &collection, const String &key,
                                    WebServerClass *server,
                                    const String &driverName) {
  if (!server || collection.isEmpty() || key.isEmpty()) {
    server->send(500, "text/plain",
                 "Internal server error: invalid storage parameters");
    return;
  }

  // Get specified storage driver for streaming
  String targetDriver = driverName.isEmpty() ? "littlefs" : driverName;
  IDatabaseDriver *driver = &StorageManager::driver(targetDriver);
  if (!driver) {
    server->send(500, "text/plain",
                 "Storage driver '" + targetDriver + "' unavailable");
    return;
  }

  // Try to get content length without loading full content
  if (!driver->exists(collection, key)) {
    server->send(404, "application/json",
                 "{\"error\":\"Content not found in storage\"}");
    return;
  }

  // For large files, we'll still need to load due to WebServer limitations
  // but with improved memory management
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(statusCode, mimeType, "");

  const size_t STORAGE_CHUNK_SIZE = 1024;

  // Load data with improved memory management
  String data = driver->retrieve(collection, key);
  if (data.isEmpty()) {
    server->sendContent(
        "{\"error\":\"Failed to retrieve content from storage\"}");
    return;
  }

  // Stream efficiently without additional buffer allocation
  size_t dataLen = data.length();
  for (size_t i = 0; i < dataLen; i += STORAGE_CHUNK_SIZE) {
    size_t chunkLen = min(STORAGE_CHUNK_SIZE, dataLen - i);
    String chunk = data.substring(i, i + chunkLen);
    server->sendContent(chunk);

    // Yield periodically to prevent watchdog timeout
    if (i % (STORAGE_CHUNK_SIZE * 8) == 0) {
      yield();
    }
  }

  // Clear data string to free memory immediately
  data = "";
  DEBUG_PRINTLN("Storage streaming completed for WebServer");
}

// Storage streaming implementation for ESP-IDF HTTPS server
esp_err_t WebResponse::streamFromStorage(const String &collection,
                                         const String &key, httpd_req *req,
                                         const String &driverName) {
  if (!req || collection.isEmpty() || key.isEmpty()) {
    const char *error = "{\"error\":\"Invalid storage parameters\"}";
    return httpd_resp_send(req, error, HTTPD_RESP_USE_STRLEN);
  }

  // Get specified storage driver for streaming
  String targetDriver = driverName.isEmpty() ? "littlefs" : driverName;
  IDatabaseDriver *driver = &StorageManager::driver(targetDriver);
  if (!driver) {
    String errorMsg =
        "{\"error\":\"Storage driver '" + targetDriver + "' unavailable\"}";
    return httpd_resp_send(req, errorMsg.c_str(), errorMsg.length());
  }

  // Check if content exists without loading it
  if (!driver->exists(collection, key)) {
    const char *error = "{\"error\":\"Content not found in storage\"}";
    return httpd_resp_send(req, error, HTTPD_RESP_USE_STRLEN);
  }

  const size_t STORAGE_CHUNK_SIZE = 1024;
  char *buffer = (char *)malloc(STORAGE_CHUNK_SIZE + 1);
  if (!buffer) {
    const char *error = "{\"error\":\"Memory allocation failed\"}";
    return httpd_resp_send(req, error, HTTPD_RESP_USE_STRLEN);
  }

  // Load data with improved error handling
  String data = driver->retrieve(collection, key);
  if (data.isEmpty()) {
    free(buffer);
    const char *error =
        "{\"error\":\"Failed to retrieve content from storage\"}";
    return httpd_resp_send(req, error, HTTPD_RESP_USE_STRLEN);
  }

  // Stream the data in chunks with improved error handling
  esp_err_t ret = ESP_OK;
  size_t dataLen = data.length();

  for (size_t i = 0; i < dataLen && ret == ESP_OK; i += STORAGE_CHUNK_SIZE) {
    size_t chunkLen = min(STORAGE_CHUNK_SIZE, dataLen - i);

    // Use more efficient string to buffer conversion
    String chunk = data.substring(i, i + chunkLen);
    chunk.toCharArray(buffer, chunkLen + 1);

    ret = httpd_resp_send_chunk(req, buffer, chunkLen);

    if (ret != ESP_OK) {
      ERROR_PRINTF("WebResponse: Chunk send failed at position %d/%d\n", i,
                   dataLen);
      break;
    }

    // Yield periodically to prevent watchdog timeout
    if (i % (STORAGE_CHUNK_SIZE * 8) == 0) {
      yield();
    }
  }

  free(buffer);

  // Clear data string to free memory immediately
  data = "";

  if (ret == ESP_OK) {
    // End chunked transfer
    ret = httpd_resp_send_chunk(req, NULL, 0);
    if (ret != ESP_OK) {
      ERROR_PRINTLN("WebResponse: Failed to end chunked transfer");
    }
  } else {
    ERROR_PRINTLN("WebResponse: Storage streaming failed for HTTPS");
  }

  return ret;
}