# WebPlatform Authentication Examples

This document provides practical examples for common authentication scenarios using the WebPlatform authentication framework.

## 1. Module Developer: Protecting Private API Routes

As a module developer, you often need to protect certain routes from unauthorized access, while allowing the application developer to access them.

### Use Case: USB PD Controller with Protected Firmware Update API

```cpp
class USBPDController : public IWebModule {
public:
  std::vector<WebRoute> getHttpRoutes() override {
    std::vector<WebRoute> routes;
    
    // Public user interface route - no auth required
    routes.push_back(WebRoute("/", WebModule::WM_GET, 
      [this](WebRequest& req, WebResponse& res) {
        res.setContent(getUserInterfaceHTML(), "text/html");
      }));
    
    // Public status API - no auth required
    routes.push_back(WebRoute("/api/status", WebModule::WM_GET, 
      [this](WebRequest& req, WebResponse& res) {
        res.setContent(getStatusJSON(), "application/json");
      }));
    
    // Protected configuration API - SESSION auth required
    // Only application developer can access this via web interface
    routes.push_back(WebRoute("/api/config", WebModule::WM_POST, 
      [this](WebRequest& req, WebResponse& res) {
        updateConfig(req.getBody());
        res.setContent("{\"success\":true}", "application/json");
      }, "application/json", "Update controller configuration"));
    
    // Protected firmware update API - SESSION auth required
    // Only application developer can access this via web interface
    routes.push_back(WebRoute("/api/firmware", WebModule::WM_POST, 
      [this](WebRequest& req, WebResponse& res) {
        bool success = updateFirmware(req.getBody());
        res.setContent(success ? 
          "{\"success\":true}" : 
          "{\"success\":false,\"error\":\"Firmware update failed\"}", 
          "application/json");
      }, "application/json", "Update controller firmware"));
    
    return routes;
  }
  
  // In the new unified handler system, WebRoute has a new constructor
  // that accepts AuthRequirements directly:
  std::vector<WebRoute> getUnifiedRoutes() {
    std::vector<WebRoute> routes;
    
    // Public user interface route - no auth required
    routes.push_back(WebRoute("/", WebModule::WM_GET, 
      [this](WebRequest& req, WebResponse& res) {
        res.setContent(getUserInterfaceHTML(), "text/html");
      }, "text/html", "User Interface", {AuthType::NONE}));
    
    // Protected configuration API - SESSION auth required
    // Only application developer can access this via web interface
    routes.push_back(WebRoute("/api/config", WebModule::WM_POST, 
      [this](WebRequest& req, WebResponse& res) {
        updateConfig(req.getBody());
        res.setContent("{\"success\":true}", "application/json");
      }, "application/json", "Update controller configuration", {AuthType::SESSION}));
      
    // ... additional routes
    
    return routes;
  }
  
  std::vector<WebRoute> getHttpsRoutes() override {
    return getHttpRoutes(); // Same routes for HTTP and HTTPS
  }
  
  String getModuleName() const override { return "USB PD Controller"; }
  
private:
  String getUserInterfaceHTML() { /* ... */ }
  String getStatusJSON() { /* ... */ }
  void updateConfig(const String& json) { /* ... */ }
  bool updateFirmware(const String& binary) { /* ... */ }
};
```

**Key Points:**
- Public routes have no auth requirements (`AuthType::NONE`)
- Sensitive operations require `AuthType::SESSION` which means only an authenticated application developer can access them
- The application developer needs to enable authentication and create login credentials
- End users cannot access protected routes without authentication

## 2. Application Developer: Adding Login Screen and Global Auth

As an application developer, you often want to protect your entire application behind a login screen to prevent unauthorized access.

### Example: Protecting an Entire Application

```cpp
#include <web_platform.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Setup navigation menu
  std::vector<NavigationItem> navItems = {
    NavigationItem("Home", "/"),
    NavigationItem("USB PD Control", "/usb_pd/"),
    NavigationItem("System", "/system/"),
    NavigationItem("Logout", "/logout")
  };
  IWebModule::setNavigationMenu(navItems);
  
  // Initialize web platform
  webPlatform.begin("SecureDevice");
  
  if (webPlatform.isConnected()) {
    // Register modules
    webPlatform.registerModule("/usb_pd", &usbPDController);
    
    // Override the module's root route to add session auth
    // This protects the entire module by securing its entry point
    webPlatform.overrideRoute("/usb_pd/", [](WebRequest& req, WebResponse& res) {
      // Get the original handler to execute but with auth protection
      usbPDController.getUnifiedRootHandler()(req, res);
    }, {AuthType::SESSION});
    
    // Add system status page (protected)
    webPlatform.registerRoute("/system/", [](WebRequest& req, WebResponse& res) {
      String html = "<html><head><title>System Status</title></head><body>";
      html += "<h1>System Status</h1>";
      html += "<p>Uptime: " + String(millis() / 1000) + " seconds</p>";
      html += "<p>Free Heap: " + String(ESP.getFreeHeap()) + " bytes</p>";
      html += "</body></html>";
      
      res.setContent(html, "text/html");
    }, {AuthType::SESSION});
    
    // Override the home page to add authentication
    webPlatform.overrideRoute("/", [](WebRequest& req, WebResponse& res) {
      String html = "<html><head><title>Home</title></head><body>";
      html += "<h1>Welcome, " + req.getAuthContext().username + "!</h1>";
      html += "<p>You are logged in.</p>";
      html += "<ul>";
      html += "<li><a href='/usb_pd/'>USB PD Control</a></li>";
      html += "<li><a href='/system/'>System Status</a></li>";
      html += "<li><a href='/api/tokens'>Manage API Tokens</a></li>";
      html += "</ul>";
      html += "</body></html>";
      
      res.setContent(html, "text/html");
    }, {AuthType::SESSION});
  }
}

void loop() {
  webPlatform.handle();
  
  if (webPlatform.isConnected()) {
    usbPDController.handle();
  }
}
```

**Key Points:**
- The login page (`/login`) is automatically provided by the WebPlatform
- Session authentication requires username/password (default is admin/password)
- All protected routes redirect to the login page if not authenticated
- You can override routes from modules to add auth requirements
- You can access the authenticated username via `req.getAuthContext().username`

## 3. Application Developer: API Token Management for End Users

As an application developer, you may want to allow end users to create API tokens for programmatic access to your application's features.

### Example: Creating and Managing API Tokens

```cpp
#include <web_platform.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Setup navigation menu
  std::vector<NavigationItem> navItems = {
    NavigationItem("Home", "/"),
    NavigationItem("USB PD Control", "/usb_pd/"),
    NavigationItem("API Access", "/api-access/"),
    NavigationItem("Logout", "/logout")
  };
  IWebModule::setNavigationMenu(navItems);
  
  // Initialize web platform
  webPlatform.begin("APIEnabledDevice");
  
  if (webPlatform.isConnected()) {
    // Register modules
    webPlatform.registerModule("/usb_pd", &usbPDController);
    
    // Add API token management page
    webPlatform.registerRoute("/api-access/", [](WebRequest& req, WebResponse& res) {
      String html = R"(
        <!DOCTYPE html>
        <html>
        <head>
          <title>API Access Management</title>
          <link rel="stylesheet" href="/styles.css">
          <script>
            async function createToken() {
              const description = document.getElementById('token-description').value;
              const response = await fetch('/api/tokens', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({description})
              });
              const data = await response.json();
              document.getElementById('token-result').innerHTML = 
                `<div class="card warning">
                  <p><strong>Your new API token:</strong> ${data.token}</p>
                  <p class="error">Save this token now! It won't be shown again.</p>
                </div>`;
              loadTokens();
            }
            
            async function loadTokens() {
              const response = await fetch('/api/tokens');
              const tokens = await response.json();
              const list = document.getElementById('token-list');
              list.innerHTML = '';
              
              tokens.forEach(token => {
                const item = document.createElement('li');
                item.className = 'token-item';
                item.innerHTML = `
                  <span class="token-id">${token.token}</span>
                  <span class="token-desc">${token.description}</span>
                  <span class="token-date">Created: ${new Date(token.created).toLocaleString()}</span>
                `;
                list.appendChild(item);
              });
            }
            
            document.addEventListener('DOMContentLoaded', loadTokens);
          </script>
        </head>
        <body>
          <div class="container">
            <h1>API Access Management</h1>
            
            <div class="card">
              <h2>Create New API Token</h2>
              <div class="form-group">
                <label for="token-description">Token Description:</label>
                <input type="text" id="token-description" class="form-control" 
                       placeholder="Home Assistant Integration">
              </div>
              <button onclick="createToken()" class="btn btn-primary">Create Token</button>
              <div id="token-result"></div>
            </div>
            
            <div class="card">
              <h2>Your API Tokens</h2>
              <ul id="token-list" class="token-list"></ul>
            </div>
            
            <div class="card info">
              <h3>API Usage</h3>
              <p>Use your API token in one of these ways:</p>
              <ol>
                <li>HTTP header: <code>Authorization: Bearer YOUR_TOKEN</code></li>
                <li>URL parameter: <code>?access_token=YOUR_TOKEN</code></li>
              </ol>
              <p>Example API endpoints:</p>
              <ul>
                <li><code>/api/usb_pd/status</code> - Get current USB PD status</li>
                <li><code>/api/usb_pd/profiles</code> - List available profiles</li>
              </ul>
            </div>
          </div>
        </body>
        </html>
      )";
      
      res.setContent(html, "text/html");
    }, {AuthType::SESSION});
    
    // Add dual-auth routes for API access
    // These can be accessed via SESSION (web interface) OR TOKEN (API)
    webPlatform.registerRoute("/api/usb_pd/status", [](WebRequest& req, WebResponse& res) {
      // Get status and return as JSON
      String status = usbPDController.getStatusJSON();
      res.setContent(status, "application/json");
    }, {AuthType::SESSION, AuthType::TOKEN});
    
    webPlatform.registerRoute("/api/usb_pd/profiles", [](WebRequest& req, WebResponse& res) {
      // Get profiles and return as JSON
      String profiles = usbPDController.getProfilesJSON();
      res.setContent(profiles, "application/json");
    }, {AuthType::SESSION, AuthType::TOKEN});
    
    // Add a route that allows controlling the device via API
    webPlatform.registerRoute("/api/usb_pd/set-profile", [](WebRequest& req, WebResponse& res) {
      if (req.getMethod() != "POST") {
        res.setStatus(405);
        res.setContent("{\"error\":\"Method not allowed\"}", "application/json");
        return;
      }
      
      String profileId = req.getParam("profile");
      bool success = usbPDController.setActiveProfile(profileId);
      
      if (success) {
        res.setContent("{\"success\":true,\"profile\":\"" + profileId + "\"}", 
                      "application/json");
      } else {
        res.setStatus(400);
        res.setContent("{\"success\":false,\"error\":\"Invalid profile\"}", 
                      "application/json");
      }
    }, {AuthType::SESSION, AuthType::TOKEN});
  }
}

void loop() {
  webPlatform.handle();
  
  if (webPlatform.isConnected()) {
    usbPDController.handle();
  }
}
```

**Key Points:**
- The API token management page at `/api-access/` is protected by session auth
- Users create tokens through a simple web interface
- Tokens are displayed only once during creation for security
- API endpoints accept both SESSION and TOKEN auth using `{AuthType::SESSION, AuthType::TOKEN}`
- Tokens can be used in HTTP headers or URL parameters
- The `req.getAuthContext()` object contains info about how the user authenticated

## Testing the API with Tokens

Once a token is created, users can access the API programmatically:

### Using cURL
```bash
# Using Authorization header (preferred)
curl -H "Authorization: Bearer abcdef123456789" http://device.local/api/usb_pd/status

# Using URL parameter (alternative)
curl "http://device.local/api/usb_pd/status?access_token=abcdef123456789"

# POST request to set profile
curl -X POST -H "Authorization: Bearer abcdef123456789" \
     "http://device.local/api/usb_pd/set-profile?profile=profile1"
```

### Using JavaScript (e.g., Home Assistant integration)
```javascript
async function getDeviceStatus() {
  const response = await fetch('http://device.local/api/usb_pd/status', {
    headers: {
      'Authorization': 'Bearer abcdef123456789'
    }
  });
  return await response.json();
}

async function setDeviceProfile(profileId) {
  const response = await fetch(`http://device.local/api/usb_pd/set-profile?profile=${profileId}`, {
    method: 'POST',
    headers: {
      'Authorization': 'Bearer abcdef123456789'
    }
  });
  return await response.json();
}
```

## Advanced Example: Customizing Authentication

If you need to customize the authentication system, you can override the default routes:

```cpp
void setup() {
  // ... setup code ...
  
  if (webPlatform.isConnected()) {
    // Override the login page with a custom implementation
    webPlatform.overrideRoute("/login", [](WebRequest& req, WebResponse& res) {
      String redirectUrl = req.getParam("redirect");
      if (redirectUrl.isEmpty()) {
        redirectUrl = "/";
      }
      
      if (req.getMethod() == "POST") {
        String username = req.getParam("username");
        String password = req.getParam("password");
        
        // Custom authentication logic
        bool isValid = validateCredentials(username, password);
        
        if (isValid) {
          // Create session (handled by WebPlatform internally)
          // This function is part of the web_platform_auth.cpp implementation
          String sessionId = createSession(username);
          
          // Set session cookie
          res.setHeader("Set-Cookie", "session=" + sessionId + "; Path=/; Max-Age=86400; SameSite=Strict");
          
          // Redirect to requested page
          res.redirect(redirectUrl);
          return;
        } else {
          // Show custom error message
          String html = getCustomLoginPage(redirectUrl, "Invalid credentials");
          res.setContent(html, "text/html");
          res.setStatus(401);
          return;
        }
      } else {
        // Show login form
        String html = getCustomLoginPage(redirectUrl);
        res.setContent(html, "text/html");
      }
    }, {AuthType::NONE});
  }
}

// Custom validation function
bool validateCredentials(const String& username, const String& password) {
  // Example: Check against EEPROM or SPIFFS stored credentials
  if (username == getSavedUsername() && password == getSavedPassword()) {
    return true;
  }
  return false;
}

// Custom login page generator
String getCustomLoginPage(const String& redirectUrl, const String& errorMsg = "") {
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Login - My Device</title>
      <link rel="stylesheet" href="/styles.css">
      <style>
        .login-container {
          max-width: 400px;
          margin: 50px auto;
        }
        .logo {
          text-align: center;
          margin-bottom: 20px;
        }
      </style>
    </head>
    <body>
      <div class="container login-container">
        <div class="logo">
          <img src="/logo.png" alt="My Device" height="80">
        </div>
        <div class="card">
          <h1>Log In</h1>
  )";
  
  if (errorMsg.length() > 0) {
    html += "<div class=\"card error\">" + errorMsg + "</div>";
  }
  
  html += R"(
          <form method="post" action="/login?redirect=)" + redirectUrl + R"(">
            <div class="form-group">
              <label for="username">Username:</label>
              <input type="text" id="username" name="username" class="form-control" required>
            </div>
            <div class="form-group">
              <label for="password">Password:</label>
              <input type="password" id="password" name="password" class="form-control" required>
            </div>
            <div class="form-group">
              <label>
                <input type="checkbox" name="remember"> Remember me
              </label>
            </div>
            <button type="submit" class="btn btn-primary" style="width:100%">Login</button>
          </form>
        </div>
      </div>
    </body>
    </html>
  )";
  
  return html;
}
```

This customization allows you to:

1. Use your own credential storage and validation
2. Create a branded login experience
3. Add features like "Remember me"
4. Implement more complex authentication flows

## Best Practices

1. **Layer Your Security**:
   - Protect sensitive operations with SESSION auth
   - Use TOKEN auth for API access
   - Consider implementing IP-based restrictions for additional security

2. **Use Appropriate Auth Types**:
   - `SESSION` for browser-based access (redirects to login page)
   - `TOKEN` for programmatic API access (returns 401 status)
   - `{SESSION, TOKEN}` for routes accessible both ways

3. **Token Management**:
   - Show tokens only once during creation
   - Implement token revocation capabilities
   - Consider adding expiration to tokens for critical applications

4. **Route Organization**:
   - Use `/api/*` prefix for programmatic endpoints
   - Keep user interface routes separate from API routes
   - Document API endpoints for developers and end users

5. **Security Considerations**:
   - Enable HTTPS when possible for encrypted communication
   - Implement rate limiting for login attempts
   - Use secure password storage (beyond the scope of this example)
   - Consider implementing token scope limitations