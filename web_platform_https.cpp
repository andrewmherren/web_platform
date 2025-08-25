#include "web_platform.h"

// HTTPS certificate detection and server configuration
// This file handles the core HTTPS server setup and certificate management

bool WebPlatform::areCertificatesAvailable() {
#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    Serial.println("WebPlatform: No embedded certificates found");
    return false;
  }

  // Basic validation - check for PEM format
  if (cert_len > 27 && key_len > 27) {
    String certStart((char *)cert_data, 27);
    String keyStart((char *)key_data, 27);

    if (certStart.indexOf("-----BEGIN CERTIFICATE-----") >= 0 &&
        keyStart.indexOf("-----BEGIN") >= 0) {
      Serial.printf("WebPlatform: SSL certificates validated (cert: %d bytes, "
                    "key: %d bytes)\n",
                    cert_len, key_len);
      return true;
    }
  }

  Serial.println("WebPlatform: Invalid certificate format");
  return false;
#else
  Serial.println("WebPlatform: Certificates not supported on this platform");
  return false;
#endif
}

bool WebPlatform::getEmbeddedCertificates(const uint8_t **cert_data,
                                          size_t *cert_len,
                                          const uint8_t **key_data,
                                          size_t *key_len) {
#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
  // Check for embedded certificates - these symbols may not exist if
  // certificates weren't embedded
  extern const uint8_t server_cert_pem_start[] asm(
      "_binary_src_server_cert_pem_start") __attribute__((weak));
  extern const uint8_t server_cert_pem_end[] asm(
      "_binary_src_server_cert_pem_end") __attribute__((weak));
  extern const uint8_t server_key_pem_start[] asm(
      "_binary_src_server_key_pem_start") __attribute__((weak));
  extern const uint8_t server_key_pem_end[] asm(
      "_binary_src_server_key_pem_end") __attribute__((weak));

  // Check if certificates are available (weak symbols may be NULL)
  if (!server_cert_pem_start || !server_cert_pem_end || !server_key_pem_start ||
      !server_key_pem_end) {
    return false;
  }

  // Calculate sizes and set pointers
  *cert_len = server_cert_pem_end - server_cert_pem_start;
  *key_len = server_key_pem_end - server_key_pem_start;
  *cert_data = server_cert_pem_start;
  *key_data = server_key_pem_start;

  // Basic sanity check
  return (*cert_len > 100 && *key_len > 100);
#else
  // Not supported on this platform
  return false;
#endif
}

void WebPlatform::configureHttpsServer() {
#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
  if (httpsServerHandle) {
    Serial.println("WebPlatform: HTTPS server already running");
    return;
  }

  // Get certificate data
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    Serial.println("WebPlatform: Failed to get certificates for HTTPS");
    httpsEnabled = false;
    return;
  }

  // Clear any existing route paths
  httpsRoutePaths.clear();

  // Configure HTTPS server
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.httpd.server_port = serverPort;
  config.httpd.max_uri_handlers = 50; // Generous limit
  config.httpd.task_priority = 5;
  config.httpd.stack_size = 8192;
  config.httpd.lru_purge_enable = true;

  // Set certificates
  config.cacert_pem = cert_data;
  config.cacert_len = cert_len;
  config.prvtkey_pem = key_data;
  config.prvtkey_len = key_len;

  // Start HTTPS server
  esp_err_t ret = httpd_ssl_start(&httpsServerHandle, &config);
  if (ret != ESP_OK) {
    Serial.printf("WebPlatform: Failed to start HTTPS server: %d\n", ret);
    httpsServerHandle = nullptr;
    httpsEnabled = false;
    return;
  }

  Serial.println("WebPlatform: HTTPS server started successfully");
  
  // Route registration is handled by web_platform_https_routes.cpp
  registerHttpsRoutes();
#else
  Serial.println("WebPlatform: HTTPS not supported on this platform");
  httpsEnabled = false;
#endif
}