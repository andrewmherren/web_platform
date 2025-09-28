#include "docs/auth_api_docs.h"

// Define module-specific tags
const std::vector<String> AuthApiDocs::USER_MANAGEMENT_TAGS = {
    "User Management"};
const std::vector<String> AuthApiDocs::TOKEN_MANAGEMENT_TAGS = {
    "Token Management"};

// User Management Documentation

OpenAPIDocumentation AuthApiDocs::createListUsers() {
  return OpenAPIFactory::create(
             "List all users",
             "Retrieves all user accounts. Admin privileges required.",
             "listUsers", USER_MANAGEMENT_TAGS)
      .withResponseExample(R"({
        "success": true,
        "users": [
          {
            "id": "550e8400-e29b-41d4-a716-446655440000",
            "username": "admin",
            "isAdmin": true,
            "createdAt": "2024-01-01T00:00:00Z"
          }
        ]
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("List of user accounts"));
}

OpenAPIDocumentation AuthApiDocs::createCreateUser() {
  return OpenAPIFactory::create(
             "Create a new user",
             "Creates a new user account. Admin privileges required.",
             "createUser", USER_MANAGEMENT_TAGS)
      .withRequestExample(R"({
        "username": "newuser",
        "password": "securepassword123"
      })")
      .withResponseExample(R"({
        "success": true,
        "user": {
          "id": "550e8400-e29b-41d4-a716-446655440001",
          "username": "newuser",
          "isAdmin": false,
          "createdAt": "2024-01-01T12:00:00Z"
        }
      })")
      .withRequestBody(R"({
        "required": true,
        "content": {
          "application/json": {
            "schema": {
              "type": "object",
              "description": "User creation payload",
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
            }
          }
        }
      })")
      .withResponseSchema(OpenAPIFactory::createSuccessResponse(
          "Created user account details"));
}

OpenAPIDocumentation AuthApiDocs::createGetUserById() {
  return OpenAPIFactory::create("Get user by ID",
                                "Retrieves detailed information about a "
                                "specific user by their UUID.",
                                "getUserById", USER_MANAGEMENT_TAGS)
      .withParameters(R"([
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
      ])")
      .withResponseExample(R"({
        "success": true,
        "user": {
          "id": "550e8400-e29b-41d4-a716-446655440000",
          "username": "admin",
          "isAdmin": true,
          "createdAt": "2024-01-01T00:00:00Z"
        }
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("User account details"));
}

OpenAPIDocumentation AuthApiDocs::createUpdateUserById() {
  return OpenAPIFactory::create(
             "Update user by ID",
             "Updates user information. Users can update their "
             "own account, admins can update any account.",
             "updateUserById", USER_MANAGEMENT_TAGS)
      .withParameters(R"([
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
      ])")
      .withRequestExample(R"({
        "password": "newsecurepassword123"
      })")
      .withRequestBody(R"({
        "required": true,
        "content": {
          "application/json": {
            "schema": {
              "type": "object",
              "description": "User update payload",
              "properties": {
                "password": {
                  "type": "string",
                  "minLength": 8,
                  "description": "New password for the account"
                }
              }
            }
          }
        }
      })")
      .withResponseExample(R"({
        "success": true,
        "message": "User updated successfully"
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Update confirmation"));
}

OpenAPIDocumentation AuthApiDocs::createDeleteUserById() {
  return OpenAPIFactory::create("Delete user by ID",
                                "Permanently removes a user account. Admin "
                                "privileges required. Cannot "
                                "delete the last admin user.",
                                "deleteUserById", USER_MANAGEMENT_TAGS)
      .withParameters(R"([
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
      ])")
      .withResponseExample(R"({
        "success": true,
        "message": "User deleted successfully"
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Deletion confirmation"));
}

// Current User Convenience Endpoints

OpenAPIDocumentation AuthApiDocs::createGetCurrentUser() {
  return OpenAPIFactory::create("Get current user",
                                "Retrieves information about the currently "
                                "authenticated user based on session or token.",
                                "getCurrentUser", USER_MANAGEMENT_TAGS)
      .withResponseExample(R"({
        "success": true,
        "user": {
          "id": "550e8400-e29b-41d4-a716-446655440000",
          "username": "admin",
          "isAdmin": true,
          "createdAt": "2024-01-01T00:00:00Z"
        }
      })")
      .withResponseSchema(OpenAPIFactory::createSuccessResponse(
          "Current user account details"));
}

OpenAPIDocumentation AuthApiDocs::createUpdateCurrentUser() {
  return OpenAPIFactory::create(
             "Update current user",
             "Updates the currently authenticated user's account information.",
             "updateCurrentUser", USER_MANAGEMENT_TAGS)
      .withRequestExample(R"({
        "password": "mynewsecurepassword123"
      })")
      .withRequestBody(R"({
        "required": true,
        "content": {
          "application/json": {
            "schema": {
              "type": "object",
              "description": "Update current user request body",
              "properties": {
                "password": {
                  "type": "string",
                  "minLength": 8,
                  "description": "New password for the current user's account"
                }
              }
            }
          }
        }
      })")
      .withResponseExample(R"({
        "success": true,
        "message": "User updated successfully"
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Update confirmation"));
}

// Token Management Documentation

OpenAPIDocumentation AuthApiDocs::createGetUserTokens() {
  return OpenAPIFactory::create(
             "Get user tokens",
             "Retrieves all API tokens for a specific user. Users can only "
             "access "
             "their own tokens, admins can access any user's tokens.",
             "getUserTokens", TOKEN_MANAGEMENT_TAGS)
      .withParameters(R"([
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
      ])")
      .withResponseExample(R"({
        "success": true,
        "tokens": [
          {
            "id": "tok_550e8400e29b41d4a716446655440000",
            "name": "My API Token",
            "createdAt": "2024-01-01T12:00:00Z",
            "lastUsed": "2024-01-01T13:30:00Z"
          }
        ]
      })")
      .withResponseSchema(
          OpenAPIFactory::createListResponse("API tokens for the user"));
}

OpenAPIDocumentation AuthApiDocs::createCreateUserToken() {
  return OpenAPIFactory::create(
             "Create user token",
             "Creates a new API token for a specific user. Users can create "
             "tokens "
             "for themselves, admins can create tokens for any user.",
             "createUserToken", TOKEN_MANAGEMENT_TAGS)
      .withParameters(R"([
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
      ])")
      .withRequestExample(R"({
        "name": "Production API Token"
      })")
      .withRequestBody(R"({
        "required": true,
        "content": {
          "application/json": {
            "schema": {
              "type": "object",
              "description": "Create user token request body",
              "required": ["name"],
              "properties": {
                "name": {
                  "type": "string",
                  "minLength": 1,
                  "maxLength": 100,
                  "description": "Descriptive name for the API token"
                }
              }
            }
          }
        }
      })")
      .withResponseExample(R"({
        "success": true,
        "token": "tok_550e8400e29b41d4a716446655440000",
        "tokenInfo": {
          "id": "tok_550e8400e29b41d4a716446655440000",
          "name": "Production API Token",
          "createdAt": "2024-01-01T12:00:00Z"
        },
        "warning": "Save this token now - it will not be shown again"
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Created API token details"));
}

OpenAPIDocumentation AuthApiDocs::createDeleteToken() {
  return OpenAPIFactory::create("Delete token",
                                "Permanently removes an API token. Users can "
                                "delete their own tokens, "
                                "admins can delete any token.",
                                "deleteToken", TOKEN_MANAGEMENT_TAGS)
      .withParameters(R"([
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
      ])")
      .withResponseExample(R"({
        "success": true,
        "message": "Token deleted successfully"
      })")
      .withResponseSchema(
          OpenAPIFactory::createSuccessResponse("Deletion confirmation"));
}
