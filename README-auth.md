# Authentication System for TickerTape

This document provides details on how to use the authentication system in the TickerTape platform.

## Architecture Overview

The authentication system is designed with a clear separation of responsibilities:

1. **WebPlatform Core**: Handles token generation, validation, and cookie management
2. **Module Templates**: Include placeholders for tokens and use the auth utilities
3. **Client-Side Library**: Provides standardized methods for authenticated API requests

This separation ensures modules remain independent from the WebPlatform internals.

## Authentication Types

The system supports multiple authentication methods:

- `AuthType::NONE` - No authentication required (public access)
- `AuthType::SESSION` - Cookie-based session authentication (for UI)
- `AuthType::TOKEN` - Bearer token authentication (for API)
- `AuthType::PAGE_TOKEN` - CSRF protection for forms
- `AuthType::LOCAL_ONLY` - Restrict to local network (implementation pending)

## Route Protection

Routes in the TickerTape platform can be protected using different authentication requirements:

```cpp
// Public route - no authentication required
webPlatform.registerRoute("/public", publicHandler, {AuthType::NONE});

// UI route - requires session authentication
webPlatform.registerRoute("/dashboard", dashboardHandler, {AuthType::SESSION});

// API route - requires token authentication
webPlatform.registerRoute("/api/data", apiHandler, {AuthType::TOKEN});

// Form submission - requires session and CSRF protection
webPlatform.registerRoute("/api/update-settings", updateHandler, 
                         {AuthType::SESSION, AuthType::PAGE_TOKEN}, 
                         WebModule::WM_POST);
```

## Authentication Best Practices

### For Web UI:

1. Public static assets (HTML, CSS, JS) should use `{AuthType::NONE}`
2. UI pages that require login should use `{AuthType::SESSION}`
3. Form submissions should use both `{AuthType::SESSION, AuthType::PAGE_TOKEN}`

### For API Routes:

1. All API endpoints should use `{AuthType::TOKEN}`
2. API routes should be prefixed with `/api/` by convention

## Client-Side Authentication

The platform includes a JavaScript utility (`auth_utils_js.h`) that simplifies API authentication in the browser:

```javascript
// Make an authenticated GET request
TickerTapeAuth.get('/api/data')
  .then(data => {
    console.log('Data received:', data);
  })
  .catch(error => {
    console.error('Error:', error);
  });

// Make an authenticated POST request
TickerTapeAuth.post('/api/update', { 
  value: 42 
})
  .then(response => {
    console.log('Update successful:', response);
  })
  .catch(error => {
    console.error('Error:', error);
  });
```

## Creating and Managing Tokens

### From the UI:

1. Navigate to the `/account` page when logged in
2. Use the token management interface to create new API tokens
3. Store the token securely - it cannot be viewed again after creation

### Using the JavaScript Library:

```javascript
// Create a new token
TickerTapeAuth.createToken('My Device')
  .then(token => {
    console.log('New token created:', token);
    // Token is automatically stored for future requests
  })
  .catch(error => {
    console.error('Failed to create token:', error);
  });

// Manually set a token
TickerTapeAuth.setToken('tok_abcdef123456');

// Show the token input dialog
TickerTapeAuth.showTokenDialog();
```

## Token Storage

Tokens are securely stored in the device's persistent storage (Preferences on ESP32, EEPROM on ESP8266) and remain valid until explicitly deleted. For security reasons:

- Tokens are never stored in plain text
- Tokens are prefixed with `tok_` to identify them
- Token validation is performed on every request

## Module Implementation

When implementing modules, follow these guidelines:

1. Use `{AuthType::NONE}` for static content (HTML, CSS, JavaScript)
2. Use `{AuthType::PAGE_TOKEN}` for API endpoints accessed from the browser
3. Use `{AuthType::TOKEN}` for API endpoints accessed by external systems
4. Use `{AuthType::SESSION}` for UI pages that require user login

### HTML Templates

Include a CSRF token placeholder in your HTML templates:

```html
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta name="csrf-token" content="{{csrfToken}}">
  <!-- Other head content -->
</head>
```

The WebPlatform will automatically replace `{{csrfToken}}` with a valid token.

### JavaScript Implementation

Include the auth utilities JavaScript in your module's HTML:

```html
<script src="/assets/auth_utils.js"></script>
<script>
  // Use TickerTapeAuth for API requests
  function loadData() {
    TickerTapeAuth.get('/api/mymodule/data')
      .then(data => {
        // Process data
      })
      .catch(error => {
        console.error('Failed to load data:', error);
      });
  }
</script>
```

The `TickerTapeAuth` library will automatically:
1. Extract the CSRF token from the meta tag
2. Include it in all API requests
3. Handle authentication errors appropriately

## Testing API Endpoints

For development purposes:

1. Use the development token button that appears in the corner on local networks
2. Enter a valid API token in the dialog
3. All API requests will include this token automatically

## Security Considerations

1. API tokens should be treated like passwords and stored securely
2. For production use, consider implementing token expiration
3. Use HTTPS when possible to protect token transmission
4. API tokens should have granular permissions (feature pending)