#include "web_platform.h"
#include "utilities/json_response_builder.h"

void WebPlatform::createJsonResponse(WebResponse &res,
                                        std::function<void(JsonObject &)> builder) {
    JsonResponseBuilder::createDynamicResponse(res, builder);
}

void WebPlatform::createJsonArrayResponse(WebResponse &res,
                                             std::function<void(JsonArray &)> builder) {
    JsonResponseBuilder::createArrayResponse(res, builder);
}
