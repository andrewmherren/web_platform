#include "web_platform.h"
#include "storage/auth_storage.h"
#include "utilities/debug_macros.h"

// Navigation Menu System
void WebPlatform::setNavigationMenu(const std::vector<NavigationItem> &items) {
  navigationMenu = items;
}

std::vector<NavigationItem> WebPlatform::getNavigationMenu() const {
  return navigationMenu;
}

String WebPlatform::generateNavigationHtml(bool isAuthenticated) const {
  DEBUG_PRINTLN("generateNavigationHtml called, isAuthenticated: " +
                String(isAuthenticated ? "true" : "false"));
  DEBUG_PRINTLN("Navigation menu size: " + String(navigationMenu.size()));

  if (navigationMenu.empty()) {
    DEBUG_PRINTLN("Navigation menu is empty, returning empty string");
    return "";
  }

  // More accurate size estimation based on typical nav items
  // Each item: ~60-80 chars for <a> tag + content
  const size_t estimatedSize = 50 + (navigationMenu.size() * 80);
  String html;
  html.reserve(estimatedSize);

  html = F("<div class=\"nav-links\">\n");

  for (const auto &item : navigationMenu) {
    // Check visibility requirements
    bool shouldShow = true;
    switch (item.visibility) {
    case NavAuthVisibility::AUTHENTICATED:
      shouldShow = isAuthenticated;
      break;
    case NavAuthVisibility::UNAUTHENTICATED:
      shouldShow = !isAuthenticated;
      break;
    case NavAuthVisibility::ALWAYS:
    default:
      shouldShow = true;
      break;
    }

    if (!shouldShow) {
      continue;
    }

    html += F("  <a href=\"");
    html += FPSTR(item.url); // Direct PROGMEM access, no String conversion
    html += F("\"");

    if (strlen_P(item.target) > 0) {
      html += F(" target=\"");
      html += FPSTR(item.target);
      html += F("\"");
    }

    html += F(">");
    html += FPSTR(item.name);
    html += F("</a>\n");
  }
  html += F("</div>\n");

  return html;
}

// Error Page Customization
void WebPlatform::setErrorPage(int statusCode, const String &html) {
  errorPages[statusCode] = html;
}

String WebPlatform::getErrorPage(int statusCode) const {
  auto it = errorPages.find(statusCode);
  if (it != errorPages.end()) {
    return it->second;
  }

  // Return default error page if no custom page is set
  return generateDefaultErrorPage(statusCode);
}

String WebPlatform::generateDefaultErrorPage(int statusCode, const String &message) const {
  // Optimized error page generation with PROGMEM templates

  // Pre-allocate result string with estimated size
  String html;
  html.reserve(1200); // Typical error page is ~1000-1200 bytes

  // Use PROGMEM for static HTML structure
  html = F("<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
           "  <meta charset=\"UTF-8\">\n"
           "  <meta name=\"viewport\" content=\"width=device-width, "
           "initial-scale=1.0\">\n"
           "  <title>");

  // Generate title and description based on status code
  const char *title;
  const char *defaultDesc;
  const char *statusClass;

  switch (statusCode) {
  case 400:
    title = "400 Bad Request";
    defaultDesc = "The request could not be understood by the server.";
    statusClass = "error";
    break;
  case 401:
    title = "401 Unauthorized";
    defaultDesc = "Authentication is required to access this resource.";
    statusClass = "error";
    break;
  case 403:
    title = "403 Forbidden";
    defaultDesc = "You don't have permission to access this resource.";
    statusClass = "error";
    break;
  case 404:
    title = "404 Page Not Found";
    defaultDesc = "The requested page could not be found on this server.";
    statusClass = "error";
    break;
  case 405:
    title = "405 Method Not Allowed";
    defaultDesc = "The request method is not allowed for this resource.";
    statusClass = "error";
    break;
  case 500:
    title = "500 Internal Server Error";
    defaultDesc = "The server encountered an unexpected condition.";
    statusClass = "error";
    break;
  case 502:
    title = "502 Bad Gateway";
    defaultDesc =
        "The server received an invalid response from an upstream server.";
    statusClass = "error";
    break;
  case 503:
    title = "503 Service Unavailable";
    defaultDesc = "The server is temporarily unavailable.";
    statusClass = "warning";
    break;
  default:
    // For unknown status codes, we need to create dynamic title
    html += String(statusCode);
    html += F(" Error</title>\n"
              "  <link rel=\"stylesheet\" href=\"/assets/style.css\">\n"
              "</head>\n<body>\n  <div class=\"container\">\n    {{NAV_MENU}}\n"
              "    <div class=\"error-page\">\n      <h1 class=\"error\">");
    html += String(statusCode);
    html += F(" Error</h1>\n      <p class=\"error-description\">");
    html += message.isEmpty()
                ? F("An error occurred while processing your request.")
                : message;
    html += F("</p>\n");
    // Continue with common error page footer
    html +=
        F("      <div class=\"status-message error\">\n"
          "        <strong>What can you do?</strong><br>\n"
          "        • Check the URL for typos<br>\n"
          "        • Use the navigation menu above<br>\n"
          "        • Return to the home page\n"
          "      </div>\n"
          "      <div class=\"error-actions button-group\">\n"
          "        <a href=\"/\" class=\"btn btn-primary\">Return to Home</a>\n"
          "        <a href=\"javascript:history.back()\" class=\"btn "
          "btn-secondary\">Go Back</a>\n"
          "      </div>\n    </div>\n  </div>\n</body>\n</html>\n");
    return html;
  }

  // For standard status codes, use PROGMEM strings
  html += FPSTR(title);
  html += F("</title>\n  <link rel=\"stylesheet\" href=\"/assets/style.css\">\n"
            "</head>\n<body>\n  <div class=\"container\">\n    {{NAV_MENU}}\n"
            "    <div class=\"error-page\">\n      <h1 class=\"");
  html += FPSTR(statusClass);
  html += F("\">");
  html += FPSTR(title);
  html += F("</h1>\n      <p class=\"error-description\">");

  // Use custom message if provided, otherwise use default
  if (message.isEmpty()) {
    html += FPSTR(defaultDesc);
  } else {
    html += message;
  }

  html += F("</p>\n      <div class=\"status-message ");
  html += FPSTR(statusClass);
  html +=
      F("\">\n        <strong>What can you do?</strong><br>\n"
        "        • Check the URL for typos<br>\n"
        "        • Use the navigation menu above<br>\n"
        "        • Return to the home page\n"
        "      </div>\n"
        "      <div class=\"error-actions button-group\">\n"
        "        <a href=\"/\" class=\"btn btn-primary\">Return to Home</a>\n"
        "        <a href=\"javascript:history.back()\" class=\"btn "
        "btn-secondary\">Go Back</a>\n"
        "      </div>\n    </div>\n  </div>\n</body>\n</html>\n");

  return html;
}

// Route Redirection System (simplified for embedded use)
void WebPlatform::addRedirect(const String &fromPath, const String &toPath) {
  // Simple exact path matching only - no dynamic manipulation needed
  redirectRules.push_back(RedirectRule(fromPath, toPath));
}

String WebPlatform::getRedirectTarget(const String &requestPath) const {
  // Simple exact path matching only
  for (const auto &rule : redirectRules) {
    if (String(rule.fromPath) == requestPath) {
      return String(rule.toPath);
    }
  }

  // No redirect found
  return "";
}