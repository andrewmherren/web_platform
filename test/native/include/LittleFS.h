#ifndef NATIVE_FAKE_LITTLEFS_H
#define NATIVE_FAKE_LITTLEFS_H

// Minimal native-only fake of ESP32's LittleFS API, scoped to exactly what
// src/storage/littlefs_database_driver.cpp uses: begin/end, exists/mkdir/
// rmdir/remove, and open() returning a File (see FS.h). Backed by a
// process-wide in-memory map of path -> content plus a set of directory
// paths - not a general-purpose filesystem. Call NativeFsFake::reset()
// between tests that need a clean slate.

#include "FS.h"
#include <Arduino.h>
#include <string>
#include <vector>

namespace NativeFsFake {
void reset();
void writeFile(const std::string &path, const std::string &content);
bool pathExists(const std::string &path);
bool isDirectory(const std::string &path);
bool makeDirectory(const std::string &path);
bool removeDirectory(const std::string &path);
bool removeFile(const std::string &path);
std::string readFile(const std::string &path);
std::vector<std::string> immediateChildren(const std::string &dirPath);
size_t totalBytes();
size_t usedBytes();
} // namespace NativeFsFake

class LittleFSClass {
public:
  bool begin(bool formatOnFail = false) {
    (void)formatOnFail;
    return true;
  }

  void end() {}

  size_t totalBytes() { return NativeFsFake::totalBytes(); }
  size_t usedBytes() { return NativeFsFake::usedBytes(); }

  bool exists(const String &path) {
    return NativeFsFake::pathExists(path.c_str());
  }

  bool mkdir(const String &path) {
    return NativeFsFake::makeDirectory(path.c_str());
  }

  bool rmdir(const String &path) {
    return NativeFsFake::removeDirectory(path.c_str());
  }

  bool remove(const String &path) {
    return NativeFsFake::removeFile(path.c_str());
  }

  File open(const String &path, const char *mode = FILE_READ);
};

extern LittleFSClass LittleFS;

#endif // NATIVE_FAKE_LITTLEFS_H
