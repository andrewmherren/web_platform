#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "database_driver_interface.h"
#include "query_builder.h"
#include <Arduino.h>
#include <map>
#include <memory>

/**
 * StorageManager - Laravel DB-inspired storage manager
 *
 * Provides centralized management of multiple database drivers.
 * Allows different storage backends for different collections or use cases.
 *
 * Usage patterns:
 * - StorageManager::query("users").where("username", "admin").get()
 * - StorageManager::driver("cloud").query("logs").getAll()
 * - StorageManager::setDefaultDriver("json")
 */
class StorageManager {
private:
  static std::map<String, std::unique_ptr<IDatabaseDriver>> drivers;
  static String defaultDriverName;
  static bool initialized;

  // Ensure default JSON driver exists
  static void ensureInitialized();

public:
  /**
   * Configure a named database driver
   * @param name Driver name (e.g. "json", "cloud", "cache")
   * @param driver Unique pointer to driver instance
   */
  static void configureDriver(const String &name,
                              std::unique_ptr<IDatabaseDriver> driver);

  /**
   * Set the default driver name
   * @param name Driver name to use as default
   * @return true if driver exists and was set as default
   */
  static bool setDefaultDriver(const String &name);

  /**
   * Get a driver by name
   * @param name Driver name ("" for default driver)
   * @return Pointer to driver or nullptr if not found
   */
  static IDatabaseDriver &driver(const String &name = "");

  /**
   * Start a query on the default driver
   * @param collection Collection name
   * @return QueryBuilder instance
   */
  static QueryBuilder query(const String &collection);

  /**
   * Get list of configured driver names
   * @return Vector of driver names
   */
  static std::vector<String> getDriverNames();

  /**
   * Get current default driver name
   * @return Default driver name
   */
  static String getDefaultDriverName();

  /**
   * Remove a driver by name
   * @param name Driver name to remove
   * @return true if driver was removed
   */
  static bool removeDriver(const String &name);

  /**
   * Clear all drivers (useful for testing)
   */
  static void clearAllDrivers();
};

// Inline implementation for driver-specific query builder
inline QueryBuilder StorageManager::query(const String &collection) {
  ensureInitialized();
  return QueryBuilder(&driver(), collection);
}

#endif // STORAGE_MANAGER_H