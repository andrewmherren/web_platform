#include "../../include/storage/auth_storage.h"
#include "../../include/web_platform.h"


String WebPlatform::prepareHtml(String html, WebRequest req,
                                const String &csrfToken) {
  // Navigation menu injection
  if (html.indexOf("{{NAV_MENU}}") >= 0) {
    String navHtml = generateNavigationHtml();
    html.replace("{{NAV_MENU}}", navHtml);
  }

  // csrf token injection
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

  // device name injection
  html.replace("{{DEVICE_NAME}}", getDeviceName());
  return html;
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

  String processedContent = prepareHtml(response.getContent(), request);
  response.setContent(processedContent, response.getMimeType());
}
