#ifndef MOCK_WEB_PLATFORM_H
#define MOCK_WEB_PLATFORM_H

// Use ArduinoFake for Arduino compatibility
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Forward declarations to avoid std::variant issues
struct WebRoute;
struct ApiRoute;

// Use simple typedef instead of std::variant for better compatibility
struct RouteVariant {
  enum Type { WEB_ROUTE, API_ROUTE } type;
  void *route;

  RouteVariant(const WebRoute &wr);
  RouteVariant(const ApiRoute &ar);
  ~RouteVariant();

  // Copy constructor and assignment for STL containers
  RouteVariant(const RouteVariant &other);
  RouteVariant &operator=(const RouteVariant &other);
};

// Mock AuthContext structure
struct AuthContext {
  bool isAuthenticated = false;
  String username = "";
  String userId = "";

  AuthContext() = default;
  AuthContext(bool auth, const char *user)
      : isAuthenticated(auth), username(user) {}
  AuthContext(bool auth, const String &user)
      : isAuthenticated(auth), username(user) {}
};

// Mock AuthType enum
enum class AuthType { NONE, SESSION, TOKEN, PAGE_TOKEN, LOCAL_ONLY };

// Mock AuthRequirements type
typedef std::vector<AuthType> AuthRequirements;

// Mock WebModule methods enum
namespace WebModule {
enum Method {
  WM_GET = 0,
  WM_POST = 1,
  WM_PUT = 2,
  WM_PATCH = 3,
  WM_DELETE = 4
};
}

// Mock WebRequest base class
class WebRequest {
public:
  virtual ~WebRequest() = default;
  virtual String getParam(const String &name) = 0;
  virtual String getBody() = 0;
  virtual const AuthContext &getAuthContext() const = 0;
};

// Mock WebResponse base class
class WebResponse {
public:
  virtual ~WebResponse() = default;
  virtual void setContent(const String &content, const String &contentType) = 0;
  virtual void setProgmemContent(const char *content,
                                 const String &contentType) = 0;
  virtual void setStatus(int statusCode) = 0;
  virtual void setHeader(const String &name, const String &value) = 0;
};

// Mock OpenAPIDocumentation structure
struct OpenAPIDocumentation {
  String summary;
  String description;
  String operationId;
  std::vector<String> tags;
  String responseExample;
  String responseSchema;

  OpenAPIDocumentation() = default;
  OpenAPIDocumentation(const String &sum, const String &desc)
      : summary(sum), description(desc) {}
  OpenAPIDocumentation(const char *sum, const char *desc)
      : summary(sum), description(desc) {}

  OpenAPIDocumentation &withResponseExample(const String &example) {
    responseExample = example;
    return *this;
  }

  OpenAPIDocumentation &withResponseSchema(const String &schema) {
    responseSchema = schema;
    return *this;
  }
};

#include "string_compat.h"

// Enhanced JsonResponseBuilder with native testing compatibility
class JsonResponseBuilder {
public:
  template <size_t Size>
  static void createResponse(WebResponse &res,
                             std::function<void(JsonObject &)> builder) {
    // Create JSON document for testing
    StaticJsonDocument<Size> doc;
    JsonObject root = doc.template to<JsonObject>();

    // Call the builder function to populate the JSON
    builder(root);

    // Use std::string for native compatibility with ArduinoJson
    std::string jsonString = StringCompat::serializeJsonToStdString(doc);

    // Convert to Arduino String for WebResponse
    String arduinoString = toArduinoString(jsonString);
    res.setContent(arduinoString, "application/json");
  }
};

// Enhanced OpenAPIFactory
class OpenAPIFactory {
public:
  static OpenAPIDocumentation create(const String &summary,
                                     const String &description,
                                     const String &operationId,
                                     const std::vector<String> &tags) {
    OpenAPIDocumentation doc;
    doc.summary = summary;
    doc.description = description;
    doc.operationId = operationId;
    doc.tags = tags;
    return doc;
  }

  static String createSuccessResponse(const String &description) {
    // Fixed string concatenation issue
    String result = "{\"type\":\"object\",\"description\":\"";
    result += description;
    result += "\"}";
    return result;
  }
};

// Mock route structures
typedef std::function<void(WebRequest &, WebResponse &)> RouteHandler;

// Mock WebModule namespace with unified handler
namespace WebModule {
  typedef std::function<void(WebRequest &, WebResponse &)> UnifiedRouteHandler;
}

struct WebRoute {
  String path;
  WebModule::Method method;
  RouteHandler handler;
  std::vector<AuthType> authTypes;

  WebRoute(const String &p, WebModule::Method m, RouteHandler h,
           const std::vector<AuthType> &auth)
      : path(p), method(m), handler(h), authTypes(auth) {}

  // Copy constructor for STL containers
  WebRoute(const WebRoute &other)
      : path(other.path), method(other.method), handler(other.handler),
        authTypes(other.authTypes) {}
};

struct ApiRoute {
  String path;
  WebModule::Method method;
  RouteHandler handler;
  std::vector<AuthType> authTypes;
  OpenAPIDocumentation docs;

  ApiRoute(const String &p, WebModule::Method m, RouteHandler h,
           const std::vector<AuthType> &auth,
           const OpenAPIDocumentation &d = OpenAPIDocumentation())
      : path(p), method(m), handler(h), authTypes(auth), docs(d) {}

  // Copy constructor for STL containers
  ApiRoute(const ApiRoute &other)
      : path(other.path), method(other.method), handler(other.handler),
        authTypes(other.authTypes), docs(other.docs) {}
};

// RouteVariant implementation with proper copy semantics
inline RouteVariant::RouteVariant(const WebRoute &wr) : type(WEB_ROUTE) {
  route = new WebRoute(wr);
}

inline RouteVariant::RouteVariant(const ApiRoute &ar) : type(API_ROUTE) {
  route = new ApiRoute(ar);
}

inline RouteVariant::RouteVariant(const RouteVariant &other)
    : type(other.type) {
  if (type == WEB_ROUTE) {
    route = new WebRoute(*static_cast<WebRoute *>(other.route));
  } else {
    route = new ApiRoute(*static_cast<ApiRoute *>(other.route));
  }
}

inline RouteVariant &RouteVariant::operator=(const RouteVariant &other) {
  if (this != &other) {
    // Clean up old route
    if (type == WEB_ROUTE) {
      delete static_cast<WebRoute *>(route);
    } else {
      delete static_cast<ApiRoute *>(route);
    }

    // Copy new route
    type = other.type;
    if (type == WEB_ROUTE) {
      route = new WebRoute(*static_cast<WebRoute *>(other.route));
    } else {
      route = new ApiRoute(*static_cast<ApiRoute *>(other.route));
    }
  }
  return *this;
}

inline RouteVariant::~RouteVariant() {
  if (type == WEB_ROUTE) {
    delete static_cast<WebRoute *>(route);
  } else {
    delete static_cast<ApiRoute *>(route);
  }
}

// Mock IWebModule interface
class IWebModule {
public:
  virtual ~IWebModule() = default;

  // Core interface methods
  virtual void begin() {}
  virtual void handle() {}
  virtual std::vector<RouteVariant> getHttpRoutes() = 0;
  virtual std::vector<RouteVariant> getHttpsRoutes() = 0;
  virtual String getModuleName() const = 0;
  virtual String getModuleVersion() const { return "1.0.0"; }
  virtual String getModuleDescription() const { return "Mock module"; }
};

// Mock API_DOC macros for test compilation
#define API_DOC(summary, description) OpenAPIDocumentation(summary, description)
#define API_DOC_BLOCK(docs) docs

// Mock compilation flags for testing
#ifndef OPENAPI_ENABLED
#ifdef WEB_PLATFORM_OPENAPI
#define OPENAPI_ENABLED 1
#else
#define OPENAPI_ENABLED 0
#endif
#endif

#ifndef MAKERAPI_ENABLED
#ifdef WEB_PLATFORM_MAKERAPI
#define MAKERAPI_ENABLED 1
#else
#define MAKERAPI_ENABLED 0
#endif
#endif

#endif // MOCK_WEB_PLATFORM_H
