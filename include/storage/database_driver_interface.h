#ifndef DATABASE_DRIVER_INTERFACE_H
#define DATABASE_DRIVER_INTERFACE_H

#include <Arduino.h>
#include <vector>

/**
 * IDatabaseDriver - Interface for all storage drivers
 * 
 * Provides abstract interface for different storage backends:
 * - JsonDatabaseDriver (Preferences/EEPROM)
 * - LittleFSDriver (flash files) - future
 * - MemoryDriver (RAM only) - future
 * - AWSRDSDriver (cloud database) - future
 */
class IDatabaseDriver {
public:
    /**
     * Store data in a collection with a specific key
     * @param collection Logical grouping (like table name)
     * @param key Unique identifier within collection
     * @param data JSON string to store
     * @return true if stored successfully
     */
    virtual bool store(const String& collection, const String& key, const String& data) = 0;
    
    /**
     * Retrieve data from a collection by key
     * @param collection Logical grouping
     * @param key Unique identifier
     * @return JSON string or empty string if not found
     */
    virtual String retrieve(const String& collection, const String& key) = 0;
    
    /**
     * Remove data from a collection by key
     * @param collection Logical grouping
     * @param key Unique identifier
     * @return true if removed successfully
     */
    virtual bool remove(const String& collection, const String& key) = 0;
    
    /**
     * List all keys in a collection
     * @param collection Logical grouping
     * @return vector of keys
     */
    virtual std::vector<String> listKeys(const String& collection) = 0;
    
    /**
     * Check if a key exists in a collection
     * @param collection Logical grouping
     * @param key Unique identifier
     * @return true if key exists
     */
    virtual bool exists(const String& collection, const String& key) = 0;
    
    /**
     * Get driver name for debugging/logging
     * @return driver name (e.g. "json", "littlefs", "aws-rds")
     */
    virtual String getDriverName() const = 0;
    
    /**
     * Virtual destructor for proper cleanup
     */
    virtual ~IDatabaseDriver() = default;
};

#endif // DATABASE_DRIVER_INTERFACE_H