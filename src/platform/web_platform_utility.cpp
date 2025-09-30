#include "storage/auth_storage.h"
#include "web_platform.h"

// Forward declaration for body tag processing
static String parseAndMergeBodyTag(const char *bodyTagStart,
                                   const String &modulePrefix,
                                   const String &deviceName);

String WebPlatform::prepareHtml(String html, WebRequest req,
                                const String &csrfToken) {
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

  // Cache for replacement values
  struct TemplateCache {
    String navHtml;
    String csrfTokenValue;
    String securityNotice;
    String username;
    String modulePrefix;
    String deviceName;
    bool authComputed = false;

    void computeAuthValues(const WebRequest &req, const String &csrfToken,
                           const WebPlatform *platform) {
      if (authComputed)
        return;

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
          String sessionId = sessionCookie.substring(sessionStart, sessionEnd);
          isAuthenticated = (AuthStorage::validateSession(
                                 sessionId, req.getClientIp()) != "");
        }
      }

      navHtml = webPlatform.generateNavigationHtml(isAuthenticated);

      csrfTokenValue = csrfToken.isEmpty()
                           ? AuthStorage::createPageToken(req.getClientIp())
                           : csrfToken;

      // PROGMEM security notices to avoid heap allocation
      if (platform->isHttpsEnabled()) {
        securityNotice = F(R"(<div class="security-notice https">
        <h4><span class="security-icon-large">🔒</span> Secure Connection</h4>
        <p>This connection is secured with HTTPS encryption. Your WiFi password will be transmitted securely.</p>
    </div>)");
      } else {
        securityNotice = F(R"(<div class="security-notice">
        <h4><span class="security-icon-large">ℹ️</span> Connection Notice</h4>
        <p>This is a direct device connection. Only enter WiFi credentials on your trusted private network.</p>
    </div>)");
      }

      username = auth.username;
      authComputed = true;
    }
  } cache;

  // Pre-compute values that are always needed
  cache.modulePrefix = req.getModuleBasePath();
  cache.deviceName = getDeviceName();

  const char *src = html.c_str();
  size_t pos = 0;

  while (pos < inputLength) {
    // Look for next template marker, head tag, or body tag
    const char *markerStart = strchr(src + pos, '{');
    const char *headStart = nullptr;
    const char *bodyStart = nullptr;

    // Look for <head> tag from current position
    if (pos <= inputLength - 6) {
      headStart = strstr(src + pos, "<head>");
    }

    // Look for <body tag from current position (allowing for attributes)
    if (pos <= inputLength - 5) {
      const char *searchStart = src + pos;
      const char *currentPos = strstr(searchStart, "<body");
      if (currentPos) {
        // Verify this is actually a body tag (not part of another word)
        char nextChar = currentPos[5];
        if (nextChar == '>' || nextChar == ' ' || nextChar == '\t' ||
            nextChar == '\n' || nextChar == '\r') {
          bodyStart = currentPos;
        }
      }
    }

    // Determine which comes first: template marker, head tag, or body tag
    size_t nextPos = inputLength; // Default to end of string
    bool isTemplateMarker = false;
    bool isHeadTag = false;
    bool isBodyTag = false;

    if (markerStart && (!headStart || markerStart < headStart) &&
        (!bodyStart || markerStart < bodyStart)) {
      nextPos = markerStart - src;
      isTemplateMarker = true;
    } else if (headStart && (!bodyStart || headStart < bodyStart)) {
      nextPos = headStart - src;
      isHeadTag = true;
    } else if (bodyStart) {
      nextPos = bodyStart - src;
      isBodyTag = true;
    }

    // Copy content before the next marker/tag
    if (nextPos > pos) {
      result.concat(src + pos, nextPos - pos);
      pos = nextPos;
    }

    // If no more markers or tags, we're done
    if (nextPos >= inputLength) {
      break;
    }

    // Process template marker
    if (isTemplateMarker && pos < inputLength - 1 && src[pos] == '{' &&
        src[pos + 1] == '{') {
      // Find closing marker
      const char *markerEnd = strstr(src + pos + 2, "}}");
      if (markerEnd) {
        size_t markerLen = (markerEnd + 2) - (src + pos);

        // Extract marker content efficiently
        size_t contentStart = pos + 2;
        size_t contentLen = (markerEnd - src) - contentStart;

        // Ensure auth-dependent values are computed
        cache.computeAuthValues(req, csrfToken, this);

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
        case 11: // DEVICE_NAME, redirectUrl
          if (strncmp(markerContent, "DEVICE_NAME", 11) == 0) {
            result += cache.deviceName;
          } else if (strncmp(markerContent, "redirectUrl", 11) == 0) {
            // Check for redirectUrl parameter
            String redirectParam = req.getParam("redirect");
            if (redirectParam.isEmpty()) {
              redirectParam = "/";
            }
            result += redirectParam;
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
    } else if (isHeadTag) {
      // CSRF meta tag injection
      cache.computeAuthValues(req, csrfToken, this);

      result += F("<head>\n    <meta name=\"csrf-token\" content=\"");
      result += cache.csrfTokenValue;
      result += F("\">");
      pos += 6;
    } else if (isBodyTag) {
      // Body tag processing with data attribute injection
      cache.computeAuthValues(req, csrfToken, this);

      String processedBodyTag =
          parseAndMergeBodyTag(src + pos, cache.modulePrefix, cache.deviceName);
      result += processedBodyTag;

      // Find the end of the body tag to update position
      const char *bodyTagEnd = strchr(src + pos, '>');
      if (bodyTagEnd) {
        pos = (bodyTagEnd + 1) - src;
      } else {
        pos += 5; // fallback, should not happen with valid HTML
      }
    } else {
      // This shouldn't happen with new logic, but safety fallback
      result += src[pos];
      pos++;
    }
  }

  return result;
}

// Template processing helpers
bool WebPlatform::shouldProcessResponse(const WebResponse &response) {
  // Skip processing if explicitly disabled via header
  if (response.getHeader("X-Skip-Template-Processing") == "true") {
    return false;
  }

  // Only process responses with these content types
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
    content = FPSTR(response.getProgmemData());
  } else {
    // Use regular content
    content = response.getContent();
  }

  // Process templates only if we have content
  if (content.length() > 0) {
    String processedContent = prepareHtml(content, request);
    // Always store as regular content after processing
    response.setContent(processedContent, response.getMimeType());
  }
}

void WebPlatform::measureHeapUsage(const char *phase) {

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t heapSize = ESP.getHeapSize();
  float heapUsagePercent = 100.0 - (freeHeap * 100.0 / heapSize);

  DEBUG_PRINTF("=== Heap Usage: %s ===\n", phase);
  DEBUG_PRINTF("Free heap: %d bytes\n", freeHeap);
  DEBUG_PRINTF("Total heap: %d bytes\n", heapSize);
  DEBUG_PRINTF("Heap usage: %.1f%%\n", heapUsagePercent);
  DEBUG_PRINTF("==========================\n");
}

// Parse and merge body tag with data attributes
static String parseAndMergeBodyTag(const char *bodyTagStart,
                                   const String &modulePrefix,
                                   const String &deviceName) {
  // Find the end of the body tag
  const char *tagEnd = strchr(bodyTagStart, '>');
  if (!tagEnd) {
    // Malformed HTML, return original
    return String(bodyTagStart).substring(0, 5); // Just "<body"
  }

  // Extract the full body tag content
  size_t tagLength = (tagEnd - bodyTagStart) + 1;
  String originalTag = String(bodyTagStart).substring(0, tagLength);

  // Parse existing attributes
  String existingAttributes = "";
  if (tagLength > 6) { // More than just "<body>"
    // Extract everything between "<body" and ">"
    existingAttributes = originalTag.substring(5, tagLength - 1);
    existingAttributes.trim();
  }

  // Build new attributes while preserving existing ones
  String newAttributes = "";

  // Add our required data attributes first
  newAttributes += " data-module-prefix=\"" + modulePrefix + "\"";
  newAttributes += " data-device-name=\"" + deviceName + "\"";

  // Process existing attributes, skipping our reserved ones
  if (existingAttributes.length() > 0) {
    // Simple attribute parsing - split by spaces and rebuild
    String remainingAttrs = existingAttributes;
    remainingAttrs.replace("data-module-prefix=",
                           "SKIP_data-module-prefix="); // Mark for removal
    remainingAttrs.replace("data-device-name=",
                           "SKIP_data-device-name="); // Mark for removal

    // Split and rebuild attributes
    int startPos = 0;
    while (startPos < remainingAttrs.length()) {
      // Find next attribute
      int spacePos = remainingAttrs.indexOf(' ', startPos);
      if (spacePos == -1)
        spacePos = remainingAttrs.length();

      String attr = remainingAttrs.substring(startPos, spacePos);
      attr.trim();

      // Only include if not marked for skipping
      if (attr.length() > 0 && !attr.startsWith("SKIP_")) {
        newAttributes += " " + attr;
      }

      startPos = spacePos + 1;
    }
  }

  return "<body" + newAttributes + ">";
}