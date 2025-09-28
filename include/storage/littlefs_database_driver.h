#ifndef LITTLEFS_DATABASE_DRIVER_H
#define LITTLEFS_DATABASE_DRIVER_H

#include "database_driver_interface.h"
#include <LittleFS.h>
#include <map>

/**
 * LittleFSDatabaseDriver - File-based storage using ESP32 LittleFS
 *
 * Storage structure:
 * /storage/
 *   /collection1/
 *     key1.json
 *     key2.json
 *   /collection2/
 *     key1.json
 *
 * Features:
 * - Each key stored as separate file for efficient access
 * - Collections organized in directories
 * - Automatic directory creation
 * - File-level caching for frequently accessed data
 * - Memory-efficient streaming for large files
 */
class LittleFSDatabaseDriver : public IDatabaseDriver {
private:
  String driverName;
  bool initialized;
  String basePath;

  // Simple cache for frequently accessed small files
  static const size_t MAX_CACHE_SIZE = 10;
  static const size_t MAX_CACHED_FILE_SIZE = 2048; // Cache files up to 2KB
  std::map<String, String> cache;                  // Full path -> content
  std::vector<String> cacheOrder;                  // LRU tracking

  /**
   * Ensure LittleFS is initialized
   */
  void ensureInitialized();

  /**
   * Get full file path for collection/key
   * @param collection Collection name
   * @param key Key name
   * @return Full file path
   */
  String getFilePath(const String &collection, const String &key);

  /**
   * Get collection directory path
   * @param collection Collection name
   * @return Directory path
   */
  String getCollectionPath(const String &collection);

  /**
   * Ensure collection directory exists
   * @param collection Collection name
   * @return true if directory exists or was created
   */
  bool ensureCollectionDirectory(const String &collection);

  /**
   * Add content to cache with LRU eviction
   * @param path File path
   * @param content File content
   */
  void addToCache(const String &path, const String &content);

  /**
   * Get content from cache
   * @param path File path
   * @return Content or empty string if not cached
   */
  String getFromCache(const String &path);

  /**
   * Remove from cache
   * @param path File path
   */
  void removeFromCache(const String &path);

  /**
   * Evict least recently used cache entry
   */
  void evictLRU();

  /**
   * Validate collection and key names for filesystem safety
   * @param name Collection or key name
   * @return true if valid
   */
  bool isValidName(const String &name);

  /**
   * Specialized method for reading large files with improved memory management
   * @param file Open file handle
   * @param fileSize Expected file size
   * @param filePath Path for debugging
   * @return File content or empty string on failure
   */
  String retrieveLargeFile(File &file, size_t fileSize, const String &filePath);

public:
  /**
   * Constructor
   * @param baseStoragePath Base path for storage (default: "/storage")
   */
  explicit LittleFSDatabaseDriver(const String &baseStoragePath = "/storage");

  /**
   * Destructor
   */
  virtual ~LittleFSDatabaseDriver();

  // IDatabaseDriver interface implementation
  bool store(const String &collection, const String &key,
             const String &data) override;
  String retrieve(const String &collection, const String &key) override;
  bool remove(const String &collection, const String &key) override;
  std::vector<String> listKeys(const String &collection) override;
  bool exists(const String &collection, const String &key) override;
  String getDriverName() const override;

  // LittleFS-specific methods

  /**
   * Clear all cached data
   */
  void clearCache();

  /**
   * Get filesystem statistics
   * @return JSON string with total/used/free bytes
   */
  String getFilesystemStats();

  /**
   * Remove entire collection directory
   * @param collection Collection name
   * @return true if removed successfully
   */
  bool removeCollection(const String &collection);

  /**
   * Get size of a specific key's data
   * @param collection Collection name
   * @param key Key name
   * @return Size in bytes, 0 if not found
   */
  size_t getKeySize(const String &collection, const String &key);

  /**
   * Get total size of a collection
   * @param collection Collection name
   * @return Total size in bytes
   */
  size_t getCollectionSize(const String &collection);

  /**
   * List all collections
   * @return Vector of collection names
   */
  std::vector<String> listCollections();

  /**
   * Format LittleFS (WARNING: destroys all data)
   * @return true if formatted successfully
   */
  bool formatFilesystem();
};

#endif // LITTLEFS_DATABASE_DRIVER_H