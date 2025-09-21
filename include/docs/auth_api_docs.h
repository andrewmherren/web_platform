#ifndef AUTH_API_DOCS_H
#define AUTH_API_DOCS_H

#include "../interface/openapi_factory.h"
#include <vector>

#if OPENAPI_ENABLED
/**
 * Documentation factory for authentication-related API endpoints
 * Following the recommended pattern of separating documentation from implementation
 */
class AuthApiDocs {
public:
    // Module-specific tags for grouping endpoints
    static const std::vector<String> USER_MANAGEMENT_TAGS;
    static const std::vector<String> TOKEN_MANAGEMENT_TAGS;
    
    // User Management Documentation
    static OpenAPIDocumentation createListUsers();
    static OpenAPIDocumentation createCreateUser();
    static OpenAPIDocumentation createGetUserById();
    static OpenAPIDocumentation createUpdateUserById();
    static OpenAPIDocumentation createDeleteUserById();
    
    // Current User Convenience Endpoints
    static OpenAPIDocumentation createGetCurrentUser();
    static OpenAPIDocumentation createUpdateCurrentUser();
    
    // Token Management Documentation
    static OpenAPIDocumentation createGetUserTokens();
    static OpenAPIDocumentation createCreateUserToken();
    static OpenAPIDocumentation createDeleteToken();
};

#endif // OPENAPI_ENABLED

#endif