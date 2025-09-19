#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"

String WebPlatform::prepareHtml(String html, WebRequest req,
                                const String &csrfToken) {
  Serial.println("Original content size: " + String(html.length()));

  // Optimized template processing with minimal heap allocations
  const size_t inputLength = html.length();
  if (inputLength == 0) {
    return html;
  }

  // More accurate size estimation based on typical template expansion
  // NAV_MENU (~300-500 bytes), SECURITY_NOTICE (~200-300), others smaller
  const size_t estimatedExpansion = 800;
  String result;
  result.reserve(inputLength + estimatedExpansion);

  // Cache for replacement values - computed only once when needed
  struct TemplateCache {
    String navHtml;
    String csrfTokenValue;
    String securityNotice;
    String username;
    String modulePrefix;
    String deviceName;
    bool computed = false;
  } cache;

  // Pre-compute values that are always needed
  cache.modulePrefix = req.getModuleBasePath();
  cache.deviceName = getDeviceName();

  const char *src = html.c_str();
  size_t pos = 0;

  while (pos < inputLength) {
    // Fast path: look for template marker start
    const char *markerStart = strchr(src + pos, '{');
    if (!markerStart) {
      // No more markers, copy rest of string
      result += (src + pos);
      break;
    }

    // Copy content before marker
    size_t copyLen = markerStart - (src + pos);
    if (copyLen > 0) {
      result.concat(src + pos, copyLen);
    }

    pos = markerStart - src;

    // Check for template marker
    if (pos < inputLength - 1 && src[pos] == '{' && src[pos + 1] == '{') {
      // Find closing marker
      const char *markerEnd = strstr(src + pos + 2, "}}");
      if (markerEnd) {
        size_t markerLen = (markerEnd + 2) - (src + pos);

        // Extract marker content efficiently
        size_t contentStart = pos + 2;
        size_t contentLen = (markerEnd - src) - contentStart;

        // Lazy computation of auth-dependent values
        if (!cache.computed) {
          const AuthContext &auth = req.getAuthContext();
          bool isAuthenticated = auth.hasValidSession();

          // Fallback session check for routes without auth requirement
          if (!isAuthenticated) {
            const String sessionCookie = req.getHeader("Cookie");
            int sessionStart = sessionCookie.indexOf("session=");
            if (sessionStart >= 0) {
              sessionStart += 8;
              int sessionEnd = sessionCookie.indexOf(";", sessionStart);
              if (sessionEnd < 0)
                sessionEnd = sessionCookie.length();
              String sessionId =
                  sessionCookie.substring(sessionStart, sessionEnd);
              isAuthenticated = (AuthStorage::validateSession(
                                     sessionId, req.getClientIp()) != "");
            }
          }

          cache.navHtml = IWebModule::generateNavigationHtml(isAuthenticated);
          cache.csrfTokenValue =
              csrfToken.isEmpty()
                  ? AuthStorage::createPageToken(req.getClientIp())
                  : csrfToken;

          // PROGMEM security notices to avoid heap allocation
          if (isHttpsEnabled()) {
            cache.securityNotice = F(R"(<div class="security-notice https">
            <h4><span class="security-icon-large">🔒</span> Secure Connection</h4>
            <p>This connection is secured with HTTPS encryption. Your WiFi password will be transmitted securely.</p>
        </div>)");
          } else {
            cache.securityNotice = F(R"(<div class="security-notice">
            <h4><span class="security-icon-large">ℹ️</span> Connection Notice</h4>
            <p>This is a direct device connection. Only enter WiFi credentials on your trusted private network.</p>
        </div>)");
          }

          cache.username = auth.username;
          cache.computed = true;
        }

        // Fast marker matching using first character and length
        const char *markerContent = src + contentStart;
        switch (contentLen) {
        case 8: // NAV_MENU
          if (strncmp(markerContent, "NAV_MENU", 8) == 0) {
            result += cache.navHtml;
          } else if (strncmp(markerContent, "username", 8) == 0) {
            result += cache.username;
          } else {
            result.concat(src + pos, markerLen); // Unknown marker
          }
          break;
        case 9: // csrfToken
          if (strncmp(markerContent, "csrfToken", 9) == 0) {
            result += cache.csrfTokenValue;
          } else {
            result.concat(src + pos, markerLen);
          }
          break;
        case 11: // DEVICE_NAME
          if (strncmp(markerContent, "DEVICE_NAME", 11) == 0) {
            result += cache.deviceName;
          } else {
            result.concat(src + pos, markerLen);
          }
          break;
        case 13: // MODULE_PREFIX
          if (strncmp(markerContent, "MODULE_PREFIX", 13) == 0) {
            result += cache.modulePrefix;
          } else {
            result.concat(src + pos, markerLen);
          }
          break;
        case 15: // SECURITY_NOTICE
          if (strncmp(markerContent, "SECURITY_NOTICE", 15) == 0) {
            result += cache.securityNotice;
          } else {
            result.concat(src + pos, markerLen);
          }
          break;
        default:
          // Unknown marker, preserve as-is
          result.concat(src + pos, markerLen);
          break;
        }

        pos += markerLen;
      } else {
        // Incomplete marker, copy single character
        result += src[pos];
        pos++;
      }
    } else if (pos <= inputLength - 6 && strncmp(src + pos, "<head>", 6) == 0) {
      // CSRF meta tag injection
      if (!cache.computed) {
        cache.csrfTokenValue =
            csrfToken.isEmpty()
                ? AuthStorage::createPageToken(req.getClientIp())
                : csrfToken;
        cache.computed = true;
      }

      result += F("<head>\n    <meta name=\"csrf-token\" content=\"");
      result += cache.csrfTokenValue;
      result += F("\">");
      pos += 6;
    } else {
      // Regular character
      result += src[pos];
      pos++;
    }
  }

  Serial.println("Processed content size: " + String(result.length()));
  return result;
}

// Template processing helpers
bool WebPlatform::shouldProcessResponse(const WebResponse &response) {
  // Skip processing if explicitly disabled via
  // header
  if (response.getHeader("X-Skip-Template-Processing") == "true") {
    return false;
  }

  // Only process responses with these content
  // types
  String contentType = response.getMimeType();
  return contentType == "text/html" || contentType == "text/plain";
}

void WebPlatform::processResponseTemplates(WebRequest &request,
                                           WebResponse &response) {

  if (!shouldProcessResponse(response)) {
    return;
  }

  String content;

  // Check if we have PROGMEM content
  if (response.hasProgmemContent() && response.getProgmemData() != nullptr) {
    // Convert PROGMEM content to String for processing
    Serial.println("Converting PROGMEM content for template processing");
    content = FPSTR(response.getProgmemData());
  } else {
    // Use regular content
    content = response.getContent();
  }

  Serial.println("Processing templates for response, content length: " +
                 String(content.length()));

  // Process templates only if we have content
  if (content.length() > 0) {
    String processedContent = prepareHtml(content, request);
    // Always store as regular content after processing
    response.setContent(processedContent, response.getMimeType());
  }
}

void WebPlatform::measureHeapUsage(const char *phase) {
#if defined(ESP32)
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t heapSize = ESP.getHeapSize();
  float heapUsagePercent = 100.0 - (freeHeap * 100.0 / heapSize);

  Serial.printf("=== Heap Usage: %s ===\n", phase);
  Serial.printf("Free heap: %d bytes\n", freeHeap);
  Serial.printf("Total heap: %d bytes\n", heapSize);
  Serial.printf("Heap usage: %.1f%%\n", heapUsagePercent);
  Serial.printf("==========================\n");
#elif defined(ESP8266)
  uint32_t freeHeap = ESP.getFreeHeap();
  // ESP8266 doesn't have getHeapSize(), estimate based on chip model
  uint32_t estimatedHeapSize = 80000; // Typical for ESP8266 with 4MB flash
  float heapUsagePercent = 100.0 - (freeHeap * 100.0 / estimatedHeapSize);

  Serial.printf("=== Heap Usage: %s ===\n", phase);
  Serial.printf("Free heap: %d bytes\n", freeHeap);
  Serial.printf("Estimated total heap: %d bytes\n", estimatedHeapSize);
  Serial.printf("Heap usage: %.1f%% (estimated)\n", heapUsagePercent);
  Serial.printf("==========================\n");
#endif
}