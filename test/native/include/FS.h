#ifndef NATIVE_FAKE_FS_H
#define NATIVE_FAKE_FS_H

// Minimal native-only fake of Arduino's FS.h File class, scoped to exactly
// what src/storage/littlefs_database_driver.cpp uses. Backed by
// LittleFS.h's in-memory filesystem (see native_fs_fake.cpp) - not a
// general-purpose File reimplementation.

#include <Arduino.h>
#include <string>
#include <vector>

#define FILE_READ "r"
#define FILE_WRITE "w"

class File {
public:
  File() = default;

  explicit operator bool() const { return valid_; }

  size_t print(const String &data) {
    if (!valid_ || isDir_) {
      return 0;
    }
    content_ = std::string(data.c_str());
    return content_.size();
  }

  void close();

  size_t size() const { return content_.size(); }

  String readString() {
    String result(content_.c_str() + pos_);
    pos_ = content_.size();
    return result;
  }

  int available() const {
    return static_cast<int>(content_.size() - pos_);
  }

  size_t readBytes(char *buffer, size_t length) {
    size_t remaining = content_.size() - pos_;
    size_t toCopy = length < remaining ? length : remaining;
    for (size_t i = 0; i < toCopy; i++) {
      buffer[i] = content_[pos_ + i];
    }
    pos_ += toCopy;
    return toCopy;
  }

  bool isDirectory() const { return isDir_; }

  // Real ESP32 File::name() returns the basename, not the full path -
  // littlefs_database_driver.cpp's listKeys()/listCollections() rely on
  // that to extract bare keys from directory entries.
  String name() const {
    size_t slash = path_.find_last_of('/');
    return String((slash == std::string::npos ? path_
                                               : path_.substr(slash + 1))
                      .c_str());
  }

  File openNextFile();

  static File makeFile(const std::string &path, const std::string &content,
                       bool forWrite);
  static File makeDirectory(const std::string &path,
                            std::vector<std::string> children);

private:
  bool valid_ = false;
  bool isDir_ = false;
  bool forWrite_ = false;
  std::string path_;
  std::string content_;
  size_t pos_ = 0;
  std::vector<std::string> children_;
  size_t childIndex_ = 0;
};

#endif // NATIVE_FAKE_FS_H
