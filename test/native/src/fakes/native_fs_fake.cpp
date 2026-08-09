#include "FS.h"
#include "LittleFS.h"
#include <cstring>
#include <map>
#include <set>

LittleFSClass LittleFS;

namespace NativeFsFake {

namespace {
std::map<std::string, std::string> &files() {
  static std::map<std::string, std::string> f;
  return f;
}
std::set<std::string> &dirs() {
  static std::set<std::string> d;
  return d;
}
} // namespace

void reset() {
  files().clear();
  dirs().clear();
}

void writeFile(const std::string &path, const std::string &content) {
  files()[path] = content;
}

bool pathExists(const std::string &path) {
  return dirs().count(path) > 0 || files().count(path) > 0;
}

bool isDirectory(const std::string &path) { return dirs().count(path) > 0; }

bool makeDirectory(const std::string &path) {
  dirs().insert(path);
  return true;
}

bool removeDirectory(const std::string &path) {
  return dirs().erase(path) > 0;
}

bool removeFile(const std::string &path) { return files().erase(path) > 0; }

std::string readFile(const std::string &path) {
  auto it = files().find(path);
  return it == files().end() ? std::string() : it->second;
}

std::vector<std::string> immediateChildren(const std::string &dirPath) {
  std::vector<std::string> result;
  const std::string prefix = dirPath + "/";

  for (const auto &d : dirs()) {
    if (d.size() > prefix.size() && d.compare(0, prefix.size(), prefix) == 0 &&
        d.find('/', prefix.size()) == std::string::npos) {
      result.push_back(d);
    }
  }
  for (const auto &entry : files()) {
    const std::string &f = entry.first;
    if (f.size() > prefix.size() && f.compare(0, prefix.size(), prefix) == 0 &&
        f.find('/', prefix.size()) == std::string::npos) {
      result.push_back(f);
    }
  }
  return result;
}

size_t totalBytes() { return 1024 * 1024; }

size_t usedBytes() {
  size_t total = 0;
  for (const auto &entry : files()) {
    total += entry.second.size();
  }
  return total;
}

} // namespace NativeFsFake

void File::close() {
  if (valid_ && forWrite_ && !isDir_) {
    NativeFsFake::writeFile(path_, content_);
  }
  valid_ = false;
}

File File::openNextFile() {
  if (!valid_ || !isDir_ || childIndex_ >= children_.size()) {
    return File();
  }
  const std::string &childPath = children_[childIndex_++];
  if (NativeFsFake::isDirectory(childPath)) {
    return File::makeDirectory(childPath,
                               NativeFsFake::immediateChildren(childPath));
  }
  return File::makeFile(childPath, NativeFsFake::readFile(childPath), false);
}

File File::makeFile(const std::string &path, const std::string &content,
                    bool forWrite) {
  File f;
  f.valid_ = true;
  f.isDir_ = false;
  f.forWrite_ = forWrite;
  f.path_ = path;
  f.content_ = content;
  return f;
}

File File::makeDirectory(const std::string &path,
                         std::vector<std::string> children) {
  File f;
  f.valid_ = true;
  f.isDir_ = true;
  f.path_ = path;
  f.children_ = std::move(children);
  return f;
}

File LittleFSClass::open(const String &path, const char *mode) {
  std::string p(path.c_str());

  if (mode && std::strcmp(mode, FILE_WRITE) == 0) {
    return File::makeFile(p, "", true);
  }

  if (NativeFsFake::isDirectory(p)) {
    return File::makeDirectory(p, NativeFsFake::immediateChildren(p));
  }
  if (NativeFsFake::pathExists(p)) {
    return File::makeFile(p, NativeFsFake::readFile(p), false);
  }
  return File();
}
