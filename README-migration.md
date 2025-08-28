# WebPlatform Route Handler Migration

This document outlines the current status of the route handler migration plan for the WebPlatform library.

## Migration Overview

The WebPlatform is transitioning from a dual system (static assets + route handlers) to a unified route-based architecture. This enables route overriding, disabling functionality, and provides a foundation for authentication and maker API features.

## Implementation Status

### Phase 1: Unified Route Handler System ✅
- [x] Created `WebRequest` and `WebResponse` classes for unified handling
- [x] Replaced static asset system with route handlers
- [x] Updated `WebPlatform` to use route handlers for all content
- [x] Updated `IWebModule` interface for route registration
- [x] Implemented route registry with global route management
- [x] Added route precedence system
- [x] Added support for HTTP and HTTPS unified handlers

### Phase 2: Authentication Framework ✅
- [x] Defined `AuthType` enum with initial types (NONE, SESSION, TOKEN)
- [x] Implemented `AuthRequirements` container for route protection
- [x] Added `AuthContext` for tracking authentication state
- [x] Implemented session-based authentication system
- [x] Implemented token-based authentication system
- [x] Updated route registration to accept auth requirements
- [x] Added authentication middleware to request handling
- [x] Created login/logout routes
- [x] Added token management API endpoint

### Phase 3: Route Override & Disable System (Planned)
- [ ] Extend overrideRoute() method with full auth requirements
- [ ] Extend disableRoute() method with better validation
- [ ] Implement module-specific override tracking
- [ ] Add validation to prevent begin() before route registration complete
- [ ] Add route conflict detection and resolution

### Phase 4: Development Tools (Planned)
- [ ] Enhance printRoutes() debug method
- [ ] Add module/override/disabled status to route listings
- [ ] Add route validation warnings for common issues
- [ ] Document route discovery workflow
- [ ] Add route search capabilities

## Authentication Framework

The authentication framework implemented in Phase 2 provides a flexible system for protecting routes with different authentication mechanisms.

### Auth Types

The system supports these authentication types:

- `AuthType::NONE` - No authentication required
- `AuthType::SESSION` - Cookie-based web session
- `AuthType::TOKEN` - API token for programmatic access
- `AuthType::LOCAL_ONLY` - Network-based restrictions (future implementation)

### Route Registration with Auth

Routes can be registered with specific authentication requirements:

```cpp
// No authentication required
webPlatform.registerRoute("/public", publicHandler, {AuthType::NONE});

// Session authentication only
webPlatform.registerRoute("/admin", adminHandler, {AuthType::SESSION});

// API token authentication only
webPlatform.registerRoute("/api/status", apiHandler, {AuthType::TOKEN});

// Accept either session OR token auth
webPlatform.registerRoute("/api/user", userHandler, {AuthType::SESSION, AuthType::TOKEN});
```

### Authentication Flow

1. When a request is received, the authentication middleware checks if the route requires authentication
2. If auth is required, it attempts to authenticate using each allowed auth type
3. If authentication succeeds, the request is processed with authentication context available
4. If authentication fails:
   - Session requests redirect to the login page
   - API requests receive a 401 Unauthorized response

### Session Management

The system implements a simple in-memory session management system:

- Login at `/login` with username/password
- Sessions stored with expiration based on activity
- Session ID stored in secure cookies
- Logout at `/logout` to invalidate sessions

### API Token Management

API tokens provide programmatic access:

- Create tokens via `/api/tokens` (requires session auth)
- Tokens passed via Bearer Authentication header
- Token validation for every API request

## Usage Examples

### Registering a Module with Auth Requirements

Modules can now specify default auth requirements for their routes:

```cpp
class SecureModule : public IWebModule {
public:
  std::vector<WebRoute> getHttpRoutes() override {
    return {
      // Public route - no auth required
      WebRoute("/", WebModule::WM_GET, 
              [this](WebRequest& req, WebResponse& res) {
                res.setContent(getPublicPage());
              }, "text/html"),
              
      // Admin route - session auth required
      WebRoute("/admin", WebModule::WM_GET, 
              [this](WebRequest& req, WebResponse& res) {
                res.setContent(getAdminPage());
              }, "text/html"),
              
      // API route - token auth required
      WebRoute("/api", WebModule::WM_GET, 
              [this](WebRequest& req, WebResponse& res) {
                res.setContent(getApiResponse(), "application/json");
              }, "application/json")
    };
  }
  
  // ... other IWebModule methods
};
```

When registering the module, WebPlatform will respect these auth requirements, but the main application can also override them:

```cpp
// Register module with default auth requirements
webPlatform.registerModule("/secure", &secureModule);

// Override a specific route to use different auth
webPlatform.overrideRoute("/secure/admin", customHandler, {AuthType::SESSION, AuthType::TOKEN});

// Disable a module route entirely
webPlatform.disableRoute("/secure/api");
```

### Accessing Auth Context in Handlers

Route handlers can access authentication information:

```cpp
auto profileHandler = [](WebRequest& req, WebResponse& res) {
  const AuthContext& auth = req.getAuthContext();
  
  // Get authenticated username
  String username = auth.username;
  
  // Check auth type
  if (auth.authenticatedVia == AuthType::SESSION) {
    // User is using web interface
    res.setContent(generateHtmlProfile(username));
  } else if (auth.authenticatedVia == AuthType::TOKEN) {
    // API access
    res.setContent("{\"username\":\"" + username + "\"}", "application/json");
  }
};

webPlatform.registerRoute("/profile", profileHandler, {AuthType::SESSION, AuthType::TOKEN});
```

## Breaking Changes

The migration introduces these breaking changes:

1. **IWebModule Interface**: 
   - Legacy `getHttpRoutes()`/`getHttpsRoutes()` methods still work but should be updated
   - New unified handler signature: `void(WebRequest&, WebResponse&)`

2. **Static Asset Registration**:
   - `addStaticAsset()` methods are now implemented as route handlers
   - Assets should be registered as routes with appropriate MIME types

3. **Route Registration**:
   - New signature includes auth requirements
   - Override mechanism has precedence over default registrations

## Backward Compatibility

For backward compatibility:

1. Legacy handler functions are automatically wrapped in unified handlers
2. Default auth type is `NONE` for converted legacy routes
3. Existing modules work without changes

## Future Enhancements

Planned enhancements beyond Phase 4:

### Parameterized Routes
```cpp
// Future enhancement - support parameterized routes
// registerRoute("/api/usb_pd/profile/{id}", handler, {AuthType::SESSION});
// WebRequest could add: String getRouteParam(String name);
```

### Additional Auth Types
- `AuthType::ADMIN` - Administrative access
- `AuthType::MAKER_API` - External API access with specialized tokens

### Dynamic Route Registration
- Runtime route registration (if needed)
- Conditional route enabling based on configuration