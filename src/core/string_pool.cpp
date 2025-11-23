#include "core/string_pool.h"
#include <algorithm>
#include <vector>
#include <memory>

namespace WebPlatform {
namespace Core {

// Internal implementation using PIMPL pattern to hide std::vector from header
struct StringPool::Impl {
  std::vector<std::string> strings;
  bool sealed = false;
  size_t reservedCapacity = 64; // Default capacity for route paths

  Impl() { strings.reserve(reservedCapacity); }

  const char *storeString(const std::string &str) {
    if (str.empty()) {
      return nullptr;
    }

    if (sealed) {
      // In a real implementation, we'd log an error here
      // For now, just return nullptr to indicate failure
      return nullptr;
    }

    // Check if we already have this string to avoid duplicates
    auto it = std::find(strings.begin(), strings.end(), str);
    if (it != strings.end()) {
      return it->c_str();
    }

    // Safety check - never exceed actual capacity
    if (strings.size() >= strings.capacity()) {
      // Capacity exceeded - return nullptr to indicate failure
      return nullptr;
    }

    // Store the new string
    strings.push_back(str);
    return strings.back().c_str();
  }

  size_t getSize() const { return strings.size(); }

  size_t getMemoryUsage() const {
    size_t total = 0;
    for (const auto &str : strings) {
      total += str.length() + 1; // +1 for null terminator
    }
    return total;
  }

  void clearStrings() {
    if (!sealed) {
      strings.clear();
    }
  }

  void sealPool() { sealed = true; }

  bool isPoolSealed() const { return sealed; }

  size_t getCapacity() const { return strings.capacity(); }

  void setReserve(size_t cap) {
    reservedCapacity = cap;
    if (!sealed) {
      // If requesting smaller capacity than current, we need to shrink
      if (cap < strings.capacity()) {
        // Create new vector with exact capacity and move existing strings
        std::vector<std::string> newStrings;
        newStrings.reserve(cap);
        for (auto &str : strings) {
          newStrings.push_back(std::move(str));
        }
        strings = std::move(newStrings);
      } else {
        strings.reserve(cap);
      }
    }
  }
};

void StringPool::ensureInitialized() {
  if (!impl) {
    impl = std::make_unique<Impl>();
  }
}

const char *StringPool::store(const std::string &str) {
  ensureInitialized();
  return impl->storeString(str);
}

const char *StringPool::store(const char *str) {
  if (!str || *str == '\0') {
    return nullptr;
  }
  ensureInitialized();
  return impl->storeString(std::string(str));
}

const char *StringPool::empty() const { return nullptr; }

void StringPool::seal() {
  ensureInitialized();
  impl->sealPool();
}

bool StringPool::isSealed() const {
  if (!impl) {
    return false;
  }
  return impl->isPoolSealed();
}

void StringPool::clear() {
  if (impl) {
    impl->clearStrings();
  }
}

size_t StringPool::size() const {
  if (!impl) {
    return 0;
  }
  return impl->getSize();
}

size_t StringPool::memoryUsage() const {
  if (!impl) {
    return 0;
  }
  return impl->getMemoryUsage();
}

size_t StringPool::capacity() const {
  if (!impl) {
    return 64; // Return default capacity if not yet initialized
  }
  return impl->getCapacity();
}

void StringPool::reserve(size_t cap) {
  ensureInitialized();
  impl->setReserve(cap);
}

} // namespace Core
} // namespace WebPlatform

// Destructor implementation - must be defined where Impl is complete
WebPlatform::Core::StringPool::~StringPool() = default;
