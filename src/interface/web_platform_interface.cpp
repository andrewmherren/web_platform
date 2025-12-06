// Provide default (non-ESP) implementations; ESP32-specific versions live in
// src/platform/web_platform_json.cpp
#ifndef ESP_PLATFORM
#include "utilities/json_response_builder.h"
#include "web_platform.h"


void WebPlatform::createJsonResponse(
    WebResponse &res, std::function<void(JsonObject &)> builder) {
  JsonResponseBuilder::createDynamicResponse(res, builder);
}

void WebPlatform::createJsonArrayResponse(
    WebResponse &res, std::function<void(JsonArray &)> builder) {
  JsonResponseBuilder::createArrayResponse(res, builder);
}
#endif // !ESP_PLATFORM
