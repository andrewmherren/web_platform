# LittleFS Database Driver

The LittleFS Database Driver provides file-based storage using ESP32's LittleFS filesystem. This driver is ideal for applications that need:

- Larger storage capacity than Preferences/EEPROM
- File-level granularity for efficient access
- Filesystem-based organization
- Direct file access capabilities

## Features

### Core Functionality
- **File-per-key storage**: Each key stored as a separate `.json` file
- **Collection directories**: Collections organized as filesystem directories  
- **Automatic directory creation**: Collections created on-demand
- **Memory-efficient caching**: LRU cache for frequently accessed small files
- **Filesystem safety**: Input validation for safe file/directory names

### Storage Structure
```
/storage/                    (configurable base path)
  /users/
    user123.json
    user456.json
  /config/
    app_settings.json
    feature_flags.json
  /sessions/
    session_abc.json
```

### Performance Optimizations
- **LRU Caching**: Up to 10 small files (<2KB) cached in memory
- **Direct file access**: No need to parse entire collections
- **Efficient listing**: Directory-based key enumeration
- **Memory streaming**: Large files read directly without full memory load

## Usage

### Basic Setup
```cpp
#include "storage/storage_manager.h"
#include "storage/littlefs_database_driver.h"

void setup() {
  // Create LittleFS driver with custom base path
  auto littleFSDriver = std::unique_ptr<IDatabaseDriver>(
    new LittleFSDatabaseDriver("/app_data")
  );
  
  // Register the driver
  StorageManager::configureDriver("littlefs", std::move(littleFSDriver));
  
  // Set as default (optional)
  StorageManager::setDefaultDriver("littlefs");
}
```

### Standard Storage Operations
```cpp
// Store data (automatically creates collection directory)
bool success = StorageManager::query("users")->store("user123", userData);

// Retrieve data (uses cache when available)
String userData = StorageManager::query("users")->get("user123");

// Check existence
bool exists = StorageManager::query("users")->exists("user123");

// List all keys in collection
std::vector<String> userIds = StorageManager::query("users")->keys();

// Remove data
bool removed = StorageManager::query("users")->remove("user123");
```

### LittleFS-Specific Features
```cpp
// Get direct access to LittleFS driver for advanced features
IDatabaseDriver* driver = StorageManager::driver("littlefs");
LittleFSDatabaseDriver* fsDriver = static_cast<LittleFSDatabaseDriver*>(driver);

if (fsDriver) {
  // Filesystem statistics
  String stats = fsDriver->getFilesystemStats();
  // {"total_bytes":1048576,"used_bytes":12345,"free_bytes":1036231,"cache_entries":3}
  
  // Collection management
  size_t collectionSize = fsDriver->getCollectionSize("users");
  bool removed = fsDriver->removeCollection("old_data");
  
  // List all collections
  std::vector<String> collections = fsDriver->listCollections();
  
  // Individual key size
  size_t keySize = fsDriver->getKeySize("users", "user123");
  
  // Cache management
  fsDriver->clearCache();
  
  // Format filesystem (WARNING: destroys all data)
  bool formatted = fsDriver->formatFilesystem();
}
```

## Configuration Options

### Constructor Parameters
```cpp
// Default: stores data in /storage
LittleFSDatabaseDriver();

// Custom base path
LittleFSDatabaseDriver("/app_data");
LittleFSDatabaseDriver("/device_config");
```

### Cache Configuration
The driver includes configurable caching constants:

```cpp
// In littlefs_database_driver.h
static const size_t MAX_CACHE_SIZE = 10;           // Max cached files
static const size_t MAX_CACHED_FILE_SIZE = 2048;   // Max file size to cache
```

## Multiple Driver Architecture

You can use LittleFS alongside other drivers for optimal storage strategy:

```cpp
void setupMultipleDrivers() {
  // JSON driver for small, frequently accessed data
  // (uses Preferences/EEPROM - faster for small data)
  
  // LittleFS driver for larger data and file-based organization
  auto littleFSDriver = std::unique_ptr<IDatabaseDriver>(
    new LittleFSDatabaseDriver()
  );
  StorageManager::configureDriver("littlefs", std::move(littleFSDriver));
  
  // Use different drivers for different purposes
  StorageManager::driver("json")->store("sessions", "current", "user123");
  StorageManager::driver("littlefs")->store("profiles", "user123", largeProfileData);
}
```

## Memory and Performance Considerations

### Memory Usage
- **Cache memory**: Up to ~20KB for cached files (10 files × 2KB max)
- **Stack usage**: Minimal - file operations use streaming
- **Heap usage**: Only for temporary operations (file paths, etc.)

### Performance Characteristics
- **Small files (<2KB)**: Very fast due to caching
- **Large files**: Efficient streaming, no memory pressure  
- **Key enumeration**: Fast directory scanning
- **Collection operations**: Direct filesystem operations

### When to Use LittleFS vs JSON Driver

**Use LittleFS for:**
- Large data objects (>1KB)
- File-based organization needs
- High storage capacity requirements
- When you need direct filesystem access
- Data that changes infrequently

**Use JSON Driver for:**
- Small configuration values
- Session data
- Frequently accessed settings
- Data that needs fastest possible access

## File Safety and Validation

The driver includes comprehensive input validation:

- **Name length limits**: Max 64 characters for collections/keys
- **Invalid character filtering**: Prevents filesystem-unsafe characters
- **Reserved name protection**: Prevents conflicts with filesystem reserved names
- **Automatic extension handling**: `.json` extension added automatically

## Integration with Web Platform

The LittleFS driver integrates seamlessly with the web platform's authentication and storage systems:

```cpp
// Authentication storage can use LittleFS
StorageManager::setDefaultDriver("littlefs");

// All auth operations now use filesystem storage
String userId = AuthStorage::createUser("username", "password");
AuthUser user = AuthStorage::findUserById(userId);
```

This provides persistent user accounts and sessions that survive filesystem operations and device reboots.

## Example Applications

### Device Configuration Management
```cpp
// Store device settings with filesystem persistence
StorageManager::driver("littlefs")->store("device", "network_config", networkJson);
StorageManager::driver("littlefs")->store("device", "sensor_calibration", calibrationJson);
```

### User Profile Storage
```cpp
// Large user profiles with images, preferences, activity logs
String profileData = generateLargeUserProfile(userId);
StorageManager::driver("littlefs")->store("profiles", userId, profileData);
```

### Log File Management
```cpp
// Rotate log files using collection/key structure
String timestamp = getCurrentTimestamp();
StorageManager::driver("littlefs")->store("logs", timestamp, logData);

// List and cleanup old logs
auto logKeys = StorageManager::driver("littlefs")->listKeys("logs");
// ... cleanup logic
```