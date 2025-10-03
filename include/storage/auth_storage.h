#ifndef AUTH_STORAGE_NEW_H
#define AUTH_STORAGE_NEW_H

#include "../models/data_models.h"
#include "storage_manager.h"
#include <Arduino.h>
#include <vector>

/**
 * AuthStorage - Storage-driver-based authentication system
 *
 * This class replaces the old AuthStorage with a driver-based approach.
 * Uses StorageManager and QueryBuilder for flexible, scalable storage.
 *
 * Features:
 * - Uses UUID-based primary keys instead of usernames
 * - Supports multiple storage drivers (JSON, cloud databases, etc.)
 * - Laravel-inspired query interface
 * - Proper data model serialization
 *
 * Collections used:
 * - "users" - AuthUser records
 * - "sessions" - AuthSession records
 * - "api_tokens" - AuthApiToken records
 * - "page_tokens" - AuthPageToken records (CSRF)
 */
class AuthStorage {
private:
  static bool initialized;
  static String driverName; // Which driver to use ("" = default)

  // Collection names
  static const String USERS_COLLECTION;
  static const String SESSIONS_COLLECTION;
  static const String API_TOKENS_COLLECTION;
  static const String PAGE_TOKENS_COLLECTION;

  // Helper methods
  static void ensureInitialized();
  static void createDefaultAdminUser();
  static void cleanExpiredData();

public:
  /**
   * Initialize with specific driver (or default)
   * @param driver Driver name to use ("" = default)
   */
  static void initialize(const String &driver = "");

  // User management

  /**
   * Create a new user
   * @param username Username
   * @param password Plain text password (will be hashed)
   * @return User ID if successful, empty string if failed
   */
  static String createUser(const String &username, const String &password);

  /**
   * Find user by ID
   * @param userId User ID
   * @return AuthUser or invalid user if not found
   */
  static AuthUser findUserById(const String &userId);

  /**
   * Find user by username
   * @param username Username to search for
   * @return AuthUser or invalid user if not found
   */
  static AuthUser findUserByUsername(const String &username);

  /**
   * Update user password
   * @param userId User ID
   * @param newPassword New plain text password
   * @return true if updated successfully
   */
  static bool updateUserPassword(const String &userId,
                                 const String &newPassword);

  /**
   * Delete user by ID
   * @param userId User ID to delete
   * @return true if deleted successfully
   */
  static bool deleteUser(const String &userId);

  /**
   * Validate username/password credentials
   * @param username Username
   * @param password Plain text password
   * @return User ID if valid, empty string if invalid
   */
  static String validateCredentials(const String &username,
                                    const String &password);

  /**
   * Get all users
   * @return Vector of AuthUser objects
   */
  static std::vector<AuthUser> getAllUsers();

  /**
   * Check if initial admin setup is required
   * @return true if admin user has no password set (initial setup needed)
   */
  static bool requiresInitialSetup();

  /**
   * Set initial admin password during first setup
   * @param password New password for admin user
   * @return true if password was set successfully
   */
  static bool setInitialAdminPassword(const String &password);

  // Session management

  /**
   * Create a new session
   * @param userId User ID
   * @return Session ID if successful, empty string if failed
   */
  static String createSession(const String &userId);

  /**
   * Find session by ID
   * @param sessionId Session ID
   * @return AuthSession or invalid session if not found
   */
  static AuthSession findSession(
      const String
          &sessionId); /**
                        * Validate session ID
                        * @param sessionId Session ID to validate
                        * @param clientIp Client IP address (optional)
                        * @return User ID if valid, empty string if invalid
                        */
  static String validateSession(const String &sessionId,
                                const String &clientIp = "");

  /**
   * Delete session
   * @param sessionId Session ID to delete
   * @return true if deleted successfully
   */
  static bool deleteSession(const String &sessionId);

  /**
   * Clean expired sessions
   * @return Number of sessions cleaned
   */
  static int cleanExpiredSessions();

  // API Token management

  /**
   * Create API token
   * @param userId User ID
   * @param name Token name/description
   * @param expireInDays Days until expiration (0 = never)
   * @return Token value if successful, empty string if failed
   */
  static String createApiToken(const String &userId, const String &name,
                               unsigned long expireInDays = 0);

  /*** Find API token by value
   * @param token Token value (not ID)
   * @return AuthApiToken or invalid token if not found
   */
  static AuthApiToken findApiToken(const String &token);

  /**
   * Validate API token
   * @param token Token value to validate
   * @return User ID if valid, empty string if invalid
   */
  static String validateApiToken(const String &token);

  /*** Delete API token
   * @param token Token value to delete (not ID)
   * @return true if deleted successfully
   */
  static bool deleteApiToken(const String &token);

  /**
   * Get user's API tokens
   * @param userId User ID
   * @return Vector of user's tokens
   */
  static std::vector<AuthApiToken> getUserApiTokens(const String &userId);

  /**
   * Clean expired API tokens
   * @return Number of tokens cleaned
   */
  static int cleanExpiredApiTokens();

  // Page Token management (CSRF protection)

  /**
   * Create page token for CSRF protection
   * @param clientIp Client IP address
   * @return Token value
   */
  static String createPageToken(const String &clientIp);

  /**
   * Validate page token
   * @param token Token value
   * @param clientIp Client IP address
   * @return true if valid
   */
  static bool validatePageToken(const String &token, const String &clientIp);

  /**
   * Clean expired page tokens
   * @return Number of tokens cleaned
   */
  static int cleanExpiredPageTokens();

  // Utility methods

  /**
   * Get driver being used
   * @return Driver name
   */
  static String getDriverName();

  /**
   * Force cleanup of all expired data
   * @return Total number of records cleaned
   */
  static int cleanupExpiredData();

  /**
   * Get storage statistics
   * @return JSON string with collection counts
   */
  static String getStorageStats();
};

#endif // AUTH_STORAGE_NEW_H