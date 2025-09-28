#include "storage/littlefs_database_driver.h"
#include "FS.h"
#include "utilities/debug_macros.h"
#include <ArduinoJson.h>

LittleFSDatabaseDriver::LittleFSDatabaseDriver(const String &baseStoragePath)
    : driverName("littlefs"), initialized(false), basePath(baseStoragePath) {
  // Ensure base path starts and ends correctly
  if (!basePath.startsWith("/")) {
    basePath = "/" + basePath;
  }
  if (basePath.endsWith("/")) {
    basePath = basePath.substring(0, basePath.length() - 1);
  }
}

LittleFSDatabaseDriver::~LittleFSDatabaseDriver() {
  // Cleanup if needed
}

void LittleFSDatabaseDriver::ensureInitialized() {
  if (!initialized) {
    DEBUG_PRINTLN("LittleFSDatabaseDriver: Initializing LittleFS...");
    if (!LittleFS.begin(true)) { // true = format if mount fails
      DEBUG_PRINTLN("LittleFSDatabaseDriver: Failed to initialize LittleFS");
      return;
    }
    DEBUG_PRINTF("LittleFSDatabaseDriver: LittleFS mounted successfully "
                 "(Total: %d bytes, Used: %d bytes)\n",
                 LittleFS.totalBytes(), LittleFS.usedBytes());

    // Ensure base storage directory exists
    if (!LittleFS.exists(basePath)) {
      if (!LittleFS.mkdir(basePath)) {
        DEBUG_PRINTF(
            "LittleFSDatabaseDriver: Failed to create base directory: %s\n",
            basePath.c_str());
        return;
      }
    }

    initialized = true;
    DEBUG_PRINTF("LittleFSDatabaseDriver: Initialized with base path: %s\n",
                 basePath.c_str());
  }
}

String LittleFSDatabaseDriver::getFilePath(const String &collection,
                                           const String &key) {
  return basePath + "/" + collection + "/" + key + ".json";
}

String LittleFSDatabaseDriver::getCollectionPath(const String &collection) {
  return basePath + "/" + collection;
}

bool LittleFSDatabaseDriver::ensureCollectionDirectory(
    const String &collection) {
  ensureInitialized();

  if (!isValidName(collection)) {
    return false;
  }

  String collectionPath = getCollectionPath(collection);

  if (!LittleFS.exists(collectionPath)) {
    if (!LittleFS.mkdir(collectionPath)) {
      DEBUG_PRINTF(
          "LittleFSDatabaseDriver: Failed to create collection directory: %s\n",
          collectionPath.c_str());
      return false;
    }
  }

  return true;
}

bool LittleFSDatabaseDriver::isValidName(const String &name) {
  if (name.length() == 0 || name.length() > 64) {
    return false;
  }

  // Check for invalid characters
  const char *invalid = "<>:\"|?*";
  for (size_t i = 0; i < strlen(invalid); i++) {
    if (name.indexOf(invalid[i]) >= 0) {
      return false;
    }
  }

  // Check for reserved names
  if (name == "." || name == ".." || name.startsWith(".")) {
    return false;
  }

  return true;
}

void LittleFSDatabaseDriver::addToCache(const String &path,
                                        const String &content) {
  // Only cache small files
  if (content.length() > MAX_CACHED_FILE_SIZE) {
    return;
  }

  // Remove if already exists (to update LRU order)
  removeFromCache(path);

  // Evict if cache is full
  while (cache.size() >= MAX_CACHE_SIZE) {
    evictLRU();
  }

  // Add to cache
  cache[path] = content;
  cacheOrder.push_back(path);
}

String LittleFSDatabaseDriver::getFromCache(const String &path) {
  auto it = cache.find(path);
  if (it != cache.end()) {
    // Move to end of LRU order (most recently used)
    auto orderIt = std::find(cacheOrder.begin(), cacheOrder.end(), path);
    if (orderIt != cacheOrder.end()) {
      cacheOrder.erase(orderIt);
      cacheOrder.push_back(path);
    }
    return it->second;
  }
  return String();
}

void LittleFSDatabaseDriver::removeFromCache(const String &path) {
  auto it = cache.find(path);
  if (it != cache.end()) {
    cache.erase(it);

    auto orderIt = std::find(cacheOrder.begin(), cacheOrder.end(), path);
    if (orderIt != cacheOrder.end()) {
      cacheOrder.erase(orderIt);
    }
  }
}

void LittleFSDatabaseDriver::evictLRU() {
  if (!cacheOrder.empty()) {
    String oldestPath = cacheOrder.front();
    removeFromCache(oldestPath);
  }
}

String LittleFSDatabaseDriver::retrieveLargeFile(File &file, size_t fileSize,
                                                 const String &filePath) {
  String content;
  content.reserve(fileSize + 1); // Pre-allocate capacity

  const size_t CHUNK_SIZE = 1024; // Smaller chunks for large files
  char buffer[CHUNK_SIZE + 1];
  size_t totalRead = 0;

  DEBUG_PRINTF(
      "LittleFSDatabaseDriver: Reading large file %s (%d bytes) in chunks\n",
      filePath.c_str(), fileSize);

  while (file.available() && totalRead < fileSize) {
    size_t toRead = std::min((size_t)file.available(), CHUNK_SIZE);
    size_t bytesRead = file.readBytes(buffer, toRead);

    if (bytesRead == 0) {
      DEBUG_PRINTF(
          "LittleFSDatabaseDriver: Read returned 0 bytes at position %d\n",
          totalRead);
      break;
    }

    buffer[bytesRead] = '\0'; // Ensure null termination

    // Use concat with explicit length to avoid string truncation issues
    content.concat(buffer, bytesRead);

    totalRead += bytesRead;

    // Debug: Check if content length matches totalRead
    if (content.length() != totalRead) {
      DEBUG_PRINTF("LittleFSDatabaseDriver: WARNING - String length mismatch: "
                   "content=%d, totalRead=%d\n",
                   content.length(), totalRead);
    }

    // Yield periodically for large files to prevent watchdog issues
    if (totalRead % (CHUNK_SIZE * 16) == 0) {
      yield();
      DEBUG_PRINTF("LittleFSDatabaseDriver: Progress: %d/%d bytes (%.1f%%), "
                   "content length: %d\n",
                   totalRead, fileSize, (totalRead * 100.0) / fileSize,
                   content.length());
    }
  }

  DEBUG_PRINTF("LittleFSDatabaseDriver: File read complete - totalRead: %d, "
               "content length: %d, expected: %d\n",
               totalRead, content.length(), fileSize);

  // Check both totalRead and actual content length
  if (totalRead != fileSize || content.length() != fileSize) {
    DEBUG_PRINTF("LittleFSDatabaseDriver: Large file read incomplete or "
                 "corrupted: totalRead=%d, content=%d, expected=%d\n",
                 totalRead, content.length(), fileSize);
    return String(); // Return empty string on failure
  }

  DEBUG_PRINTF(
      "LittleFSDatabaseDriver: Successfully read large file: %d bytes\n",
      content.length());
  return content;
}

bool LittleFSDatabaseDriver::store(const String &collection, const String &key,
                                   const String &data) {
  if (!isValidName(collection) || !isValidName(key)) {
    DEBUG_PRINTLN("LittleFSDatabaseDriver: Invalid collection or key name");
    return false;
  }

  if (!ensureCollectionDirectory(collection)) {
    return false;
  }

  String filePath = getFilePath(collection, key);

  File file = LittleFS.open(filePath, FILE_WRITE);
  if (!file) {
    DEBUG_PRINTF(
        "LittleFSDatabaseDriver: Failed to open file for writing: %s\n",
        filePath.c_str());
    return false;
  }

  size_t written = file.print(data);
  file.close();

  if (written == data.length()) {
    // Add to cache if small enough
    addToCache(filePath, data);
    DEBUG_PRINTF("LittleFSDatabaseDriver: Stored %s/%s (%u bytes)\n",
                 collection.c_str(), key.c_str(), data.length());
    return true;
  } else {
    DEBUG_PRINTF("LittleFSDatabaseDriver: Write failed for %s/%s\n",
                 collection.c_str(), key.c_str());
    return false;
  }
}

String LittleFSDatabaseDriver::retrieve(const String &collection,
                                        const String &key) {
  if (!isValidName(collection) || !isValidName(key)) {
    return String();
  }

  ensureInitialized();

  String filePath = getFilePath(collection, key);

  // Check cache first
  String cached = getFromCache(filePath);
  if (cached.length() > 0) {
    return cached;
  }

  if (!LittleFS.exists(filePath)) {
    return String();
  }

  File file = LittleFS.open(filePath, FILE_READ);
  if (!file) {
    DEBUG_PRINTF(
        "LittleFSDatabaseDriver: Failed to open file for reading: %s\n",
        filePath.c_str());
    return String();
  }

  size_t fileSize = file.size();

  // Use different strategies for small vs large files
  String content;

  if (fileSize <= 16384) { // 16KB - use simple read
    content = file.readString();
    file.close();

    if (content.length() != fileSize) {
      DEBUG_PRINTF("LittleFSDatabaseDriver: Simple read size mismatch for %s: "
                   "expected %d, got %d\n",
                   filePath.c_str(), fileSize, content.length());
      return String();
    }
  } else { // Large file - use improved chunked reading
    content = retrieveLargeFile(file, fileSize, filePath);
    file.close();

    if (content.isEmpty()) {
      DEBUG_PRINTF(
          "LittleFSDatabaseDriver: Large file retrieval failed for %s\n",
          filePath.c_str());
      return String();
    }
  }

  // Final verification
  if (content.length() != fileSize) {
    DEBUG_PRINTF("LittleFSDatabaseDriver: CRITICAL - Final content size "
                 "mismatch for %s: expected %d bytes, got %d bytes\n",
                 filePath.c_str(), fileSize, content.length());

    // Log first and last few characters for debugging
    if (content.length() > 10) {
      DEBUG_PRINTF("LittleFSDatabaseDriver: First 50 chars: '%.50s'\n",
                   content.c_str());
      if (content.length() > 50) {
        String lastChars = content.substring(content.length() - 50);
        DEBUG_PRINTF("LittleFSDatabaseDriver: Last 50 chars: '%.50s'\n",
                     lastChars.c_str());
      }
    }

    return String(); // Return empty string to indicate failure
  }

  DEBUG_PRINTF("LittleFSDatabaseDriver: Read file %s: expected %d bytes, got "
               "%d bytes - SUCCESS\n",
               filePath.c_str(), fileSize, content.length());

  // Add to cache only if small enough
  addToCache(filePath, content);

  return content;
}

bool LittleFSDatabaseDriver::remove(const String &collection,
                                    const String &key) {
  if (!isValidName(collection) || !isValidName(key)) {
    return false;
  }

  ensureInitialized();

  String filePath = getFilePath(collection, key);

  if (!LittleFS.exists(filePath)) {
    return false;
  }

  bool removed = LittleFS.remove(filePath);
  if (removed) {
    removeFromCache(filePath);
    DEBUG_PRINTF("LittleFSDatabaseDriver: Removed %s/%s\n", collection.c_str(),
                 key.c_str());
  }

  return removed;
}

std::vector<String> LittleFSDatabaseDriver::listKeys(const String &collection) {
  std::vector<String> keys;

  if (!isValidName(collection)) {
    return keys;
  }

  ensureInitialized();

  String collectionPath = getCollectionPath(collection);

  if (!LittleFS.exists(collectionPath)) {
    return keys;
  }

  File dir = LittleFS.open(collectionPath);
  if (!dir || !dir.isDirectory()) {
    return keys;
  }

  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      // Remove .json extension
      if (filename.endsWith(".json")) {
        String key = filename.substring(0, filename.length() - 5);
        keys.push_back(key);
      }
    }
    file = dir.openNextFile();
  }

  return keys;
}

bool LittleFSDatabaseDriver::exists(const String &collection,
                                    const String &key) {
  if (!isValidName(collection) || !isValidName(key)) {
    return false;
  }

  ensureInitialized();

  String filePath = getFilePath(collection, key);
  return LittleFS.exists(filePath);
}

String LittleFSDatabaseDriver::getDriverName() const { return driverName; }

void LittleFSDatabaseDriver::clearCache() {
  cache.clear();
  cacheOrder.clear();
  DEBUG_PRINTLN("LittleFSDatabaseDriver: Cache cleared");
}

String LittleFSDatabaseDriver::getFilesystemStats() {
  ensureInitialized();

  DynamicJsonDocument doc(512);
  doc["total_bytes"] = LittleFS.totalBytes();
  doc["used_bytes"] = LittleFS.usedBytes();
  doc["free_bytes"] = LittleFS.totalBytes() - LittleFS.usedBytes();
  doc["cache_entries"] = cache.size();

  String result;
  serializeJson(doc, result);
  return result;
}

bool LittleFSDatabaseDriver::removeCollection(const String &collection) {
  if (!isValidName(collection)) {
    return false;
  }

  ensureInitialized();

  String collectionPath = getCollectionPath(collection);

  if (!LittleFS.exists(collectionPath)) {
    return true; // Already doesn't exist
  }

  // Remove all files in the collection
  std::vector<String> keys = listKeys(collection);
  for (const String &key : keys) {
    remove(collection, key);
  }

  // Remove the directory
  bool removed = LittleFS.rmdir(collectionPath);
  if (removed) {
    DEBUG_PRINTF("LittleFSDatabaseDriver: Removed collection: %s\n",
                 collection.c_str());
  }

  return removed;
}

size_t LittleFSDatabaseDriver::getKeySize(const String &collection,
                                          const String &key) {
  if (!isValidName(collection) || !isValidName(key)) {
    return 0;
  }

  ensureInitialized();

  String filePath = getFilePath(collection, key);

  if (!LittleFS.exists(filePath)) {
    return 0;
  }

  File file = LittleFS.open(filePath, FILE_READ);
  if (!file) {
    return 0;
  }

  size_t size = file.size();
  file.close();

  return size;
}

size_t LittleFSDatabaseDriver::getCollectionSize(const String &collection) {
  if (!isValidName(collection)) {
    return 0;
  }

  ensureInitialized();

  String collectionPath = getCollectionPath(collection);

  if (!LittleFS.exists(collectionPath)) {
    return 0;
  }

  size_t totalSize = 0;
  std::vector<String> keys = listKeys(collection);

  for (const String &key : keys) {
    totalSize += getKeySize(collection, key);
  }

  return totalSize;
}

std::vector<String> LittleFSDatabaseDriver::listCollections() {
  std::vector<String> collections;

  ensureInitialized();

  if (!LittleFS.exists(basePath)) {
    return collections;
  }

  File dir = LittleFS.open(basePath);
  if (!dir || !dir.isDirectory()) {
    return collections;
  }

  File file = dir.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      String name = file.name();
      // Remove the base path prefix if present
      if (name.startsWith(basePath + "/")) {
        name = name.substring(basePath.length() + 1);
      }
      collections.push_back(name);
    }
    file = dir.openNextFile();
  }

  return collections;
}

bool LittleFSDatabaseDriver::formatFilesystem() {
  DEBUG_PRINTLN("LittleFSDatabaseDriver: WARNING - Formatting filesystem (all "
                "data will be lost)");

  // Clear cache first
  clearCache();

  // End current filesystem
  LittleFS.end();

  // Format and restart
  bool formatted = LittleFS.begin(true); // true forces format

  if (formatted) {
    initialized = false; // Force re-initialization
    ensureInitialized();
    DEBUG_PRINTLN("LittleFSDatabaseDriver: Filesystem formatted successfully");
  } else {
    DEBUG_PRINTLN("LittleFSDatabaseDriver: Failed to format filesystem");
  }

  return formatted;
}