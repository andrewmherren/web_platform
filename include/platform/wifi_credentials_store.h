#ifndef WIFI_CREDENTIALS_STORE_H
#define WIFI_CREDENTIALS_STORE_H

#include <interface/string_compat.h>
#include <testing/arduino_string_compat.h>

// EEPROM-backed WiFi credential storage - moved out of WebPlatform
// (src/platform/web_platform_wifi.cpp) as the low-risk half of Phase 2c's
// god-class breakup (see PROJECT_PLAN.md). Cleanly self-contained: only
// ever touches its own four EEPROM addresses, never any other WebPlatform
// member state, so it lifted out with no behavior change. The
// server/certificate machinery flagged in the same investigation stays a
// separate, more invasive decision - not attempted here.
namespace WiFiCredentialsStore {

constexpr int EEPROM_SIZE = 512;
constexpr int SSID_ADDR = 0;
constexpr int PASS_ADDR = 64;
constexpr int CONFIG_FLAG_ADDR = 128;

// Must be called once before any other function (mirrors EEPROM.begin()).
void begin();

// Reads stored credentials into ssid/password. Returns false (leaving both
// untouched beyond being cleared) if no credentials are stored or the
// stored SSID is empty.
bool load(String &ssid, String &password);

// Writes ssid/password and sets the "configured" flag. Returns false if the
// flag write couldn't be verified after commit.
bool save(const String &ssid, const String &password);

// Clears the "configured" flag so a subsequent load() reports no stored
// credentials. Does not erase the SSID/password bytes themselves, matching
// prior behavior - only the flag ever gated whether they're trusted.
void reset();

} // namespace WiFiCredentialsStore

#endif // WIFI_CREDENTIALS_STORE_H
