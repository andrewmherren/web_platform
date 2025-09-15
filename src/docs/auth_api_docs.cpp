#include "../../include/docs/auth_api_docs.h"

// Define module-specific tags
const std::vector<String> AuthApiDocs::USER_MANAGEMENT_TAGS = {
    "User Management"};
const std::vector<String> AuthApiDocs::TOKEN_MANAGEMENT_TAGS = {
    "Token Management"};

// User Management Documentation

OpenAPIDocumentation AuthApiDocs::createListUsers() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "List all users",
      "Retrieves all user accounts. Admin privileges required.", "listUsers",
      USER_MANAGEMENT_TAGS);

  doc.responseExample = R"({
      "success": true,
      "users": [
        {
          "id": "550e8400-e29b-41d4-a716-446655440000",
          "username": "admin",
          "isAdmin": true,
          "createdAt": "2024-01-01T00:00:00Z"
        }
      ]
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("List of user accounts");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createCreateUser() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Create a new user",
      "Creates a new user account. Admin privileges required.", "createUser",
      USER_MANAGEMENT_TAGS);

  doc.requestExample = R"({
      "username": "newuser",
      "password": "securepassword123"
    })";

  doc.responseExample = R"({
      "success": true,
      "user": {
        "id": "550e8400-e29b-41d4-a716-446655440001",
        "username": "newuser",
        "isAdmin": false,
        "createdAt": "2024-01-01T12:00:00Z"
      }
    })";

  doc.requestSchema = R"({
      "type": "object",
      "required": ["username", "password"],
      "properties": {
        "username": {
          "type": "string",
          "minLength": 3,
          "maxLength": 50,
          "description": "Unique username for the new account"
        },
        "password": {
          "type": "string",
          "minLength": 8,
          "description": "Password for the new account"
        }
      }
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Created user account details");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createGetUserById() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Get user by ID",
      "Retrieves detailed information about a specific user by their UUID.",
      "getUserById", USER_MANAGEMENT_TAGS);

  doc.parameters = R"([
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "string",
          "format": "uuid",
          "description": "User UUID"
        }
      }
    ])";

  doc.responseExample = R"({
      "success": true,
      "user": {
        "id": "550e8400-e29b-41d4-a716-446655440000",
        "username": "admin",
        "isAdmin": true,
        "createdAt": "2024-01-01T00:00:00Z"
      }
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("User account details");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createUpdateUserById() {
  OpenAPIDocumentation doc =
      OpenAPIFactory::create("Update user by ID",
                             "Updates user information. Users can update their "
                             "own account, admins can update any account.",
                             "updateUserById", USER_MANAGEMENT_TAGS);

  doc.parameters = R"([
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "string",
          "format": "uuid",
          "description": "User UUID"
        }
      }
    ])";

  doc.requestExample = R"({
      "password": "newsecurepassword123"
    })";

  doc.requestSchema = R"({
      "type": "object",
      "properties": {
        "password": {
          "type": "string",
          "minLength": 8,
          "description": "New password for the account"
        }
      }
    })";

  doc.responseExample = R"({
      "success": true,
      "message": "User updated successfully"
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Update confirmation");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createDeleteUserById() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Delete user by ID",
      "Permanently removes a user account. Admin privileges required. Cannot "
      "delete the last admin user.",
      "deleteUserById", USER_MANAGEMENT_TAGS);

  doc.parameters = R"([
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "string",
          "format": "uuid",
          "description": "User UUID"
        }
      }
    ])";

  doc.responseExample = R"({
      "success": true,
      "message": "User deleted successfully"
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Deletion confirmation");

  return doc;
}

// Current User Convenience Endpoints

OpenAPIDocumentation AuthApiDocs::createGetCurrentUser() {
  OpenAPIDocumentation doc =
      OpenAPIFactory::create("Get current user",
                             "Retrieves information about the currently "
                             "authenticated user based on session or token.",
                             "getCurrentUser", USER_MANAGEMENT_TAGS);

  doc.responseExample = R"({
      "success": true,
      "user": {
        "id": "550e8400-e29b-41d4-a716-446655440000",
        "username": "admin",
        "isAdmin": true,
        "createdAt": "2024-01-01T00:00:00Z"
      }
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Current user account details");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createUpdateCurrentUser() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Update current user",
      "Updates the currently authenticated user's account information.",
      "updateCurrentUser", USER_MANAGEMENT_TAGS);

  doc.requestExample = R"({
      "password": "mynewsecurepassword123"
    })";

  doc.requestSchema = R"({
      "type": "object",
      "properties": {
        "password": {
          "type": "string",
          "minLength": 8,
          "description": "New password for the current user's account"
        }
      }
    })";

  doc.responseExample = R"({
      "success": true,
      "message": "User updated successfully"
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Update confirmation");

  return doc;
}

// Token Management Documentation

OpenAPIDocumentation AuthApiDocs::createGetUserTokens() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Get user tokens",
      "Retrieves all API tokens for a specific user. Users can only access "
      "their own tokens, admins can access any user's tokens.",
      "getUserTokens", TOKEN_MANAGEMENT_TAGS);

  doc.parameters = R"([
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "string",
          "format": "uuid",
          "description": "User UUID"
        }
      }
    ])";

  doc.responseExample = R"({
      "success": true,
      "tokens": [
        {
          "id": "tok_550e8400e29b41d4a716446655440000",
          "name": "My API Token",
          "createdAt": "2024-01-01T12:00:00Z",
          "lastUsed": "2024-01-01T13:30:00Z"
        }
      ]
    })";

  doc.responseSchema =
      OpenAPIFactory::createListResponse("API tokens for the user");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createCreateUserToken() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Create user token",
      "Creates a new API token for a specific user. Users can create tokens "
      "for themselves, admins can create tokens for any user.",
      "createUserToken", TOKEN_MANAGEMENT_TAGS);

  doc.parameters = R"([
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "string",
          "format": "uuid",
          "description": "User UUID"
        }
      }
    ])";

  doc.requestExample = R"({
      "name": "Production API Token"
    })";

  doc.requestSchema = R"({
      "type": "object",
      "required": ["name"],
      "properties": {
        "name": {
          "type": "string",
          "minLength": 1,
          "maxLength": 100,
          "description": "Descriptive name for the API token"
        }
      }
    })";

  doc.responseExample = R"({
      "success": true,
      "token": "tok_550e8400e29b41d4a716446655440000",
      "tokenInfo": {
        "id": "tok_550e8400e29b41d4a716446655440000",
        "name": "Production API Token",
        "createdAt": "2024-01-01T12:00:00Z"
      },
      "warning": "Save this token now - it will not be shown again"
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Created API token details");

  return doc;
}

OpenAPIDocumentation AuthApiDocs::createDeleteToken() {
  OpenAPIDocumentation doc = OpenAPIFactory::create(
      "Delete token",
      "Permanently removes an API token. Users can delete their own tokens, "
      "admins can delete any token.",
      "deleteToken", TOKEN_MANAGEMENT_TAGS);

  doc.parameters = R"([
      {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "string",
          "pattern": "^tok_[a-f0-9]{32}$",
          "description": "API token ID"
        }
      }
    ])";

  doc.responseExample = R"({
      "success": true,
      "message": "Token deleted successfully"
    })";

  doc.responseSchema =
      OpenAPIFactory::createSuccessResponse("Deletion confirmation");

  return doc;
}