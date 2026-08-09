#include "platform/openapi_spec_helpers.h"
#include <interface/auth_types.h>
#include <map>
#include <string>

namespace OpenApiSpecHelpers {

String generateDefaultSummary(const String &path, const String &method) {
  String summary = method.substring(0, 1);
  summary.toUpperCase();
  summary += method.substring(1) + " ";

  String cleanPath = path;
  cleanPath.replace("/api/", "");
  cleanPath.replace("/", " ");
  cleanPath.replace("_", " ");
  cleanPath.replace("-", " ");

  if (cleanPath.length() == 0) {
    summary += "endpoint";
  } else {
    summary += cleanPath;
  }

  return summary;
}

String generateOperationId(const String &method, const String &path) {
  String operationId = method + path;
  operationId.replace("/", "_");
  operationId.replace("-", "_");
  operationId.replace(".", "_");
  operationId.replace("{", "");
  operationId.replace("}", "");
  return operationId;
}

String formatModuleName(const String &moduleName) {
  String formatted = moduleName;

  formatted.replace("_", " ");
  formatted.replace("-", " ");

  bool capitalizeNext = true;
  for (size_t i = 0; i < formatted.length(); i++) {
    if (capitalizeNext && formatted[i] >= 'a' && formatted[i] <= 'z') {
      formatted[i] = formatted[i] - 'a' + 'A';
      capitalizeNext = false;
    } else if (formatted[i] == ' ') {
      capitalizeNext = true;
    } else {
      capitalizeNext = false;
    }
  }

  return formatted;
}

String inferModuleFromPath(
    const String &path,
    const std::vector<std::pair<String, String>> &registeredModules) {
  String moduleName = "";

  for (const auto &mod : registeredModules) {
    if (path.startsWith(mod.first)) {
      moduleName = mod.second;
      break;
    }
  }

  if (moduleName.length() != 0) {
    return formatModuleName(moduleName);
  }

  // All WebPlatform internal routes (including auth routes) should return
  // "Web Platform". The specific functional tags like "User Management"
  // are added as explicit tags instead.
  return "Web Platform";
}

bool isMakerAPIRoute(
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc,
    const std::vector<String> &makerApiTags) {
  for (const String &routeTag : routeDoc.docs.getTags()) {
    for (const String &makerTag : makerApiTags) {
      if (routeTag.equalsIgnoreCase(makerTag)) {
        return true;
      }
    }
  }
  return false;
}

void createOpenAPIDocumentStructure(JsonDocument &doc, const String &title,
                                    const String &description,
                                    const String &systemVersion,
                                    const String &baseUrl) {
  doc["openapi"] = "3.0.3";

  JsonObject info = doc["info"].to<JsonObject>();
  info["title"] = title;
  info["description"] = description;
  info["version"] = systemVersion;

  JsonArray servers = doc["servers"].to<JsonArray>();
  JsonObject server = servers.add<JsonObject>();
  server["url"] = baseUrl;
  server["description"] = "Device API Server";

  JsonObject components = doc["components"].to<JsonObject>();
  JsonObject securitySchemes = components["securitySchemes"].to<JsonObject>();

  JsonObject bearerAuth = securitySchemes["bearerAuth"].to<JsonObject>();
  bearerAuth["type"] = "http";
  bearerAuth["scheme"] = "bearer";
  bearerAuth["bearerFormat"] = "JWT";

  JsonObject cookieAuth = securitySchemes["cookieAuth"].to<JsonObject>();
  cookieAuth["type"] = "apiKey";
  cookieAuth["in"] = "cookie";
  cookieAuth["name"] = "session";

  JsonObject tokenParam = securitySchemes["tokenParam"].to<JsonObject>();
  tokenParam["type"] = "apiKey";
  tokenParam["in"] = "query";
  tokenParam["name"] = "access_token";
}

void addParametersToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) {
#if OPENAPI_ENABLED
  JsonArray parameters = operation["parameters"].to<JsonArray>();
  const OpenAPIDocumentation &docs = routeDoc.docs;

  std::map<String, bool> parameterNames;

  if (docs.getParameters().length() != 0) {
    JsonDocument paramDoc;
    if (deserializeJson(paramDoc, docs.getParameters().c_str()) ==
        DeserializationError::Ok) {
      if (paramDoc.is<JsonArray>()) {
        JsonArray customParams = paramDoc.as<JsonArray>();
        for (JsonVariant param : customParams) {
          if (param.is<JsonObject>()) {
            JsonObject paramObj = param.as<JsonObject>();
            if (!paramObj["name"].isNull() && !paramObj["in"].isNull()) {
              String paramName =
                  String(paramObj["name"].as<std::string>().c_str());
              String paramIn =
                  String(paramObj["in"].as<std::string>().c_str());
              String paramKey = paramName + ":" + paramIn;

              if (parameterNames.find(paramKey) == parameterNames.end()) {
                parameters.add(param);
                parameterNames[paramKey] = true;
              }
            }
          }
        }
      }
    }
  }

  String routePathStr = routeDoc.path;
  if (routePathStr.indexOf("{") != -1) {
    String pathCopy = routePathStr;
    int startPos = 0;
    while ((startPos = pathCopy.indexOf("{", startPos)) != -1) {
      int endPos = pathCopy.indexOf("}", startPos);
      if (endPos != -1) {
        String paramName = pathCopy.substring(startPos + 1, endPos);
        String paramKey = paramName + ":path";

        if (parameterNames.find(paramKey) == parameterNames.end()) {
          JsonObject param = parameters.add<JsonObject>();
          param["name"] = paramName;
          param["in"] = "path";
          param["required"] = true;

          if (paramName == "id") {
            param["description"] = "Resource identifier";
          } else if (paramName == "userId") {
            param["description"] = "User identifier (UUID)";
          } else if (paramName == "tokenId") {
            param["description"] = "Token identifier";
          } else {
            param["description"] = "Path parameter: " + paramName;
          }

          JsonObject schema = param["schema"].to<JsonObject>();
          if (paramName.endsWith("Id") && paramName != "id") {
            schema["type"] = "string";
            schema["format"] = "uuid";
          } else {
            schema["type"] = "string";
          }

          parameterNames[paramKey] = true;
        }

        startPos = endPos + 1;
      } else {
        break;
      }
    }
  }

  bool hasTokenAuth = false;
  for (const auto &authType : routeDoc.authRequirements) {
    if (authType == AuthType::TOKEN) {
      hasTokenAuth = true;
      break;
    }
  }

  if (hasTokenAuth &&
      parameterNames.find("access_token:query") == parameterNames.end()) {
    JsonObject tokenParam = parameters.add<JsonObject>();
    tokenParam["name"] = "access_token";
    tokenParam["in"] = "query";
    tokenParam["required"] = false;
    tokenParam["description"] =
        "API access token (alternative to Bearer header)";
    JsonObject tokenSchema = tokenParam["schema"].to<JsonObject>();
    tokenSchema["type"] = "string";
    parameterNames["access_token:query"] = true;
  }
#endif
}

void addResponsesToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) {
#if OPENAPI_ENABLED
  JsonObject responses = operation["responses"].to<JsonObject>();
  const OpenAPIDocumentation &docs = routeDoc.docs;

  JsonObject response200 = responses["200"].to<JsonObject>();
  response200["description"] = "Successful operation";

  JsonObject content = response200["content"].to<JsonObject>();
  String contentType = "application/json";
  JsonObject mediaType = content[contentType].to<JsonObject>();

  if (docs.getResponseSchema().length() != 0) {
    JsonDocument schemaDoc;
    if (deserializeJson(schemaDoc, docs.getResponseSchema().c_str()) ==
        DeserializationError::Ok) {
      mediaType["schema"] = schemaDoc.as<JsonObject>();
    }
  }

  if (docs.getResponseExample().length() != 0) {
    JsonDocument exampleDoc;
    if (deserializeJson(exampleDoc, docs.getResponseExample().c_str()) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
  }

  if (docs.getResponsesJson().length() != 0) {
    JsonDocument responseDoc;
    if (deserializeJson(responseDoc, docs.getResponsesJson().c_str()) ==
        DeserializationError::Ok) {
      for (JsonPair kv : responseDoc.as<JsonObject>()) {
        if (responses[kv.key().c_str()].isNull()) {
          responses[kv.key().c_str()] = kv.value();
        }
      }
    }
  }

  if (!routeDoc.authRequirements.empty()) {
    JsonObject response401 = responses["401"].to<JsonObject>();
    response401["description"] = "Unauthorized - Authentication required";

    JsonObject response403 = responses["403"].to<JsonObject>();
    response403["description"] = "Forbidden - Insufficient permissions";
  }

  JsonObject response500 = responses["500"].to<JsonObject>();
  response500["description"] = "Internal server error";
#endif
}

void addRequestBodyToOperationFromDocs(
    JsonObject &operation,
    const OpenAPIGenerationContext::RouteDocumentation &routeDoc) {
#if OPENAPI_ENABLED
  const OpenAPIDocumentation &docs = routeDoc.docs;

  if (docs.getRequestSchema().length() == 0 && docs.getRequestExample().length() == 0) {
    if (routeDoc.method == WebModule::WM_POST ||
        routeDoc.method == WebModule::WM_PUT) {
      JsonObject requestBody = operation["requestBody"].to<JsonObject>();
      requestBody["description"] = "Request payload";
      requestBody["required"] = true;
      JsonObject content = requestBody["content"].to<JsonObject>();
      JsonObject mediaType = content["application/json"].to<JsonObject>();
      JsonObject schema = mediaType["schema"].to<JsonObject>();
      schema["type"] = "object";
    }
    return;
  }

  if (docs.getRequestSchema().length() != 0) {
    JsonDocument schemaDoc;
    DeserializationError error =
        deserializeJson(schemaDoc, docs.getRequestSchema().c_str());

    if (error == DeserializationError::Ok) {
      JsonObject schemaObj = schemaDoc.as<JsonObject>();

      bool isCompleteRequestBody = !schemaObj["content"].isNull() &&
                                   !schemaObj["required"].isNull();

      JsonObject requestBody = operation["requestBody"].to<JsonObject>();

      if (isCompleteRequestBody) {
        if (!schemaObj["description"].isNull()) {
          requestBody["description"] = schemaObj["description"];
        } else {
          requestBody["description"] = "Request payload";
        }

        if (!schemaObj["required"].isNull()) {
          requestBody["required"] = schemaObj["required"];
        }

        if (!schemaObj["content"].isNull()) {
          requestBody["content"] = schemaObj["content"];
        }
      } else {
        requestBody["description"] = "Request payload";
        requestBody["required"] = true;
        JsonObject content = requestBody["content"].to<JsonObject>();
        JsonObject mediaType = content["application/json"].to<JsonObject>();
        mediaType["schema"] = schemaObj;
      }
    } else {
      JsonObject requestBody = operation["requestBody"].to<JsonObject>();
      requestBody["description"] = "Request payload";
      requestBody["required"] = true;
      JsonObject content = requestBody["content"].to<JsonObject>();
      JsonObject mediaType = content["application/json"].to<JsonObject>();
      JsonObject schema = mediaType["schema"].to<JsonObject>();
      schema["type"] = "object";
    }

    schemaDoc.clear();

    if (docs.getRequestExample().length() != 0) {
      JsonObject requestBody = operation["requestBody"];
      if (!requestBody["content"].isNull()) {
        JsonObject content = requestBody["content"];
        for (JsonPair contentType : content) {
          JsonObject mediaType = contentType.value().as<JsonObject>();
          if (mediaType) {
            JsonDocument exampleDoc;
            if (deserializeJson(exampleDoc, docs.getRequestExample().c_str()) ==
                DeserializationError::Ok) {
              mediaType["example"] = exampleDoc.as<JsonVariant>();
            }
            exampleDoc.clear();
            break;
          }
        }
      }
    }

  } else if (docs.getRequestExample().length() != 0) {
    JsonObject requestBody = operation["requestBody"].to<JsonObject>();
    requestBody["description"] = "Request payload";
    requestBody["required"] = true;
    JsonObject content = requestBody["content"].to<JsonObject>();
    JsonObject mediaType = content["application/json"].to<JsonObject>();
    JsonObject schema = mediaType["schema"].to<JsonObject>();
    schema["type"] = "object";

    JsonDocument exampleDoc;
    if (deserializeJson(exampleDoc, docs.getRequestExample().c_str()) ==
        DeserializationError::Ok) {
      mediaType["example"] = exampleDoc.as<JsonVariant>();
    }
    exampleDoc.clear();
  }
#endif
}

} // namespace OpenApiSpecHelpers
