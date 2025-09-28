#include "storage/storage_manager.h"
#include "storage/json_database_driver.h"
#include "storage/littlefs_database_driver.h"
#include "storage/query_builder.h"
#include "utilities/debug_macros.h"
#include <ArduinoJson.h>

// Initialize static members
std::map<String, std::unique_ptr<IDatabaseDriver>> StorageManager::drivers;
String StorageManager::defaultDriverName = "json";
bool StorageManager::initialized = false;

void StorageManager::ensureInitialized() {
  if (!initialized) {
    // Create default JSON driver if it doesn't exist
    if (drivers.find("json") == drivers.end()) {
      drivers["json"] =
          std::unique_ptr<IDatabaseDriver>(new JsonDatabaseDriver());
    }

    // Create LittleFS driver if it doesn't exist
    if (drivers.find("littlefs") == drivers.end()) {
      drivers["littlefs"] = std::unique_ptr<IDatabaseDriver>(
          new LittleFSDatabaseDriver("/openapi_storage"));
    }

    initialized = true;
  }
}

void StorageManager::configureDriver(const String &name,
                                     std::unique_ptr<IDatabaseDriver> driver) {
  if (name.length() == 0 || !driver) {
    return;
  }

  ensureInitialized();
  drivers[name] = std::move(driver);

  DEBUG_PRINTF("StorageManager: Configured driver '%s'\n", name.c_str());
}

bool StorageManager::setDefaultDriver(const String &name) {
  ensureInitialized();

  if (drivers.find(name) != drivers.end()) {
    defaultDriverName = name;
    DEBUG_PRINTF("StorageManager: Set default driver to '%s'\n", name.c_str());
    return true;
  }

  WARN_PRINTF(
      "StorageManager: Warning - driver '%s' not found, keeping default '%s'\n",
      name.c_str(), defaultDriverName.c_str());
  return false;
}

IDatabaseDriver *StorageManager::driver(const String &name) {
  ensureInitialized();

  String targetName = name.length() > 0 ? name : defaultDriverName;

  auto it = drivers.find(targetName);
  if (it != drivers.end()) {
    return it->second.get();
  }

  WARN_PRINTF("StorageManager: Warning - driver '%s' not found\n",
              targetName.c_str());
  return nullptr;
}

std::vector<String> StorageManager::getDriverNames() {
  ensureInitialized();

  std::vector<String> names;
  for (const auto &pair : drivers) {
    names.push_back(pair.first);
  }
  return names;
}

String StorageManager::getDefaultDriverName() {
  ensureInitialized();
  return defaultDriverName;
}

bool StorageManager::removeDriver(const String &name) {
  ensureInitialized();

  if (name == "json") {
    DEBUG_PRINTLN("StorageManager: Cannot remove default JSON driver");
    return false;
  }

  auto it = drivers.find(name);
  if (it != drivers.end()) {
    drivers.erase(it);

    // If we removed the default driver, fall back to JSON
    if (defaultDriverName == name) {
      defaultDriverName = "json";
      DEBUG_PRINTF(
          "StorageManager: Removed default driver, falling back to 'json'\n");
    }

    return true;
  }

  return false;
}

void StorageManager::clearAllDrivers() {
  drivers.clear();
  defaultDriverName = "json";
  initialized = false;

  DEBUG_PRINTLN("StorageManager: Cleared all drivers");
}
