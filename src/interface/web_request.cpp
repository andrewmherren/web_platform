#include "../../include/interface/web_request.h"
#include "../../include/interface/webserver_typedefs.h"
#include "../../include/storage/auth_storage.h"
#include "../../include/models/data_models.h"
#include <ArduinoJson.h>

#if defined(ESP32)
#include <WebServer.h>
#include <arpa/inet.h>
#include <esp_http_server.h>
#include <netinet/in.h>
#include <sys/socket.h>
// For IN6_IS_ADDR_V4MAPPED macro
#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a)                                                \
  (((const uint32_t *)(a))[0] == 0 && ((const uint32_t *)(a))[1] == 0 &&       \
   ((const uint32_t *)(a))[2] == htonl(0xffff))
#endif
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#endif

const char *COMMON_HTTP_HEADERS[] = {"Host",
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
const size_t COMMON_HTTP_HEADERS_COUNT =
    sizeof(COMMON_HTTP_HEADERS) / sizeof(COMMON_HTTP_HEADERS[0]);// Constructor for Arduino WebServer
WebRequest::WebRequest(WebServerClass *server) {
  if (!server)
    return;

  // Extract path without query parameters
  String fullUri = server->uri();
  int queryStart = fullUri.indexOf('?');
  path = (queryStart >= 0) ? fullUri.substring(0, queryStart) : fullUri;
  method = httpMethodToWMMethod(server->method());

  // Parse URL parameters (query string)
  for (int i = 0; i < server->args(); i++) {
    params[server->argName(i)] = server->arg(i);
  }
  
  // Parse headers
  for (size_t i = 0; i < COMMON_HTTP_HEADERS_COUNT; i++) {
    headers[COMMON_HTTP_HEADERS[i]] = server->header(COMMON_HTTP_HEADERS[i]);
  }

  // Get request body for POST, PUT, PATCH requests
  if (server->method() == HTTP_POST || server->method() == HTTP_PUT ||
      server->method() == HTTP_PATCH) {
    body = server->arg("plain");
    // Parse request body based on content type
    String contentType = getHeader("Content-Type");
    parseRequestBody(body, contentType);
  }

  // Parse ClientIp
  clientIp = headers["X-Forwarded-For"];
  if (clientIp.isEmpty()) {
    clientIp = server->client().remoteIP().toString();
  }

  // Always check for session information (for UI state, not authentication)
  checkSessionInformation();
}

// Constructor for ESP-IDF HTTPS server
#if defined(ESP32)
WebRequest::WebRequest(httpd_req *req) {
  if (!req)
    return;

  // Extract path without query parameters
  String fullUri = String(req->uri);
  int queryStart = fullUri.indexOf('?');
  path = (queryStart >= 0) ? fullUri.substring(0, queryStart) : fullUri;

  method = httpMethodToWMMethod((HTTPMethod)req->method);

  // Parse query string
  size_t query_len = httpd_req_get_url_query_len(req);
  if (query_len > 0) {
    char *query = new char[query_len + 1];
    if (httpd_req_get_url_query_str(req, query, query_len + 1) == ESP_OK) {
      parseQueryParams(String(query));
    }
    delete[] query;
  }

  for (const char *headerName : COMMON_HTTP_HEADERS) {
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

  // Get request body for POST, PUT, PATCH requests
  if ((req->method == HTTP_POST || req->method == HTTP_PUT ||
       req->method == HTTP_PATCH) &&
      req->content_len > 0) {
    char *content = new char[req->content_len + 1];
    int received = httpd_req_recv(req, content, req->content_len);
    if (received > 0) {
      content[received] = '\0';
      body = String(content);

      // Parse request body based on content type
      String contentType = getHeader("Content-Type");
      parseRequestBody(body, contentType);
    }
    delete[] content;
  }

  // Parse ClientIp
  clientIp = headers["X-Forwarded-For"];
  if (clientIp.isEmpty()) {
    parseClientIp(req);
  }

  // Always check for session information (for UI state, not authentication)
  checkSessionInformation();
}
#endif

String WebRequest::getParam(const String &name) const {
  auto it = params.find(name);
  return (it != params.end()) ? it->second : String();
}

bool WebRequest::hasParam(const String &name) const {
  return params.find(name) != params.end();
}

String WebRequest::getJsonParam(const String &name) const {
  auto it = jsonParams.find(name);
  return (it != jsonParams.end()) ? it->second : String();
}

bool WebRequest::hasJsonParam(const String &name) const {
  return jsonParams.find(name) != jsonParams.end();
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

void WebRequest::parseJsonData(const String &jsonData) {
  Serial.println("Parsing: " + jsonData);
  if (jsonData.length() == 0)
    return;

  // Use ArduinoJson for robust JSON parsing
  // Allocate a document with sufficient capacity (adjust size as needed)
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonData);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract all key-value pairs from the JSON object
  if (doc.is<JsonObject>()) {
    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair kv : obj) {
      String key = kv.key().c_str();
      String value = "";

      // Convert different JSON value types to strings
      if (kv.value().is<const char *>()) {
        value = kv.value().as<const char *>();
      } else if (kv.value().is<int>()) {
        value = String(kv.value().as<int>());
      } else if (kv.value().is<float>()) {
        value = String(kv.value().as<float>());
      } else if (kv.value().is<bool>()) {
        value = kv.value().as<bool>() ? "true" : "false";
      }

      jsonParams[key] = value;
    }
  }
}

void WebRequest::parseRequestBody(const String &body,
                                  const String &contentType) {
  if (body.length() == 0)
    return;

  if (contentType.indexOf("application/x-www-form-urlencoded") >= 0) {
    parseFormData(body);
  } else if (contentType.indexOf("application/json") >= 0) {
    parseJsonData(body);
  }
}

#if defined(ESP32)
void WebRequest::parseClientIp(httpd_req *req) {
  int sockfd = httpd_req_to_sockfd(req);
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  if (sockfd >= 0 && getpeername(sockfd, (struct sockaddr *)&client_addr,
                                 &client_addr_len) == 0) {
    if (client_addr.ss_family == AF_INET) {
      // Pure IPv4
      struct sockaddr_in *addr_in = (struct sockaddr_in *)&client_addr;
      char ipStr[INET_ADDRSTRLEN];
      if (inet_ntop(AF_INET, &addr_in->sin_addr, ipStr, INET_ADDRSTRLEN) !=
          NULL) {
        clientIp = String(ipStr);
      }
    } else if (client_addr.ss_family == AF_INET6) {
      // IPv6 or IPv4-mapped IPv6
      struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&client_addr;

      // Check if this is an IPv4-mapped IPv6 address (::FFFF:x.x.x.x)
      if (IN6_IS_ADDR_V4MAPPED(&addr_in6->sin6_addr)) {
        // Extract the IPv4 address from the IPv4-mapped IPv6 address
        // The IPv4 address is in the last 4 bytes of the IPv6 address
        uint32_t ipv4_addr = *((uint32_t *)&addr_in6->sin6_addr.s6_addr[12]);
        struct in_addr ipv4_in_addr;
        ipv4_in_addr.s_addr = ipv4_addr;

        char ipStr[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &ipv4_in_addr, ipStr, INET_ADDRSTRLEN) != NULL) {
          clientIp = String(ipStr);
        }
      } else {
        // Pure IPv6 address
        char ipStr[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &addr_in6->sin6_addr, ipStr,
                      INET6_ADDRSTRLEN) != NULL) {
          clientIp = String(ipStr);
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

// Path parameter extraction helpers
String WebRequest::getPathSegment(int index) const {
  if (index < 0)
    return String();

  String pathCopy = path;
  // Remove leading slash if present
  if (pathCopy.startsWith("/")) {
    pathCopy = pathCopy.substring(1);
  }

  int currentIndex = 0;
  int start = 0;
  int slashPos = pathCopy.indexOf('/');

  while (currentIndex < index && slashPos >= 0) {
    start = slashPos + 1;
    slashPos = pathCopy.indexOf('/', start);
    currentIndex++;
  }

  if (currentIndex == index) {
    if (slashPos >= 0) {
      return pathCopy.substring(start, slashPos);
    } else {
      return pathCopy.substring(start);
    }
  }

  return String();
}

String WebRequest::getLastPathSegment() const {
  int lastSlash = path.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < (int)path.length() - 1) {
    return path.substring(lastSlash + 1);
  }
  return String();
}

String WebRequest::getPathParameter(const String &routePattern) const {
  // Simple single parameter extraction for patterns like "/api/token/*"
  // where we want the parameter after the last fixed segment

  // Find the position of the wildcard or parameter marker
  int wildcardPos = routePattern.indexOf('*');
  if (wildcardPos < 0) {
    // No wildcard found, try to match exact pattern
    return String();
  }

  // Extract the fixed prefix (everything before the wildcard)
  String prefix = routePattern.substring(0, wildcardPos);

  // Check if our path starts with this prefix
  if (path.startsWith(prefix)) {
    // Return everything after the prefix
    String param = path.substring(prefix.length());
    // Remove leading slash if present
    if (param.startsWith("/")) {
      param = param.substring(1);
    }
    return param;
  }

  return String();
}

String WebRequest::getPathParameter(const String &routePattern,
                                    const String &paramName) const {
  // More advanced parameter extraction for patterns like
  // "/api/user/{userId}/token/{tokenId}" This is a simplified implementation -
  // could be expanded for more complex routing

  String pattern = routePattern;
  String pathCopy = path;

  // Replace parameter placeholders with wildcards for simple matching
  String paramPlaceholder = "{" + paramName + "}";
  int paramPos = pattern.indexOf(paramPlaceholder);

  if (paramPos < 0) {
    return String(); // Parameter not found in pattern
  }

  // Find the segment that contains our parameter
  String beforeParam = pattern.substring(0, paramPos);
  String afterParam = pattern.substring(paramPos + paramPlaceholder.length());

  // Simple extraction - find the parameter value between the before and after
  // parts
  if (pathCopy.startsWith(beforeParam)) {
    int valueStart = beforeParam.length();
    int valueEnd = pathCopy.length();

    if (afterParam.length() > 0) {
      int afterPos = pathCopy.indexOf(afterParam, valueStart);
      if (afterPos >= 0) {
        valueEnd = afterPos;
      }
    }

    if (valueEnd > valueStart) {
      return pathCopy.substring(valueStart, valueEnd);
    }
  }

  return String();
}

// Convenience method that uses the matched route pattern
String WebRequest::getRouteParameter(const String &paramName) const {
  if (matchedRoutePattern.isEmpty()) {
    return String(); // No matched route pattern available
  }

  // Simple Laravel-style parameter extraction by comparing route pattern with
  // actual path Example: pattern "/api/token/{tokenId}" with path
  // "/api/token/abc123" should extract "abc123"

  // Split both pattern and path into segments separated by '/'
  int patternStart = matchedRoutePattern.startsWith("/") ? 1 : 0;
  int pathStart = path.startsWith("/") ? 1 : 0;

  String pattern = matchedRoutePattern.substring(patternStart);
  String actualPath = path.substring(pathStart);

  // Simple approach: split by '/' and compare segment by segment
  int patternPos = 0, pathPos = 0;
  int patternLen = pattern.length(), pathLen = actualPath.length();

  while (patternPos < patternLen && pathPos < pathLen) {
    // Find next segment in pattern
    int patternSegmentEnd = pattern.indexOf('/', patternPos);
    if (patternSegmentEnd == -1)
      patternSegmentEnd = patternLen;

    // Find next segment in path
    int pathSegmentEnd = actualPath.indexOf('/', pathPos);
    if (pathSegmentEnd == -1)
      pathSegmentEnd = pathLen;

    String patternSegment = pattern.substring(patternPos, patternSegmentEnd);
    String pathSegment = actualPath.substring(pathPos, pathSegmentEnd);

    // Check if this segment is a parameter (enclosed in braces)
    if (patternSegment.startsWith("{") && patternSegment.endsWith("}")) {
      String segmentParamName =
          patternSegment.substring(1, patternSegment.length() - 1);
      if (segmentParamName == paramName) {
        return pathSegment; // Found the matching parameter
      }
    }

    // Move to next segment
    patternPos = patternSegmentEnd + 1;
    pathPos = pathSegmentEnd + 1;
  }

  return String(); // Parameter not found
}

// Helper method to check session information for UI state (not authentication)
void WebRequest::checkSessionInformation() {
  // Extract session cookie to check if user is logged in (for UI state only)
  String sessionCookie = getHeader("Cookie");
  if (sessionCookie.indexOf("session=") >= 0) {
    int start = sessionCookie.indexOf("session=") + 8;
    int end = sessionCookie.indexOf(";", start);
    if (end < 0)
      end = sessionCookie.length();
    String sessionId = sessionCookie.substring(start, end);
    
    // Simple check to see if session exists (no IP validation for UI state)
    // We validate without IP checking for UI purposes only
    if (AuthStorage::validateSession(sessionId)) {
      // Get the session details for UI context
      AuthSession session = AuthStorage::findSession(sessionId);
      if (session.isValid()) {
        // Set basic auth context for UI rendering purposes only
        authContext.isAuthenticated = true;
        authContext.authenticatedVia = AuthType::SESSION;
        authContext.sessionId = sessionId;
        authContext.username = session.username;
        authContext.authenticatedAt = session.createdAt;
      }
    }
  }
}