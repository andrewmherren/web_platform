#if defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)

#include "esp_https_server.h"
#include "wifi_ap.h"
#include <WiFi.h>

// Handler for HTTP to HTTPS redirect
// This is used in the HTTP server to redirect all traffic to HTTPS when enabled
void handleHttpsRedirect(WebServerClass &server) {
  String httpsUrl = "https://" + WiFi.localIP().toString() + server.uri();
  server.sendHeader("Location", httpsUrl, true);
  server.send(301, "text/plain", "Redirecting to HTTPS...");
}

// Generic HTTPS handler for the ESP-IDF HTTPS server
esp_err_t httpsGenericHandler(httpd_req_t *req) {
  // You can parse the URL from req->uri to handle different endpoints
  const char *resp_str = "<html><body>"
                         "<h1>ESP32-S3 HTTPS Server</h1>"
                         "<p>This request was served over HTTPS.</p>"
                         "<p>This is a secure connection.</p>"
                         "<p><a href='/'>Home</a></p>"
                         "<p><a href='/wifi'>WiFi Settings</a></p>"
                         "</body></html>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, resp_str, strlen(resp_str));

  return ESP_OK;
}

// API endpoint handler for HTTPS
esp_err_t httpsApiHandler(httpd_req_t *req) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "{\"status\":\"success\",\"message\":\"HTTPS API is "
           "running\",\"secure\":true}");

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, buffer, strlen(buffer));

  return ESP_OK;
}

// Register basic HTTPS handlers
void registerHttpsHandlers(httpd_handle_t server) {
  // Root handler
  httpd_uri_t uri_root = {.uri = "/",
                          .method = HTTP_GET,
                          .handler = httpsGenericHandler,
                          .user_ctx = NULL};
  httpd_register_uri_handler(server, &uri_root);

  // API handler
  httpd_uri_t uri_api = {.uri = "/api/*",
                         .method = HTTP_GET,
                         .handler = httpsApiHandler,
                         .user_ctx = NULL};
  httpd_register_uri_handler(server, &uri_api);
}

#endif // defined(ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
