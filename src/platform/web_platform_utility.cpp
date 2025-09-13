#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"

String WebPlatform::prepareHtml(
    String html, WebRequest req,
    const String
        &csrfToken) { // Navigation menu injection with authentication awareness
  if (html.indexOf("{{NAV_MENU}}") >= 0) {
    const AuthContext &auth = req.getAuthContext();
    bool isAuthenticated = auth.hasValidSession();

    // Manual session check as a fallback for routes that don't require auth
    if (!isAuthenticated) {
      // Try to extract session from cookie for pages that don't require auth
      String sessionCookie = req.getHeader("Cookie");
      if (sessionCookie.indexOf("session=") >= 0) {
        int start = sessionCookie.indexOf("session=") + 8;
        int end = sessionCookie.indexOf(";", start);
        if (end < 0)
          end = sessionCookie.length();
        String sessionId = sessionCookie.substring(start, end);

        // Check if this is a valid session
        if (AuthStorage::validateSession(sessionId, req.getClientIp()) != "") {
          isAuthenticated = true;
        }
      }
    }

    String navHtml = IWebModule::generateNavigationHtml(isAuthenticated);
    html.replace("{{NAV_MENU}}", navHtml);
  }

  // Automatic CSRF meta tag injection for HTML
  // documents
  if (html.indexOf("<head>") >= 0) {
    String token = csrfToken == ""
                       ? AuthStorage::createPageToken(req.getClientIp())
                       : csrfToken;
    String csrfMetaTag = "\n    <meta name=\"csrf-token\" "
                         "content=\"" +
                         token + "\">";
    html.replace("<head>", "<head>" + csrfMetaTag);
  }

  // Legacy csrf token bookmark injection (for
  // backwards compatibility)
  if (html.indexOf("{{csrfToken}}") >= 0) {
    String token = csrfToken == ""
                       ? AuthStorage::createPageToken(req.getClientIp())
                       : csrfToken;
    html.replace("{{csrfToken}}", token);
  }

  // security notice injection
  if (html.indexOf("{{SECURITY_NOTICE}}") >= 0) {
    String securityNotice;
    if (isHttpsEnabled()) {
      securityNotice = R"(
        <div class="security-notice https">
            <h4><span class="security-icon-large">🔒</span> Secure Connection</h4>
            <p>This connection is secured with HTTPS encryption. Your WiFi password will be transmitted securely.</p>
        </div>)";
    } else {
      securityNotice = R"(
        <div class="security-notice">
            <h4><span class="security-icon-large">ℹ️</span> Connection Notice</h4>
            <p>This is a direct device connection. Only enter WiFi credentials on your trusted private network.</p>
        </div>)";
    }
    html.replace("{{SECURITY_NOTICE}}", securityNotice);
  }

  // authenticated user name injection
  if (html.indexOf("{{username}}") >= 0) {
    const AuthContext &auth = req.getAuthContext();
    html.replace("{{username}}", auth.username);
  }

  html.replace("{{MODULE_PREFIX}}", req.getModuleBasePath());

  // device name injection
  html.replace("{{DEVICE_NAME}}", getDeviceName());
  return html;
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

  String processedContent = prepareHtml(response.getContent(), request);
  response.setContent(processedContent, response.getMimeType());
}
