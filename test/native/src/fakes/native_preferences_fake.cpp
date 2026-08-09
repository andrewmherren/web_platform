#include "Preferences.h"

namespace NativePreferencesFake {

std::map<std::string, std::map<std::string, std::string>> &store() {
  static std::map<std::string, std::map<std::string, std::string>> s;
  return s;
}

void reset() { store().clear(); }

} // namespace NativePreferencesFake
