#include "native_wifi_cred_eeprom.h"

NativeWifiCredEepromClass NativeWifiCredEeprom;

namespace NativeEEPROMFake {

std::vector<uint8_t> &store() {
  static std::vector<uint8_t> s;
  return s;
}

void reset() { store().assign(store().size(), 0); }

} // namespace NativeEEPROMFake
