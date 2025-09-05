#include "../../include/interface/web_module_interface.h"
#include "../../include/storage/auth_storage.h"

// Initialize static variables
std::vector<NavigationItem> IWebModule::navigationMenu;
String IWebModule::currentPath = "";
std::map<int, String> IWebModule::errorPages;
std::vector<RedirectRule> IWebModule::redirectRules;

// Phase 2: Navigation Menu System
void IWebModule::setNavigationMenu(const std::vector<NavigationItem> &items) {
  navigationMenu = items;
}

std::vector<NavigationItem> IWebModule::getNavigationMenu() {
  return navigationMenu;
}

void IWebModule::setCurrentPath(const String &path) { currentPath = path; }

String IWebModule::getCurrentPath() { return currentPath; }

String IWebModule::generateNavigationHtml() {
  if (navigationMenu.empty()) {
    return "";
  }

  String html = "<div class=\"nav-links\">\n";
  for (const auto &item : navigationMenu) {
    html += "  <a href=\"" + item.url + "\"";

    // Auto-detect active state by comparing with current path
    if (!currentPath.isEmpty() &&
        (currentPath == item.url ||
         (item.url != "/" && currentPath.startsWith(item.url)))) {
      html += " class=\"active\"";
    }

    if (item.target.length() > 0) {
      html += " target=\"" + item.target + "\"";
    }

    html += ">" + item.name + "</a>\n";
  }
  html += "</div>\n";

  return html;
}

// Error Page Customization
void IWebModule::setErrorPage(int statusCode, const String &html) {
  errorPages[statusCode] = html;
}

String IWebModule::getErrorPage(int statusCode) {
  auto it = errorPages.find(statusCode);
  if (it != errorPages.end()) {
    return it->second;
  }

  // Return default error page if no custom page is set
  return generateDefaultErrorPage(statusCode);
}

String IWebModule::generateDefaultErrorPage(int statusCode,
                                            const String &message) {
  // Common error messages
  String title, description;
  String statusClass = "error"; // Default to error class for styling

  switch (statusCode) {
  case 400:
    title = "400 Bad Request";
    description = message.isEmpty()
                      ? "The request could not be understood by the server."
                      : message;
    break;
  case 401:
    title = "401 Unauthorized";
    description = message.isEmpty()
                      ? "Authentication is required to access this resource."
                      : message;
    break;
  case 403:
    title = "403 Forbidden";
    description = message.isEmpty()
                      ? "You don't have permission to access this resource."
                      : message;
    break;
  case 404:
    title = "404 Page Not Found";
    description = message.isEmpty()
                      ? "The requested page could not be found on this server."
                      : message;
    break;
  case 405:
    title = "405 Method Not Allowed";
    description = message.isEmpty()
                      ? "The request method is not allowed for this resource."
                      : message;
    break;
  case 500:
    title = "500 Internal Server Error";
    description = message.isEmpty()
                      ? "The server encountered an unexpected condition."
                      : message;
    break;
  case 502:
    title = "502 Bad Gateway";
    description = message.isEmpty() ? "The server received an invalid "
                                      "response from an upstream server."
                                    : message;
    break;
  case 503:
    title = "503 Service Unavailable";
    description =
        message.isEmpty() ? "The server is temporarily unavailable." : message;
    statusClass = "warning"; // Service unavailable is more of a warning
    break;
  default:
    title = String(statusCode) + " Error";
    description = message.isEmpty()
                      ? "An error occurred while processing your request."
                      : message;
    break;
  }

  // Generate HTML error page that uses static asset CSS
  String html = "<!DOCTYPE html>\n";
  html += "<html lang=\"en\">\n";
  html += "<head>\n";
  html += "  <meta charset=\"UTF-8\">\n";
  html += "  <meta name=\"viewport\" content=\"width=device-width, "
          "initial-scale=1.0\">\n";
  html += "  <title>" + title + "</title>\n";
  html += "  <link rel=\"stylesheet\" href=\"/assets/style.css\">\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "  <div class=\"container\">\n";
  html += "    {{NAV_MENU}}\n";
  html += "    <div class=\"error-page\">\n";
  html += "      <h1 class=\"" + statusClass + "\">" + title + "</h1>\n";
  html += "      <p class=\"error-description\">" + description + "</p>\n";
  html += "      <div class=\"status-message " + statusClass + "\">\n";
  html += "        <strong>What can you do?</strong><br>\n";
  html += "        • Check the URL for typos<br>\n";
  html += "        • Use the navigation menu above<br>\n";
  html += "        • Return to the home page\n";
  html += "      </div>\n";
  html += "      <div class=\"error-actions button-group\">\n";
  html +=
      "        <a href=\"/\" class=\"btn btn-primary\">Return to Home</a>\n";
  html += "        <a href=\"javascript:history.back()\" class=\"btn "
          "btn-secondary\">Go Back</a>\n";
  html += "      </div>\n";
  html += "    </div>\n";
  html += "  </div>\n";
  html += "</body>\n";
  html += "</html>\n";

  // DO NOT auto-inject navigation - we'll do that explicitly when we use the
  // error page This allows the caller to set the current path first
  return html;
}

// Route Redirection System (simplified for embedded use)
void IWebModule::addRedirect(const String &fromPath, const String &toPath) {
  // Simple exact path matching only - no dynamic manipulation needed
  redirectRules.push_back(RedirectRule(fromPath, toPath));
}

String IWebModule::getRedirectTarget(const String &requestPath) {
  // Simple exact path matching only
  for (const auto &rule : redirectRules) {
    if (rule.fromPath == requestPath) {
      return rule.toPath;
    }
  }

  // No redirect found
  return "";
}