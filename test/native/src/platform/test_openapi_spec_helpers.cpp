#include "platform/openapi_spec_helpers.h"
#include <unity.h>

using RouteDocumentation = OpenAPIGenerationContext::RouteDocumentation;

namespace {

RouteDocumentation makeRouteDoc(const String &path, WebModule::Method method,
                                const OpenAPIDocumentation &docs,
                                const AuthRequirements &auth = {}) {
  return RouteDocumentation(path, method, docs, auth);
}

} // namespace

// --- generateDefaultSummary ---

void test_generate_default_summary_capitalizes_method(void) {
  String summary =
      OpenApiSpecHelpers::generateDefaultSummary("/api/widgets", "get");
  TEST_ASSERT_EQUAL_STRING("Get widgets", summary.c_str());
}

void test_generate_default_summary_strips_api_prefix_and_separators(void) {
  String summary = OpenApiSpecHelpers::generateDefaultSummary(
      "/api/user_tokens-list", "post");
  TEST_ASSERT_EQUAL_STRING("Post user tokens list", summary.c_str());
}

void test_generate_default_summary_defaults_to_endpoint_for_empty_path(void) {
  String summary = OpenApiSpecHelpers::generateDefaultSummary("/api/", "get");
  TEST_ASSERT_EQUAL_STRING("Get endpoint", summary.c_str());
}

// --- generateOperationId ---

void test_generate_operation_id_sanitizes_special_characters(void) {
  String opId =
      OpenApiSpecHelpers::generateOperationId("get", "/api/users/{id}.json");
  TEST_ASSERT_EQUAL_STRING("get_api_users_id_json", opId.c_str());
}

// --- formatModuleName ---

void test_format_module_name_capitalizes_words(void) {
  TEST_ASSERT_EQUAL_STRING(
      "Web Module Example",
      OpenApiSpecHelpers::formatModuleName("web_module_example").c_str());
}

void test_format_module_name_handles_dashes(void) {
  TEST_ASSERT_EQUAL_STRING(
      "Usb Pd Controller",
      OpenApiSpecHelpers::formatModuleName("usb-pd-controller").c_str());
}

// --- inferModuleFromPath ---

void test_infer_module_from_path_matches_registered_module(void) {
  std::vector<std::pair<String, String>> modules = {
      {"/api/maker", "Maker API"}, {"/api/usb", "usb_pd_controller"}};
  String tag =
      OpenApiSpecHelpers::inferModuleFromPath("/api/usb/status", modules);
  TEST_ASSERT_EQUAL_STRING("Usb Pd Controller", tag.c_str());
}

void test_infer_module_from_path_falls_back_to_web_platform(void) {
  std::vector<std::pair<String, String>> modules = {
      {"/api/usb", "usb_pd_controller"}};
  String tag = OpenApiSpecHelpers::inferModuleFromPath("/login", modules);
  TEST_ASSERT_EQUAL_STRING("Web Platform", tag.c_str());
}

// --- isMakerAPIRoute ---

void test_is_maker_api_route_matches_case_insensitively(void) {
  OpenAPIDocumentation docs("summary", "desc", "opId", {"Maker"});
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/x", WebModule::WM_GET, docs);
  std::vector<String> makerTags = {"maker"};
  TEST_ASSERT_TRUE(OpenApiSpecHelpers::isMakerAPIRoute(routeDoc, makerTags));
}

void test_is_maker_api_route_false_when_no_tag_matches(void) {
  OpenAPIDocumentation docs("summary", "desc", "opId", {"Admin"});
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/x", WebModule::WM_GET, docs);
  std::vector<String> makerTags = {"maker"};
  TEST_ASSERT_FALSE(OpenApiSpecHelpers::isMakerAPIRoute(routeDoc, makerTags));
}

// --- createOpenAPIDocumentStructure ---

void test_create_openapi_document_structure_populates_expected_fields(void) {
  JsonDocument doc;
  OpenApiSpecHelpers::createOpenAPIDocumentStructure(
      doc, "My API", "A description", "1.2.3", "http://device.local");

  TEST_ASSERT_EQUAL_STRING("3.0.3", doc["openapi"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING("My API",
                           doc["info"]["title"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING("1.2.3",
                           doc["info"]["version"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING(
      "http://device.local",
      doc["servers"][0]["url"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING(
      "bearer",
      doc["components"]["securitySchemes"]["bearerAuth"]["scheme"]
          .as<std::string>()
          .c_str());
  TEST_ASSERT_EQUAL_STRING(
      "session",
      doc["components"]["securitySchemes"]["cookieAuth"]["name"]
          .as<std::string>()
          .c_str());
}

// --- addParametersToOperationFromDocs ---

void test_add_parameters_extracts_path_placeholders(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc = makeRouteDoc(
      "/api/users/{userId}/tokens/{tokenId}", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonArray parameters = operation["parameters"];
  TEST_ASSERT_EQUAL(2, parameters.size());
  TEST_ASSERT_EQUAL_STRING(
      "userId", parameters[0]["name"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING(
      "User identifier (UUID)",
      parameters[0]["description"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING(
      "uuid", parameters[0]["schema"]["format"].as<std::string>().c_str());
}

void test_add_parameters_adds_access_token_for_token_auth_routes(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc = makeRouteDoc(
      "/api/widgets", WebModule::WM_GET, docs, {AuthType::TOKEN});

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonArray parameters = operation["parameters"];
  bool foundAccessToken = false;
  for (JsonObject p : parameters) {
    if (String(p["name"].as<std::string>().c_str()) == "access_token") {
      foundAccessToken = true;
    }
  }
  TEST_ASSERT_TRUE(foundAccessToken);
}

void test_add_parameters_no_placeholders_no_token_auth_yields_empty(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonArray parameters = operation["parameters"];
  TEST_ASSERT_EQUAL(0, parameters.size());
}

void test_add_parameters_includes_custom_parameters_from_docs(void) {
  OpenAPIDocumentation docs =
      OpenAPIDocumentation("summary").withParameters(
          "[{\"name\":\"limit\",\"in\":\"query\"}]");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonArray parameters = operation["parameters"];
  TEST_ASSERT_EQUAL(1, parameters.size());
  TEST_ASSERT_EQUAL_STRING("limit",
                           parameters[0]["name"].as<std::string>().c_str());
}

void test_add_parameters_custom_parameters_deduplicate_against_path(void) {
  // A custom "id"/"path" parameter should suppress the auto-generated one
  // for the same {id} placeholder, not add a duplicate.
  OpenAPIDocumentation docs =
      OpenAPIDocumentation("summary").withParameters(
          "[{\"name\":\"id\",\"in\":\"path\",\"description\":\"custom\"}]");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets/{id}", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonArray parameters = operation["parameters"];
  TEST_ASSERT_EQUAL(1, parameters.size());
  TEST_ASSERT_EQUAL_STRING(
      "custom", parameters[0]["description"].as<std::string>().c_str());
}

void test_add_parameters_generic_id_uses_resource_identifier_description(
    void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets/{id}", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonObject param = operation["parameters"][0];
  TEST_ASSERT_EQUAL_STRING("Resource identifier",
                           param["description"].as<std::string>().c_str());
  TEST_ASSERT_EQUAL_STRING("string",
                           param["schema"]["type"].as<std::string>().c_str());
  TEST_ASSERT_TRUE(param["schema"]["format"].isNull());
}

void test_add_parameters_generic_name_uses_fallback_description(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets/{slug}", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonObject param = operation["parameters"][0];
  TEST_ASSERT_EQUAL_STRING("Path parameter: slug",
                           param["description"].as<std::string>().c_str());
}

void test_add_parameters_ignores_unclosed_brace(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets/{unterminated", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  // Should not hang or crash on a malformed path - just stops extracting.
  OpenApiSpecHelpers::addParametersToOperationFromDocs(operation, routeDoc);

  JsonArray parameters = operation["parameters"];
  TEST_ASSERT_EQUAL(0, parameters.size());
}

// --- addResponsesToOperationFromDocs ---

void test_add_responses_includes_success_and_server_error(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addResponsesToOperationFromDocs(operation, routeDoc);

  JsonObject responses = operation["responses"];
  TEST_ASSERT_FALSE(responses["200"].isNull());
  TEST_ASSERT_FALSE(responses["500"].isNull());
  TEST_ASSERT_TRUE(responses["401"].isNull());
}

void test_add_responses_includes_auth_errors_when_route_requires_auth(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc = makeRouteDoc(
      "/api/widgets", WebModule::WM_GET, docs, {AuthType::SESSION});

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addResponsesToOperationFromDocs(operation, routeDoc);

  JsonObject responses = operation["responses"];
  TEST_ASSERT_FALSE(responses["401"].isNull());
  TEST_ASSERT_FALSE(responses["403"].isNull());
}

void test_add_responses_merges_response_schema(void) {
  OpenAPIDocumentation docs =
      OpenAPIDocumentation("summary").withResponseSchema(
          "{\"type\":\"object\"}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addResponsesToOperationFromDocs(operation, routeDoc);

  JsonObject schema =
      operation["responses"]["200"]["content"]["application/json"]["schema"];
  TEST_ASSERT_EQUAL_STRING("object", schema["type"].as<std::string>().c_str());
}

void test_add_responses_merges_response_example(void) {
  OpenAPIDocumentation docs =
      OpenAPIDocumentation("summary").withResponseExample(
          "{\"id\":\"abc123\"}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addResponsesToOperationFromDocs(operation, routeDoc);

  JsonObject example =
      operation["responses"]["200"]["content"]["application/json"]["example"];
  TEST_ASSERT_EQUAL_STRING("abc123", example["id"].as<std::string>().c_str());
}

void test_add_responses_merges_custom_responses_json(void) {
  OpenAPIDocumentation docs =
      OpenAPIDocumentation("summary").withResponses(
          "{\"404\":{\"description\":\"Not found\"}}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addResponsesToOperationFromDocs(operation, routeDoc);

  JsonObject responses = operation["responses"];
  TEST_ASSERT_EQUAL_STRING(
      "Not found", responses["404"]["description"].as<std::string>().c_str());
  // Custom responsesJson should not clobber the default 200.
  TEST_ASSERT_EQUAL_STRING(
      "Successful operation",
      responses["200"]["description"].as<std::string>().c_str());
}

// --- addRequestBodyToOperationFromDocs ---

void test_add_request_body_defaults_for_post_with_no_docs(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  JsonObject requestBody = operation["requestBody"];
  TEST_ASSERT_FALSE(requestBody.isNull());
  TEST_ASSERT_TRUE(requestBody["required"].as<bool>());
}

void test_add_request_body_none_for_get_with_no_docs(void) {
  OpenAPIDocumentation docs("summary");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_GET, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  TEST_ASSERT_TRUE(operation["requestBody"].isNull());
}

void test_add_request_body_wraps_simple_schema(void) {
  OpenAPIDocumentation docs = OpenAPIDocumentation("summary").withRequestBody(
      "{\"type\":\"object\",\"properties\":{}}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  JsonObject schema = operation["requestBody"]["content"]["application/json"]
                                ["schema"];
  TEST_ASSERT_EQUAL_STRING("object", schema["type"].as<std::string>().c_str());
}

void test_add_request_body_passes_through_complete_request_body(void) {
  OpenAPIDocumentation docs = OpenAPIDocumentation("summary").withRequestBody(
      "{\"required\":true,\"content\":{\"application/json\":{\"schema\":{"
      "\"type\":\"object\"}}}}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  JsonObject requestBody = operation["requestBody"];
  TEST_ASSERT_TRUE(requestBody["required"].as<bool>());
  TEST_ASSERT_FALSE(requestBody["content"]["application/json"].isNull());
}

void test_add_request_body_malformed_schema_falls_back_to_generic_object(
    void) {
  OpenAPIDocumentation docs = OpenAPIDocumentation("summary").withRequestBody(
      "not valid json");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  JsonObject requestBody = operation["requestBody"];
  TEST_ASSERT_TRUE(requestBody["required"].as<bool>());
  JsonObject schema =
      requestBody["content"]["application/json"]["schema"];
  TEST_ASSERT_EQUAL_STRING("object", schema["type"].as<std::string>().c_str());
}

void test_add_request_body_complete_request_body_uses_explicit_description(
    void) {
  OpenAPIDocumentation docs = OpenAPIDocumentation("summary").withRequestBody(
      "{\"description\":\"Custom payload\",\"required\":true,\"content\":{"
      "\"application/json\":{\"schema\":{\"type\":\"object\"}}}}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  TEST_ASSERT_EQUAL_STRING(
      "Custom payload",
      operation["requestBody"]["description"].as<std::string>().c_str());
}

void test_add_request_body_schema_and_example_together(void) {
  OpenAPIDocumentation docs = OpenAPIDocumentation("summary")
                                  .withRequestBody("{\"type\":\"object\"}")
                                  .withRequestExample("{\"name\":\"widget\"}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  JsonObject example = operation["requestBody"]["content"]["application/json"]
                                 ["example"];
  TEST_ASSERT_EQUAL_STRING("widget", example["name"].as<std::string>().c_str());
}

void test_add_request_body_example_only(void) {
  OpenAPIDocumentation docs = OpenAPIDocumentation("summary").withRequestExample(
      "{\"name\":\"widget\"}");
  RouteDocumentation routeDoc =
      makeRouteDoc("/api/widgets", WebModule::WM_POST, docs);

  JsonDocument doc;
  JsonObject operation = doc.to<JsonObject>();
  OpenApiSpecHelpers::addRequestBodyToOperationFromDocs(operation, routeDoc);

  JsonObject requestBody = operation["requestBody"];
  TEST_ASSERT_TRUE(requestBody["required"].as<bool>());
  JsonObject example = requestBody["content"]["application/json"]["example"];
  TEST_ASSERT_EQUAL_STRING("widget", example["name"].as<std::string>().c_str());
}

void register_openapi_spec_helpers_tests(void) {
  RUN_TEST(test_generate_default_summary_capitalizes_method);
  RUN_TEST(test_generate_default_summary_strips_api_prefix_and_separators);
  RUN_TEST(test_generate_default_summary_defaults_to_endpoint_for_empty_path);

  RUN_TEST(test_generate_operation_id_sanitizes_special_characters);

  RUN_TEST(test_format_module_name_capitalizes_words);
  RUN_TEST(test_format_module_name_handles_dashes);

  RUN_TEST(test_infer_module_from_path_matches_registered_module);
  RUN_TEST(test_infer_module_from_path_falls_back_to_web_platform);

  RUN_TEST(test_is_maker_api_route_matches_case_insensitively);
  RUN_TEST(test_is_maker_api_route_false_when_no_tag_matches);

  RUN_TEST(test_create_openapi_document_structure_populates_expected_fields);

  RUN_TEST(test_add_parameters_extracts_path_placeholders);
  RUN_TEST(test_add_parameters_adds_access_token_for_token_auth_routes);
  RUN_TEST(test_add_parameters_no_placeholders_no_token_auth_yields_empty);
  RUN_TEST(test_add_parameters_includes_custom_parameters_from_docs);
  RUN_TEST(test_add_parameters_custom_parameters_deduplicate_against_path);
  RUN_TEST(test_add_parameters_generic_id_uses_resource_identifier_description);
  RUN_TEST(test_add_parameters_generic_name_uses_fallback_description);
  RUN_TEST(test_add_parameters_ignores_unclosed_brace);

  RUN_TEST(test_add_responses_includes_success_and_server_error);
  RUN_TEST(test_add_responses_includes_auth_errors_when_route_requires_auth);
  RUN_TEST(test_add_responses_merges_response_schema);
  RUN_TEST(test_add_responses_merges_response_example);
  RUN_TEST(test_add_responses_merges_custom_responses_json);

  RUN_TEST(test_add_request_body_defaults_for_post_with_no_docs);
  RUN_TEST(test_add_request_body_none_for_get_with_no_docs);
  RUN_TEST(test_add_request_body_wraps_simple_schema);
  RUN_TEST(test_add_request_body_passes_through_complete_request_body);
  RUN_TEST(test_add_request_body_malformed_schema_falls_back_to_generic_object);
  RUN_TEST(test_add_request_body_complete_request_body_uses_explicit_description);
  RUN_TEST(test_add_request_body_schema_and_example_together);
  RUN_TEST(test_add_request_body_example_only);
}
