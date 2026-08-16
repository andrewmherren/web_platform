#ifndef NATIVE_WIFI_CRED_EEPROM_H
#define NATIVE_WIFI_CRED_EEPROM_H

// Minimal native-only fake of ESP32's EEPROM API, scoped to exactly what
// src/platform/wifi_credentials_store.cpp uses: begin/read/write/commit.
// Deliberately NOT named EEPROM/EEPROMClass: ArduinoFake already ships its
// own Fakeit-mockable EEPROMClass/EEPROM (linked into every test_native
// binary regardless of whether it's used), but that fake only covers the
// base AVR-style read/write/update/length API - it has no begin()/commit(),
// which are ESP32-only flash-emulation extensions this code needs. Reusing
// the same names would conflict (duplicate global) at link time, so this
// is its own distinct type/global instead, wired in only for
// wifi_credentials_store.cpp's native build (see the #ifdef ESP_PLATFORM
// switch there).
//
// Backed by a process-wide in-memory buffer so state persists across
// separate begin() calls within one test run, the same way real EEPROM
// (backed by flash) persists across begin() calls on device. Call
// NativeEEPROMFake::reset() between tests that need a clean slate - nothing
// resets it automatically.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace NativeEEPROMFake {
std::vector<uint8_t> &store();
void reset();
} // namespace NativeEEPROMFake

class NativeWifiCredEepromClass {
public:
  bool begin(size_t size) {
    auto &s = NativeEEPROMFake::store();
    if (s.size() < size) {
      s.resize(size, 0);
    }
    return true;
  }

  uint8_t read(int address) {
    auto &s = NativeEEPROMFake::store();
    return (address >= 0 && static_cast<size_t>(address) < s.size())
               ? s[address]
               : 0;
  }

  void write(int address, uint8_t value) {
    auto &s = NativeEEPROMFake::store();
    if (address >= 0 && static_cast<size_t>(address) < s.size()) {
      s[address] = value;
    }
  }

  bool commit() { return true; }
};

extern NativeWifiCredEepromClass NativeWifiCredEeprom;

#endif // NATIVE_WIFI_CRED_EEPROM_H
