#include "../../include/interface/web_request.h"
#include "../../include/interface/webserver_typedefs.h"

#if defined(ESP32)
#include <WebServer.h>
#include <arpa/inet.h>
#include <esp_http_server.h>
#include <netinet/in.h>
#include <sys/socket.h>
// For IN6_IS_ADDR_V4MAPPED macro
#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) \
    (((const uint32_t *) (a))[0] == 0 \
     && ((const uint32_t *) (a))[1] == 0 \
     && ((const uint32_t *) (a))[2] == htonl (0xffff))
#endif
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

// Constructor for Arduino WebServer
WebRequest::WebRequest(WebServerClass *server) {
  if (!server)
    return;

  path = server->uri();
  method = server->method() == HTTP_GET      ? WebModule::WM_GET
           : server->method() == HTTP_POST   ? WebModule::WM_POST
           : server->method() == HTTP_PUT    ? WebModule::WM_PUT
           : server->method() == HTTP_DELETE ? WebModule::WM_DELETE
           : server->method() == HTTP_PATCH  ? WebModule::WM_PATCH
                                             : WebModule::WM_GET;

  // Get request body for POST requests
  if (server->method() == HTTP_POST) {
    body = server->arg("plain");
  }

  // Parse URL parameters (query string)
  for (int i = 0; i < server->args(); i++) {
    params[server->argName(i)] = server->arg(i);
  }

  // Parse headers
  for (int i = 0; i < server->headers(); i++) {
    headers[server->headerName(i)] = server->header(i);
  }

  // Parse ClientIp
  clientIp = headers["X-Forwarded-For"];
  if (clientIp.isEmpty()) {
    clientIp = server->client().remoteIP().toString();
  }
}

// Constructor for ESP-IDF HTTPS server
#if defined(ESP32)
WebRequest::WebRequest(httpd_req *req) {
  if (!req)
    return;

  path = String(req->uri);

  // Convert HTTP method
  switch (req->method) {
  case HTTP_GET:
    method = WebModule::WM_GET;
    break;
  case HTTP_POST:
    method = WebModule::WM_POST;
    break;
  case HTTP_PUT:
    method = WebModule::WM_PUT;
    break;
  case HTTP_PATCH:
    method = WebModule::WM_PATCH;
    break;
  case HTTP_DELETE:
    method = WebModule::WM_DELETE;
    break;
  default:
    method = WebModule::WM_GET;
    break;
  }

  // Parse query string
  size_t query_len = httpd_req_get_url_query_len(req);
  if (query_len > 0) {
    char *query = new char[query_len + 1];
    if (httpd_req_get_url_query_str(req, query, query_len + 1) == ESP_OK) {
      parseQueryParams(String(query));
    }
    delete[] query;
  }

  // Get request body for POST requests
  if (req->method == HTTP_POST && req->content_len > 0) {
    char *content = new char[req->content_len + 1];
    int received = httpd_req_recv(req, content, req->content_len);
    if (received > 0) {
      content[received] = '\0';
      body = String(content);

      // Parse form data if content type is form-urlencoded
      String contentType = getHeader("Content-Type");
      if (contentType.indexOf("application/x-www-form-urlencoded") >= 0) {
        parseFormData(body);
      }
    }
    delete[] content;
  }

  // Parse headers - ESP-IDF requires requesting headers individually by name
  const char *commonHeaders[] = {"Host",
                                 "User-Agent",
                                 "Accept",
                                 "Accept-Language",
                                 "Accept-Encoding",
                                 "Content-Type",
                                 "Content-Length",
                                 "Authorization",
                                 "Cookie",
                                 "X-CSRF-Token",
                                 "X-Requested-With",
                                 "Referer",
                                 "Cache-Control",
                                 "Connection",
                                 "Pragma"};

  for (const char *headerName : commonHeaders) {
    size_t headerLen = httpd_req_get_hdr_value_len(req, headerName);
    if (headerLen > 0) {
      char *headerValue = new char[headerLen + 1];
      if (httpd_req_get_hdr_value_str(req, headerName, headerValue,
                                      headerLen + 1) == ESP_OK) {
        headers[String(headerName)] = String(headerValue);
      }
      delete[] headerValue;
    }
  }

  // Parse ClientIp
  clientIp = headers["X-Forwarded-For"];
  if (clientIp.isEmpty()) {
    parseClientIp(req);
  }
}
#endif

String WebRequest::getParam(const String &name) const {
  auto it = params.find(name);
  return (it != params.end()) ? it->second : String();
}

bool WebRequest::hasParam(const String &name) const {
  return params.find(name) != params.end();
}

String WebRequest::getHeader(const String &name) const {
  auto it = headers.find(name);
  return (it != headers.end()) ? it->second : String();
}

bool WebRequest::hasHeader(const String &name) const {
  return headers.find(name) != headers.end();
}

String WebRequest::getQueryString() const {
  String query = "";
  bool first = true;

  for (const auto &param : params) {
    if (!first) {
      query += "&";
    }
    query += param.first + "=" + param.second;
    first = false;
  }

  return query;
}

String WebRequest::getContentType() const { return getHeader("Content-Type"); }

size_t WebRequest::getContentLength() const {
  String lengthStr = getHeader("Content-Length");
  return lengthStr.length() > 0 ? lengthStr.toInt() : 0;
}

void WebRequest::parseQueryParams(const String &query) {
  if (query.length() == 0)
    return;

  int start = 0;
  int end = query.indexOf('&');

  while (start < (int)query.length()) {
    String param =
        (end == -1) ? query.substring(start) : query.substring(start, end);

    int equalPos = param.indexOf('=');
    if (equalPos > 0) {
      String key = param.substring(0, equalPos);
      String value = param.substring(equalPos + 1);

      // URL decode (basic implementation)
      value.replace("+", " ");
      value.replace("%20", " ");

      params[key] = value;
    }

    if (end == -1)
      break;

    start = end + 1;
    end = query.indexOf('&', start);
  }
}

void WebRequest::parseFormData(const String &formData) {
  parseQueryParams(formData); // Form data uses same format as query params
}

#if defined(ESP32)
void WebRequest::parseClientIp(httpd_req *req) {
  int sockfd = httpd_req_to_sockfd(req);
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  
  if (sockfd >= 0 && getpeername(sockfd, (struct sockaddr*)&client_addr, &client_addr_len) == 0) {
    if (client_addr.ss_family == AF_INET) {
      // Pure IPv4
      struct sockaddr_in* addr_in = (struct sockaddr_in*)&client_addr;
      char ipStr[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &addr_in->sin_addr, ipStr, INET_ADDRSTRLEN) != NULL) {
        clientIp = String(ipStr);
      }
    } else if (client_addr.ss_family == AF_INET6) {
      // IPv6 or IPv4-mapped IPv6
      struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&client_addr;
      
      // Check if this is an IPv4-mapped IPv6 address (::FFFF:x.x.x.x)
      if (IN6_IS_ADDR_V4MAPPED(&addr_in6->sin6_addr)) {
        // Extract the IPv4 address from the IPv4-mapped IPv6 address
        // The IPv4 address is in the last 4 bytes of the IPv6 address
        uint32_t ipv4_addr = *((uint32_t*)&addr_in6->sin6_addr.s6_addr[12]);
        struct in_addr ipv4_in_addr;
        ipv4_in_addr.s_addr = ipv4_addr;
        
        char ipStr[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &ipv4_in_addr, ipStr, INET_ADDRSTRLEN) != NULL) {
          clientIp = String(ipStr);
        }
      } else {
        // Pure IPv6 address
        char ipStr[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &addr_in6->sin6_addr, ipStr, INET6_ADDRSTRLEN) != NULL) {
          clientIp = String(ipStr);
          Serial.printf("--> Client IP parsed (IPv6): %s\n", clientIp.c_str());
        }
      }
    }
  }
  
  // Final fallback
  if (clientIp.isEmpty()) {
    clientIp = "unknown";
  }
}
#endif