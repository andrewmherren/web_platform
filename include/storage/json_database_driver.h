#ifndef JSON_DATABASE_DRIVER_H
#define JSON_DATABASE_DRIVER_H

#include "database_driver_interface.h"
#include <map>

/**
 * JsonDatabaseDriver - Default storage driver using Preferences
 *
 * Stores data as JSON in ESP32 Preferences.
 * Collections are stored as separate preference keys.
 *
 * Storage format:
 * - Each collection is a JSON array stored under key "collection_name"
 * - Each item in array has format: {"key": "unique_id", "data": {...}}
 *
 * Example storage for "users" collection:
 * [
 *   {"key": "admin", "data": {"username": "admin", "hash": "...", ...}},
 *   {"key": "user1", "data": {"username": "user1", "hash": "...", ...}}
 * ]
 */
class JsonDatabaseDriver : public IDatabaseDriver {
private:
  String driverName;
  bool initialized;

  // In-memory cache for performance
  std::map<String, std::map<String, String>> cache;
  
  // Cache management
  static const size_t MAX_CACHED_COLLECTIONS = 5;

  // Internal methods
  void loadCollection(const String &collection);
  void saveCollection(const String &collection);
  void ensureInitialized();
  void evictOldCollections();
  size_t calculateJsonSize(const std::map<String, String>& collectionData);

public:
  JsonDatabaseDriver();
  virtual ~JsonDatabaseDriver();

  // IDatabaseDriver interface implementation
  bool store(const String &collection, const String &key,
             const String &data) override;
  String retrieve(const String &collection, const String &key) override;
  bool remove(const String &collection, const String &key) override;
  std::vector<String> listKeys(const String &collection) override;
  bool exists(const String &collection, const String &key) override;
  String getDriverName() const override;

  // Additional methods for JsonDatabaseDriver
  void clearCache();
  void clearCollection(const String &collection);
  size_t getCacheSize() const;
  void evictCollection(const String &collection);
};

#endif // JSON_DATABASE_DRIVER_H