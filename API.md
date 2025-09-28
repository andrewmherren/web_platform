# WebPlatform API Reference

This document provides a comprehensive API reference for the WebPlatform library.

## Table of Contents
- [WebPlatform Core API](#webplatform-core-api)
- [IWebModule Interface](#iwebmodule-interface)
- [WebRoute Structure](#webroute-structure)
- [WebRequest Class](#webrequest-class)
- [WebResponse Class](#webresponse-class)
- [Authentication API](#authentication-api)
- [Storage API](#storage-api)
- [Navigation System](#navigation-system)
- [Error Handling](#error-handling)

## WebPlatform Core API

### Initialization

```cpp
// Basic initialization
void begin(const char* deviceName = "Device", bool forceHttpsOnly = false);
```

Parameters:
- `deviceName`: Name of the device, used for AP SSID and mDNS hostname
- `forceHttpsOnly`: If true, only HTTPS will be used (if available); HTTP requests are rejected

### Module Management

```cpp
// Register a module with base path
bool registerModule(const char* basePath, IWebModule* module);
```

Parameters:
- `basePath`: Base URL path for this module (e.g., "/sensor")
- `module`: Pointer to an IWebModule instance

Returns:
- `true` if module was registered successfully
- `false` if registration failed (e.g., already in config portal mode)

### Route Management

```cpp
// Register, replace, or disable a route
void registerWebRoute(const String& path, WebModule::UnifiedRouteHandler handler,
                  const AuthRequirements& auth = {AuthType::NONE},
                  WebModule::Method method = WebModule::WM_GET);
```

This unified method can be used to:
- **Register a new route**: When the path doesn't exist yet
- **Replace an existing route**: When registering a path that already exists
- **Disable a route**: By passing `nullptr` as the handler

Parameters:
- `path`: URL path for the route
- `handler`: Function to handle the request
- `auth`: Authentication requirements (defaults to public access)
- `method`: HTTP method (GET, POST, etc.)

```cpp
// Register, replace, or disable an API route
void registerApiRoute(const String &path,
                     WebModule::UnifiedRouteHandler handler,
                     const AuthRequirements &auth, WebModule::Method method,
                     const OpenAPIDocumentation &docs = OpenAPIDocumentation());
```
This unified method can be used to:
- **Register a new API route**: When the path doesn't exist yet
- **Replace an existing API route**: When registering a path that already exists
- **Disable a API route**: By passing `nullptr` as the handler

Parameters:
- `path`: URL path for the route
- `handler`: Function to handle the request
- `auth`: Authentication requirements (defaults to public access)
- `method`: HTTP method (GET, POST, etc.)
- `docs`: OpenAPI documentation details (optional, use `API_DOC_BLOCK()` macro for memory efficiency)

**Note**: OpenAPI documentation is optional and controlled by the `WEB_PLATFORM_OPENAPI` build flag. When disabled, documentation has zero memory impact.

### Request Handling

```cpp
// Process HTTP requests and WiFi operations (call in loop)
void handle();
```

### State Queries

```cpp
// Check if connected to WiFi
bool isConnected() const;

// Get current connection state
WiFiConnectionState getConnectionState() const;

// Get current platform mode
PlatformMode getCurrentMode() const;

// Check if HTTPS is enabled
bool isHttpsEnabled() const;

// Get base URL for the device
String getBaseUrl() const;

// Get server port
// int getPort() const;
```

### WiFi Management

```cpp
// Reset stored WiFi credentials
void resetWiFiCredentials();

// Get access point name
const char* getAPName() const;

// Get mDNS hostname
String getHostname() const;
```

### Debug and Monitoring

```cpp
// Get number of registered routes
size_t getRouteCount() const;

// Print all registered routes (for debugging)
void printUnifiedRoutes(const String* moduleBasePath = nullptr,
                       IWebModule* module = nullptr) const;
```

## IWebModule Interface

### Required Methods

```cpp
// Get HTTP routes for this module
virtual std::vector<RouteVariant> getHttpRoutes() = 0;

// Get HTTPS routes for this module
virtual std::vector<RouteVariant> getHttpsRoutes() = 0;

// Get module name
virtual String getModuleName() const = 0;
```

### Optional Methods

```cpp
// Get module version (defaults to "1.0.0")
virtual String getModuleVersion() const;

// Get module description (defaults to "Web-enabled module")
virtual String getModuleDescription() const;

// Convenience method for modules with identical HTTP/HTTPS routes
virtual std::vector<RouteVariant> getWebRoutes();
```

### Static Utility Methods

```cpp
// Set navigation menu items
static void setNavigationMenu(const std::vector<NavigationItem>& items);

// Get current navigation menu items
static std::vector<NavigationItem> getNavigationMenu();

// Generate navigation HTML
static String generateNavigationHtml(bool isAuthenticated = false);

// Set custom error page for specific status code
static void setErrorPage(int statusCode, const String& html);

// Get error page for specific status code
static String getErrorPage(int statusCode);

// Generate default error page with status code and optional message
static String generateDefaultErrorPage(int statusCode, const String& message = "");

// Add URL redirect
static void addRedirect(const String& fromPath, const String& toPath);

// Get redirect target for path (empty if no redirect)
static String getRedirectTarget(const String& requestPath);
```

## WebRoute Structure

```cpp
struct WebRoute {
    String path;                     // Route path
    WebModule::Method method;        // HTTP method  
    WebModule::RouteHandler handler; // Legacy function pointer (deprecated)
    WebModule::UnifiedRouteHandler unifiedHandler; // New unified handler
    String contentType;              // Response content type
    String description;              // Optional description
    AuthRequirements authRequirements; // Authentication requirements
};
```

### Constructors

```cpp
// Basic constructor
WebRoute(const String& p, WebModule::Method m, WebModule::UnifiedRouteHandler h);

// With content type
WebRoute(const String& p, WebModule::Method m, WebModule::UnifiedRouteHandler h, const String& ct);

// With content type and description
WebRoute(const String& p, WebModule::Method m, WebModule::UnifiedRouteHandler h, const String& ct, const String& desc);

// With auth requirements
WebRoute(const String& p, WebModule::Method m, WebModule::UnifiedRouteHandler h, const AuthRequirements& auth);

// With auth requirements and content type
WebRoute(const String& p, WebModule::Method m, WebModule::UnifiedRouteHandler h, const AuthRequirements& auth, const String& ct);

// With auth requirements, content type, and description
WebRoute(const String& p, WebModule::Method m, WebModule::UnifiedRouteHandler h, const AuthRequirements& auth, const String& ct, const String& desc);
```

## WebRequest Class

```cpp
class WebRequest {
public:
    // Get request method
    WebModule::Method getMethod() const;
    
    // Get request path
    String getPath() const;
    
    // Get query parameter value
    String getParam(const String& name, const String& defaultValue = "") const;
    
    // Get all query parameters
    std::map<String, String> getParams() const;
    
    // Get request body
    String getBody() const;
    
    // Get request header value
    String getHeader(const String& name) const;
    
    // Get all headers
    std::map<String, String> getHeaders() const;
    
    // Get client IP address
    IPAddress getClientIP() const;
    
    // Get authentication context
    const AuthContext& getAuthContext() const;
    
    // Check if request is from local network
    bool isLocalRequest() const;
};
```

## WebResponse Class

```cpp
class WebResponse {
public:
    // Set response content and content type
    void setContent(const String& content, const String& contentType);
    
    // Set PROGMEM content for memory-efficient streaming
    void setProgmemContent(const char* progmemData, const String& mimeType);
    
    // Set response status code
    void setStatus(int code);
    
    // Set response header
    void setHeader(const String& name, const String& value);
    
    // Redirect to another URL
    void redirect(const String& url);
    
    // Send response as JSON
    void json(const String& jsonContent);
    
    // Get response content
    String getContent() const;
    
    // Get status code
    // int getStatus() const;
    
    // Get headers
    std::map<String, String> getHeaders() const;
};
```

### Asset Serving Methods

- **`setContent(content, mimeType)`**: Use for dynamic content, JSON responses, and computed strings
- **`setProgmemContent(progmemData, mimeType)`**: Use for all embedded assets stored as `const char PROGMEM` arrays

The `setProgmemContent` method provides memory-efficient streaming that prevents fragmentation with large assets while maintaining optimal performance for smaller ones.

## Authentication API

### Authentication Types

```cpp
enum class AuthType {
    NONE,         // Public access (no authentication)
    SESSION,      // Web-based login required (cookies)
    TOKEN,        // API token required (Bearer or URL param)
    PAGE_TOKEN,   // CSRF protection for forms
    LOCAL_ONLY    // Restrict to local network access
};
```

### Authentication Requirements

```cpp
struct AuthRequirements {
    std::vector<AuthType> requiredAuthTypes;  // Authentication types required
    
    // Default constructor (no authentication)
    AuthRequirements() : requiredAuthTypes({AuthType::NONE}) {}
    
    // Constructor with a single auth type
    AuthRequirements(AuthType type) : requiredAuthTypes({type}) {}
    
    // Constructor with multiple auth types
    AuthRequirements(std::initializer_list<AuthType> types) : requiredAuthTypes(types) {}
    
    // Check if any authentication is required
    bool requiresAuth() const;
    
    // Check if a specific auth type is required
    bool requires(AuthType type) const;
};
```

### Authentication Context

```cpp
struct AuthContext {
    bool isAuthenticated;        // Whether request is authenticated
    AuthType authenticatedVia;   // Which auth method was used
    String username;             // Username (for SESSION auth)
    String userId;               // User ID (for SESSION auth)
    String token;                // Token ID (for TOKEN auth)
    bool isAdmin;                // Whether user has admin privileges
};
```

### Auth Storage API

```cpp
// User management
String AuthStorage::createUser(const String& username, const String& password);
AuthUser AuthStorage::findUserById(const String& userId);
AuthUser AuthStorage::findUserByUsername(const String& username);
bool AuthStorage::updateUser(const String& userId, const String& username, const String& password);
bool AuthStorage::deleteUser(const String& userId);

// Session management
String AuthStorage::createSession(const String& userId);
AuthSession AuthStorage::findSessionById(const String& sessionId);
bool AuthStorage::deleteSession(const String& sessionId);

// Token management
String AuthStorage::createApiToken(const String& userId, const String& name);
std::vector<AuthApiToken> AuthStorage::getUserApiTokens(const String& userId);
bool AuthStorage::deleteApiToken(const String& tokenId);
```

## Storage API

### Storage Manager

```cpp
// Get query builder for a collection (uses default driver)
static QueryBuilder StorageManager::query(const String& collection);

// Get specific storage driver
static IDatabaseDriver* StorageManager::driver(const String& driverName = "");

// Configure a new storage driver
static void StorageManager::configureDriver(const String& name, std::unique_ptr<IDatabaseDriver> driver);

// Set default driver
static bool StorageManager::setDefaultDriver(const String& name);

// Get list of available driver names
static std::vector<String> StorageManager::getDriverNames();

// Remove a driver (cannot remove "json" driver)
static bool StorageManager::removeDriver(const String& name);

// Direct storage operations (use default driver)
static bool StorageManager::store(const String& collection, const String& key, const String& data);
static String StorageManager::get(const String& collection, const String& key);
static bool StorageManager::remove(const String& collection, const String& key);
```

### Database Driver Interface

```cpp
class IDatabaseDriver {
public:
    // Core storage operations
    virtual bool store(const String& collection, const String& key, const String& data) = 0;
    virtual String retrieve(const String& collection, const String& key) = 0;
    virtual bool remove(const String& collection, const String& key) = 0;
    virtual std::vector<String> listKeys(const String& collection) = 0;
    virtual bool exists(const String& collection, const String& key) = 0;
    
    // Driver identification
    virtual String getDriverName() const = 0;
};
```

### Query Builder

```cpp
class QueryBuilder {
public:
    // Constructor with specific driver
    QueryBuilder(IDatabaseDriver* driver, const String& collection);
    
    // Filter by field value
    QueryBuilder& where(const String& field, const String& value);
    
    // Get single result
    String get();
    
    // Get all matching results
    std::vector<String> getAll();
    
    // Store data with key
    bool store(const String& key, const String& data);
    
    // Delete data by key
    bool remove(const String& key);
    
    // Delete all matching results
    bool removeAll();
    
    // Check if key exists
    bool exists(const String& key);
    
    // Get all keys in collection
    std::vector<String> keys();
};
```

### JSON Database Driver (Default)

```cpp
class JsonDatabaseDriver : public IDatabaseDriver {
public:
    JsonDatabaseDriver();
    
    // Cache management
    void clearCache();
    void clearCollection(const String& collection);
    size_t getCacheSize() const;
    void evictCollection(const String& collection);
    
    // IDatabaseDriver implementation
    bool store(const String& collection, const String& key, const String& data) override;
    String retrieve(const String& collection, const String& key) override;
    bool remove(const String& collection, const String& key) override;
    std::vector<String> listKeys(const String& collection) override;
    bool exists(const String& collection, const String& key) override;
    String getDriverName() const override;
};
```

### LittleFS Database Driver

```cpp
class LittleFSDatabaseDriver : public IDatabaseDriver {
public:
    // Constructor with custom base path
    LittleFSDatabaseDriver(const String& baseStoragePath = "/storage");
    
    // LittleFS-specific features
    String getFilesystemStats();  // JSON with filesystem usage info
    size_t getKeySize(const String& collection, const String& key);
    size_t getCollectionSize(const String& collection);
    bool removeCollection(const String& collection);
    std::vector<String> listCollections();
    bool formatFilesystem();  // WARNING: Destroys all data
    
    // Cache management
    void clearCache();
    
    // IDatabaseDriver implementation
    bool store(const String& collection, const String& key, const String& data) override;
    String retrieve(const String& collection, const String& key) override;
    bool remove(const String& collection, const String& key) override;
    std::vector<String> listKeys(const String& collection) override;
    bool exists(const String& collection, const String& key) override;
    String getDriverName() const override;
};
```

### Setting Up Multiple Drivers

```cpp
void setupStorageDrivers() {
    // JSON driver is configured automatically
    
    // Configure LittleFS driver
    auto littleFSDriver = std::unique_ptr<IDatabaseDriver>(
        new LittleFSDatabaseDriver("/app_data")
    );
    StorageManager::configureDriver("littlefs", std::move(littleFSDriver));
    
    // Use JSON as default for auth data (fast access)
    StorageManager::setDefaultDriver("json");
    
    // Direct driver usage
    StorageManager::driver("json")->store("sessions", "current", sessionData);
    StorageManager::driver("littlefs")->store("documents", "spec", largeDocument);
}
```

## Navigation System

### Navigation Item

```cpp
struct NavigationItem {
    String name;                 // Display name
    String url;                  // URL to link to
    String target;               // Optional target attribute
    NavAuthVisibility visibility; // When this item should be visible
};
```

### Navigation Visibility

```cpp
enum class NavAuthVisibility {
    ALWAYS,         // Always visible regardless of auth state
    AUTHENTICATED,  // Only visible when user has valid session
    UNAUTHENTICATED // Only visible when user is not authenticated
};
```

### Helper Functions

```cpp
// Make navigation item only visible to authenticated users
NavigationItem Authenticated(const NavigationItem& item);

// Make navigation item only visible to unauthenticated users
NavigationItem Unauthenticated(const NavigationItem& item);
```

### Storage Usage Examples

```cpp
// Example: Optimal storage strategy
void demonstrateStorageStrategy() {
    // Small, frequently accessed data -> JSON driver
    StorageManager::driver("json")->store("config", "app_name", "MyDevice");
    StorageManager::driver("json")->store("config", "current_user", "user123");
    
    // Large data that changes less frequently -> LittleFS driver
    String largeDocument = generateOpenAPISpec();  // Could be 20KB+
    StorageManager::driver("littlefs")->store("api_docs", "openapi_spec", largeDocument);
    
    // Query builder works with any driver
    String username = StorageManager::query("users")->where("id", "user123")->get();
    
    // LittleFS specific operations
    LittleFSDatabaseDriver* fs = static_cast<LittleFSDatabaseDriver*>(
        StorageManager::driver("littlefs")
    );
    if (fs) {
        String stats = fs->getFilesystemStats();
        size_t docSize = fs->getKeySize("api_docs", "openapi_spec");
        Serial.printf("Document size: %u bytes\n", docSize);
    }
}
```

## Error Handling

### Setting Custom Error Pages

```cpp
// Set custom error page for specific status code
IWebModule::setErrorPage(404, "<html><body><h1>Not Found</h1></body></html>");

// Generate default error page with custom message
String errorPage = IWebModule::generateDefaultErrorPage(500, "Database connection failed");
```

### Responding with Errors

```cpp
// In route handler
void errorHandler(WebRequest& req, WebResponse& res) {
    // Return 404 Not Found
    res.setStatus(404);
    res.setContent(IWebModule::getErrorPage(404), "text/html");
    
    // Alternative: JSON error
    res.setStatus(400);
    res.setContent("{\"error\":\"Bad request\",\"message\":\"Missing required parameter\"}", 
                   "application/json");
}

## OpenAPI Documentation (Optional)

**Dual OpenAPI System**: WebPlatform now supports two types of OpenAPI specifications:

1. **Full OpenAPI Specification** - Complete API documentation for all routes
2. **Maker API Specification** - Filtered specification containing only routes tagged for public/maker consumption

**Build Flag Control**: Both systems are controlled by separate build flags and are **disabled by default** to conserve memory.

```ini
# Enable full OpenAPI documentation
build_flags = -DWEB_PLATFORM_OPENAPI=1

# Enable Maker API documentation (filtered subset)
build_flags = -DWEB_PLATFORM_MAKERAPI=1

# Enable both (recommended for development)
build_flags = 
  -DWEB_PLATFORM_OPENAPI=1
  -DWEB_PLATFORM_MAKERAPI=1

# Disable both for production (default)
build_flags = 
  -DWEB_PLATFORM_OPENAPI=0
  -DWEB_PLATFORM_MAKERAPI=0
```

### OpenAPIDocumentation Class

```cpp
// Template-based documentation that optimizes away when disabled
using OpenAPIDocumentation = OpenAPIDoc<OPENAPI_ENABLED>;

class OpenAPIDoc<true> {  // When enabled
public:
    // Constructors
    OpenAPIDocumentation(
        const String& summary,
        const String& description = "",
        const String& operationId = "",
        const std::vector<String>& tags = {}
    );
    
    // Properties
    String summary;                // Brief summary of the endpoint
    String description;            // Detailed description
    String operationId;            // Unique operation identifier
    std::vector<String> tags;      // Tags for grouping endpoints
    
    // Examples and schema
    String requestExample;         // Example request body
    String responseExample;        // Example response body
    String requestSchema;          // JSON Schema for request
    String responseSchema;         // JSON Schema for response
    String parametersJson;         // JSON array of parameter objects
};

// When disabled, OpenAPIDoc<false> accepts any constructor args and does nothing
```

### Memory-Efficient Documentation Macros

```cpp
// Simple documentation
#define API_DOC(...) OpenAPIDocumentation(__VA_ARGS__)

// Complex documentation block that disappears when OpenAPI disabled
#define API_DOC_BLOCK(code) (code)  // When enabled
#define API_DOC_BLOCK(code) OpenAPIDocumentation()  // When disabled

// Usage examples
webPlatform.registerApiRoute("/status", handler, {AuthType::TOKEN}, WebModule::WM_GET,
    API_DOC("Get status", "Returns system status"));

webPlatform.registerApiRoute("/data", handler, {AuthType::TOKEN}, WebModule::WM_GET,
    API_DOC_BLOCK(MyApiDocs::createGetData()));
```

### OpenAPIFactory (Available When Enabled)

```cpp
#if OPENAPI_ENABLED
class OpenAPIFactory {
public:
    // Create OpenAPI documentation object
    static OpenAPIDocumentation create(
        const String& summary,
        const String& description = "",
        const String& operationId = "",
        const std::vector<String>& tags = {}
    );
    
    // Create standard response schemas
    static String createSuccessResponse(const String& description = "Operation successful");
    static String createErrorResponse(const String& description = "Error details");
    static String createListResponse(const String& itemDescription);
    
    // Create standard request schemas
    static String createJsonRequest(const String& description, const String& properties);
    static String createStringRequest(const String& description, int minLength = 1);
    
    // Create parameter definitions
    static String createIdParameter(const String& name, const String& description);
    
    // Utility methods
    static String generateOperationId(const String& method, const String& resource);
    static String formatTag(const String& moduleName);
};
#endif
```

### Documentation Factory Pattern

The recommended pattern for organizing API documentation:

```cpp
// Wrap documentation classes to disappear when OpenAPI disabled
#if OPENAPI_ENABLED
class ModuleNameDocs {
public:
    // Define module-specific tags
    static const std::vector<String> MODULE_TAGS;
    
    // Factory methods for each endpoint
    static OpenAPIDocumentation createGetResource();
    static OpenAPIDocumentation createUpdateResource();
    static OpenAPIDocumentation createDeleteResource();
};

// Implementation
const std::vector<String> ModuleNameDocs::MODULE_TAGS = {"Module Category"};

OpenAPIDocumentation ModuleNameDocs::createGetResource() {
    OpenAPIDocumentation doc = OpenAPIFactory::create(
        "Get resource details",
        "Returns detailed information about the resource",
        "getResource",
        MODULE_TAGS
    );
    
    // Define examples and schemas...
    
    return doc;
}
#endif // OPENAPI_ENABLED

// Usage in routes (works whether OpenAPI is enabled or disabled)
webPlatform.registerApiRoute("/resource", handler, {AuthType::TOKEN}, WebModule::WM_GET,
    API_DOC_BLOCK(ModuleNameDocs::createGetResource()));
```

### Accessing the OpenAPI Specifications

When enabled, two OpenAPI specifications are available:

**Full OpenAPI Specification** (when `OPENAPI_ENABLED`):
```
/openapi.json
```

**Maker API Specification** (when `MAKERAPI_ENABLED`):
```
/maker/openapi.json
```

The Maker API contains only routes that are:
- Tagged with configured "maker" tags (default: "maker")
- Intended for public/external consumption
- Suitable for maker projects and integrations

### Third-Party Tool Integration

Both OpenAPI specifications can be used with external tools:

**Full API Documentation**:
- **Swagger UI**: Import `/openapi.json` for complete API documentation
- **Internal Development**: Full access to all system endpoints
- **Administrative Tools**: Complete API coverage for system management

**Maker API Documentation**:
- **Public Documentation**: Import `/maker/openapi.json` for user-facing docs
- **Client Libraries**: Generate SDKs for only the public/maker endpoints
- **Postman Collections**: Create focused collections for public APIs
- **Integration Guides**: Simplified API reference for external developers

### Development Workflow

1. **Development Phase**: Enable both OpenAPI and Maker API for full exploration
2. **Documentation Phase**: Create comprehensive API documentation and tag routes for maker inclusion
3. **Public API Phase**: Use Maker API spec for external documentation and client generation
4. **Production Phase**: Choose appropriate flags based on deployment needs:
   - Internal systems: Both disabled for maximum memory efficiency
   - Public-facing devices: Keep Maker API enabled for user documentation
   - Development environments: Keep both enabled

### Maker API Route Tagging

To include routes in the Maker API specification, tag them appropriately:

```cpp
// Route automatically included in Maker API (has "maker" tag)
webPlatform.registerApiRoute("/device/status", handler, {AuthType::TOKEN}, WebModule::WM_GET,
    API_DOC("Get device status", "Returns current device status", "getStatus", {"maker"}));

// Route included with custom maker tags
webPlatform.registerApiRoute("/sensor/data", handler, {AuthType::TOKEN}, WebModule::WM_GET,
    API_DOC("Get sensor data", "Returns sensor readings", "getSensorData", {"maker", "sensor"}));

// Route NOT included in Maker API (no maker tags)
webPlatform.registerApiRoute("/admin/config", handler, {AuthType::SESSION}, WebModule::WM_GET,
    API_DOC("Get admin config", "Internal configuration", "getConfig", {"admin"}));
```

### Custom Maker API Tag Configuration

When registering the Maker API module, you can specify which tags should be included:

```cpp
// Default: only "maker" tag included
webPlatform.registerModule("/api-explorer", &makerAPI);

// Custom: include routes with "maker", "public", or "external" tags
DynamicJsonDocument tagsConfig(256);
JsonArray tagsArray = tagsConfig.createNestedArray("tags");
tagsArray.add("maker");
tagsArray.add("public");
tagsArray.add("external");
webPlatform.registerModule("/api-explorer", &makerAPI, tagsConfig);
```
