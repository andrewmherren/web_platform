#include "web_platform.h"
#include "utilities/json_response_builder.h"

#ifdef ESP_PLATFORM

void WebPlatform::createJsonResponse(
    WebResponse &res, std::function<void(JsonObject &)> builder) {
  JsonResponseBuilder::createResponse(res, builder);
}

void WebPlatform::createJsonArrayResponse(
    WebResponse &res, std::function<void(JsonArray &)> builder) {
  JsonResponseBuilder::createArrayResponse(res, builder);
}

#endif // ESP_PLATFORM
