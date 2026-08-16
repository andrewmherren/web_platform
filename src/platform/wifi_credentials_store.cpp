#include "platform/wifi_credentials_store.h"

#include "utilities/debug_macros.h"

// ArduinoFake already ships its own Fakeit-mockable EEPROMClass/EEPROM
// (always linked into test_native, whether used or not), but it only
// covers the base AVR-style read/write/update/length API - no begin()/
// commit(), which are ESP32-only flash-emulation extensions this file
// needs. Reusing the name EEPROM natively would conflict with ArduinoFake's
// own global at link time, so native builds get a distinctly-named fake
// instead (see test/native/include/native_wifi_cred_eeprom.h).
#ifdef ESP_PLATFORM
#include <EEPROM.h>
#define WIFI_CRED_EEPROM EEPROM
#else
#include <native_wifi_cred_eeprom.h>
#define WIFI_CRED_EEPROM NativeWifiCredEeprom
#endif

namespace WiFiCredentialsStore {

void begin() {
  WIFI_CRED_EEPROM.begin(EEPROM_SIZE);
  DEBUG_PRINTF("WebPlatform: EEPROM initialized with size %d bytes\n",
               EEPROM_SIZE);

  // Debug: Check the flag status
  uint8_t configFlag = WIFI_CRED_EEPROM.read(CONFIG_FLAG_ADDR);
  DEBUG_PRINTF("WebPlatform: EEPROM config flag status: %d (1=configured, "
               "0=unconfigured)\n",
               configFlag);
}

bool load(String &ssid, String &password) {
  // Check if credentials exist
  if (WIFI_CRED_EEPROM.read(CONFIG_FLAG_ADDR) != 1) {
    DEBUG_PRINTLN("WebPlatform: No WiFi credentials found in EEPROM");
    return false;
  }

  // Read SSID
  ssid = "";
  for (int i = 0; i < 32; i++) {
    char c = WIFI_CRED_EEPROM.read(SSID_ADDR + i);
    if (c == 0)
      break;
    ssid += c;
  }

  // Read password
  password = "";
  for (int i = 0; i < 64; i++) {
    char c = WIFI_CRED_EEPROM.read(PASS_ADDR + i);
    if (c == 0)
      break;
    password += c;
  }

  if (ssid.length() > 0) {
    DEBUG_PRINTF("WebPlatform: Loaded stored WiFi credentials - SSID: %s, "
                 "Password length: %d chars\n",
                 ssid.c_str(), password.length());
    return true;
  } else {
    DEBUG_PRINTLN("WebPlatform: Invalid or empty WiFi credentials in EEPROM");
    return false;
  }
}

bool save(const String &ssid, const String &password) {
  // Clear previous data
  for (int i = 0; i < 96; i++) {
    WIFI_CRED_EEPROM.write(SSID_ADDR + i, 0);
  }
  // Write SSID
  for (unsigned int i = 0; i < ssid.length() && i < 31; i++) {
    WIFI_CRED_EEPROM.write(SSID_ADDR + i, ssid[i]);
  }

  // Write password
  for (unsigned int i = 0; i < password.length() && i < 63; i++) {
    WIFI_CRED_EEPROM.write(PASS_ADDR + i, password[i]);
  }

  // Set configuration flag - do this last in case of failure midway
  WIFI_CRED_EEPROM.write(CONFIG_FLAG_ADDR, 1);
  bool success = WIFI_CRED_EEPROM.commit();

  // Double-check that the write was successful
  uint8_t flagCheck = WIFI_CRED_EEPROM.read(CONFIG_FLAG_ADDR);
  if (flagCheck != 1) {
    ERROR_PRINTLN(
        "WebPlatform: ERROR - Failed to write WiFi config flag to EEPROM!");
    success = false;
  }

  DEBUG_PRINTF("WebPlatform: WiFi credentials saved for SSID: %s, Password "
               "length: %d chars, EEPROM commit %s\n",
               ssid.c_str(), password.length(),
               success ? "successful" : "failed");
  return success;
}

void reset() {
  WIFI_CRED_EEPROM.write(CONFIG_FLAG_ADDR, 0);
  WIFI_CRED_EEPROM.commit();
  DEBUG_PRINTLN("WebPlatform: WiFi credentials reset");
}

} // namespace WiFiCredentialsStore
