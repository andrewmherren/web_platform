#ifndef QUERY_BUILDER_H
#define QUERY_BUILDER_H

#include "database_driver_interface.h"
#include <Arduino.h>
#include <map>

/**
 * QueryBuilder - Fluent interface for database operations
 * 
 * Provides Laravel-style query building with method chaining.
 * Executes operations on the specified driver.
 */
class QueryBuilder {
private:
    IDatabaseDriver* targetDriver;
    String collection;
    std::map<String, String> conditions;
    String selectFields;
    int limitCount;
    String orderField;
    String orderDirection;
    
public:
    /**
     * Constructor
     * @param driver Target database driver
     * @param collectionName Collection to query
     */
    QueryBuilder(IDatabaseDriver* driver, const String& collectionName);
    
    /**
     * Add WHERE condition
     * @param key Field name
     * @param value Field value to match
     * @return Reference to this QueryBuilder for chaining
     */
    QueryBuilder& where(const String& key, const String& value);
    
    /**
     * Set fields to select (currently not implemented in drivers)
     * @param fields Field names ("*" for all)
     * @return Reference to this QueryBuilder for chaining
     */
    QueryBuilder& select(const String& fields = "*");
    
    /**
     * Limit number of results
     * @param count Maximum number of results
     * @return Reference to this QueryBuilder for chaining
     */
    QueryBuilder& limit(int count);
    
    /**
     * Set result ordering (currently not implemented in drivers)
     * @param field Field name to order by
     * @param direction "ASC" or "DESC"
     * @return Reference to this QueryBuilder for chaining
     */
    QueryBuilder& orderBy(const String& field, const String& direction = "ASC");
    
    // Execution methods
    
    /**
     * Get first matching record
     * @return JSON string of first match or empty string
     */
    String get();
    
    /**
     * Get all matching records
     * @return Vector of JSON strings
     */
    std::vector<String> getAll();
    
    /**
     * Check if any matching record exists
     * @return true if at least one match exists
     */
    bool exists();
    
    /**
     * Store data with specified key
     * @param key Unique identifier
     * @param data JSON string to store
     * @return true if stored successfully
     */
    bool store(const String& key, const String& data);
    
    /**
     * Remove all matching records
     * @return true if any records were removed
     */
    bool remove();
    
    /**
     * Get the target driver
     * @return Pointer to target driver
     */
    IDatabaseDriver* getDriver() const;
    
    /**
     * Get the collection name
     * @return Collection name
     */
    String getCollection() const;
};

#endif // QUERY_BUILDER_H