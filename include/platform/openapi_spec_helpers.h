#ifndef OPENAPI_SPEC_HELPERS_H
#define OPENAPI_SPEC_HELPERS_H

#include <ArduinoJson.h>
#include <interface/openapi_generation_context.h>
#include <utility>
#include <vector>

// Pure, WebPlatform-independent pieces of OpenAPI spec generation - moved
// out of WebPlatform (src/platform/web_platform_openapi.cpp) because they
// don't touch any WebPlatform member state, unlike generateAndStoreSpec()/
// generateOpenAPISpec() (which orchestrate route iteration, storage, and
// ESP.getFreeHeap() heap budgeting and stay WebPlatform methods). These had
// zero external callers before the move - previously private WebPlatform
// helper methods used only within that one file.
namespace OpenApiSpecHelpers {

// Builds a human-readable summary like "Get users id" from a path/method
// when a route doesn't supply its own via OpenAPIDocumentation.
String generateDefaultSummary(const String &path, const String &method);

// Builds an OpenAPI-compliant operationId (e.g. "getapi_usersid") by
// stripping characters operationId disallows.
String generateOperationId(const String &method, const String &path);

// "web_module_example" -> "Web Module Example".
String formatModuleName(const String &moduleName);

// Finds which registered module owns a route path (by longest/first
// matching basePath prefix) and returns its formatted display name, or
// "Web Platform" for internal routes. Takes (basePath, moduleName) pairs
// rather than IWebModule pointers so this stays free of the module
// interface entirely.
String inferModuleFromPath(
    const String &path,
    const std::vector<std::pair<String, String>> &registeredModules);

// Whether a route should appear in the Maker API subset, based on whether
// any of its OpenAPIDocumentation tags case-insensitively match a
// configured maker tag.
bool isMakerAPIRoute(
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc,
    const std::vector<String> &makerApiTags);

// Populates the top-level OpenAPI 3.0.3 document skeleton (info, servers,
// security schemes) that every generated spec starts from.
void createOpenAPIDocumentStructure(JsonDocument &doc, const String &title,
                                    const String &description,
                                    const String &systemVersion,
                                    const String &baseUrl);

// Adds path/query parameters (custom + auto-detected {placeholders} +
// access_token for token-authed routes) to a route's OpenAPI operation.
void addParametersToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc);

// Adds response schemas/examples/standard error responses to a route's
// OpenAPI operation.
void addResponsesToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc);

// Adds a requestBody (schema and/or example) to a route's OpenAPI
// operation for POST/PUT routes.
void addRequestBodyToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc);

} // namespace OpenApiSpecHelpers

#endif // OPENAPI_SPEC_HELPERS_H
