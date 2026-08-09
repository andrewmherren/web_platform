#ifndef NATIVE_FAKE_PREFERENCES_H
#define NATIVE_FAKE_PREFERENCES_H

// Minimal native-only fake of ESP32's Preferences (NVS) API, scoped to
// exactly what src/storage/json_database_driver.cpp uses: begin/end and
// get/putString. Backed by a process-wide in-memory map so state persists
// across separate `Preferences prefs;` instances within one test run, the
// same way real NVS persists across separate begin()/end() sessions on
// device. Call NativePreferencesFake::reset() between tests that need a
// clean slate - nothing resets it automatically.
//
// Only exists so json_database_driver.cpp can compile and run natively;
// not a general-purpose Preferences reimplementation.

#include <Arduino.h>
#include <map>
#include <string>

namespace NativePreferencesFake {
std::map<std::string, std::map<std::string, std::string>> &store();
void reset();
} // namespace NativePreferencesFake

class Preferences {
private:
  std::string ns_;
  bool open_ = false;

public:
  bool begin(const char *name, bool readOnly = false) {
    (void)readOnly;
    ns_ = name ? name : "";
    open_ = true;
    return true;
  }

  void end() { open_ = false; }

  String getString(const char *key, const char *defaultValue = "") {
    if (!open_ || !key) {
      return String(defaultValue);
    }
    auto &ns = NativePreferencesFake::store()[ns_];
    auto it = ns.find(key);
    return it == ns.end() ? String(defaultValue) : String(it->second.c_str());
  }

  size_t putString(const char *key, const String &value) {
    if (!open_ || !key) {
      return 0;
    }
    NativePreferencesFake::store()[ns_][key] = value.c_str();
    return value.length();
  }
};

#endif // NATIVE_FAKE_PREFERENCES_H
