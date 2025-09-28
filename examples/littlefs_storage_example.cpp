/**
 * LittleFS Database Driver Example
 * 
 * Demonstrates how to use the LittleFSDatabaseDriver for persistent storage
 * on ESP32 devices using the filesystem instead of Preferences/EEPROM.
 */

#include "storage/storage_manager.h"
#include "storage/littlefs_database_driver.h"
#include <ArduinoJson.h>

void setupLittleFSStorage() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== LittleFS Database Driver Example ===");
  
  // Create and configure LittleFS driver
  auto littleFSDriver = std::unique_ptr<IDatabaseDriver>(new LittleFSDatabaseDriver("/app_data"));
  StorageManager::configureDriver("littlefs", std::move(littleFSDriver));
  
  // Set LittleFS as default driver
  StorageManager::setDefaultDriver("littlefs");
  
  Serial.println("LittleFS driver configured as default");
}

void demonstrateBasicOperations() {
  Serial.println("\n--- Basic Storage Operations ---");
  
  // Store some user data
  DynamicJsonDocument userDoc(512);
  userDoc["id"] = "user123";
  userDoc["username"] = "john_doe";
  userDoc["email"] = "john@example.com";
  userDoc["created"] = "2024-01-01T00:00:00Z";
  
  String userData;
  serializeJson(userDoc, userData);
  
  // Store using default driver (LittleFS)
  bool stored = StorageManager::query("users")->store("user123", userData);
  Serial.printf("Stored user data: %s\n", stored ? "SUCCESS" : "FAILED");
  
  // Retrieve the data
  String retrievedData = StorageManager::query("users")->get("user123");
  Serial.printf("Retrieved data: %s\n", retrievedData.c_str());
  
  // Check if key exists
  bool exists = StorageManager::query("users")->exists("user123");
  Serial.printf("Key exists: %s\n", exists ? "YES" : "NO");
}

void demonstrateCollectionOperations() {
  Serial.println("\n--- Collection Operations ---");
  
  // Store multiple configuration items
  StorageManager::query("config")->store("app_name", "\"TickerTape Device\"");
  StorageManager::query("config")->store("version", "\"1.2.3\"");
  StorageManager::query("config")->store("debug_enabled", "true");
  StorageManager::query("config")->store("max_connections", "10");
  
  // List all keys in config collection
  std::vector<String> configKeys = StorageManager::query("config")->keys();
  Serial.printf("Config keys (%d total):\n", configKeys.size());
  for (const String& key : configKeys) {
    String value = StorageManager::query("config")->get(key);
    Serial.printf("  %s = %s\n", key.c_str(), value.c_str());
  }
}

void demonstrateLittleFSSpecificFeatures() {
  Serial.println("\n--- LittleFS-Specific Features ---");
  
  // Get direct access to LittleFS driver for advanced features
  IDatabaseDriver* driver = StorageManager::driver("littlefs");
  LittleFSDatabaseDriver* littleFSDriver = static_cast<LittleFSDatabaseDriver*>(driver);
  
  if (littleFSDriver) {
    // Show filesystem statistics
    String stats = littleFSDriver->getFilesystemStats();
    Serial.printf("Filesystem stats: %s\n", stats.c_str());
    
    // Show collection sizes
    size_t usersSize = littleFSDriver->getCollectionSize("users");
    size_t configSize = littleFSDriver->getCollectionSize("config");
    Serial.printf("Users collection size: %u bytes\n", usersSize);
    Serial.printf("Config collection size: %u bytes\n", configSize);
    
    // List all collections
    std::vector<String> collections = littleFSDriver->listCollections();
    Serial.printf("Collections (%d total):\n", collections.size());
    for (const String& collection : collections) {
      Serial.printf("  - %s\n", collection.c_str());
    }
  }
}

void demonstrateMultipleDrivers() {
  Serial.println("\n--- Multiple Driver Support ---");
  
  // We can use both JSON (Preferences) and LittleFS drivers simultaneously
  // JSON driver is good for small, frequently accessed data
  // LittleFS driver is good for larger data or when you need filesystem features
  
  // Store session data in JSON driver (fast access)
  StorageManager::driver("json")->store("sessions", "current_user", "user123");
  
  // Store large user profiles in LittleFS driver
  DynamicJsonDocument profileDoc(2048);
  profileDoc["user_id"] = "user123";
  profileDoc["profile_image"] = "base64encodedimagedatahere...";
  profileDoc["preferences"]["theme"] = "dark";
  profileDoc["preferences"]["language"] = "en";
  profileDoc["activity_log"] = JsonArray(); // Could be large
  
  String profileData;
  serializeJson(profileDoc, profileData);
  StorageManager::driver("littlefs")->store("profiles", "user123", profileData);
  
  Serial.println("Data distributed across multiple storage drivers");
  
  // Show driver info
  std::vector<String> drivers = StorageManager::getDriverNames();
  Serial.printf("Available drivers (%d total):\n", drivers.size());
  for (const String& driverName : drivers) {
    IDatabaseDriver* driver = StorageManager::driver(driverName);
    if (driver) {
      Serial.printf("  - %s\n", driver->getDriverName().c_str());
    }
  }
}

void runExample() {
  setupLittleFSStorage();
  demonstrateBasicOperations();
  demonstrateCollectionOperations();
  demonstrateLittleFSSpecificFeatures();
  demonstrateMultipleDrivers();
  
  Serial.println("\n=== Example Complete ===");
  Serial.println("Check your ESP32's LittleFS filesystem - data is persisted!");
  Serial.println("Files are stored in: /app_data/collection_name/key_name.json");
}

// For Arduino IDE compatibility
void setup() {
  runExample();
}

void loop() {
  delay(10000);
  Serial.println("Storage system running... (data persists across reboots)");
}