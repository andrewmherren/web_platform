#include "platform/ntp_client.h"
#include "utilities/debug_macros.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <time.h>


// Static member definitions
const char *NTPClient::_ntpServer = "pool.ntp.org";
unsigned long NTPClient::_updateInterval = 3600000; // 1 hour
unsigned long NTPClient::_lastUpdate = 0;
unsigned long NTPClient::_lastSyncTime = 0;
bool NTPClient::_synchronized = false;
bool NTPClient::_initialized = false;

void NTPClient::begin(const char *server, unsigned long updateInterval) {
  _ntpServer = server;
  _updateInterval = updateInterval;
  _initialized = true;

  DEBUG_PRINTLN("[NTP] Initializing NTP client (UTC mode)...");
  DEBUG_PRINT("[NTP] Server: ");
  DEBUG_PRINTLN(_ntpServer);

  // Configure NTP in UTC mode
  configTime(0, 0, _ntpServer);

  // Try initial sync
  forceSync();
}

void NTPClient::handle() {
  if (!_initialized || !WiFi.isConnected()) {
    return;
  }

  unsigned long now = millis();

  // Check if it's time for an update
  if (_lastUpdate == 0 || (now - _lastUpdate) >= _updateInterval) {
    if (syncTimeFromNTP()) {
      _lastUpdate = now;
    }
  }
}

bool NTPClient::forceSync() {
  if (!_initialized || !WiFi.isConnected()) {
    DEBUG_PRINTLN("[NTP] Cannot sync: not initialized or WiFi not connected");
    return false;
  }

  DEBUG_PRINTLN("[NTP] Forcing time synchronization...");
  return syncTimeFromNTP();
}

bool NTPClient::syncTimeFromNTP() {
  // Wait for time sync (up to 10 seconds)
  int attempts = 0;
  const int maxAttempts = 20; // 10 seconds with 500ms delays

  time_t now = time(nullptr);
  while (now < 100000 && attempts < maxAttempts) { // Basic sanity check
    delay(500);
    now = time(nullptr);
    attempts++;
    DEBUG_PRINT(".");
  }

  if (now > 100000) { // Successfully got time
    _synchronized = true;
    _lastSyncTime = millis();

    DEBUG_PRINTLN();
    DEBUG_PRINT("[NTP] Time synchronized: ");
    DEBUG_PRINTLN(getFormattedTime());

    return true;
  } else {
    WARN_PRINTLN();
    WARN_PRINTLN("[NTP] Failed to synchronize time");
    return false;
  }
}

bool NTPClient::isSynchronized() {
  if (!_synchronized) {
    return false;
  }

  // Check if time is still reasonable (not rolled back to 1970)
  time_t now = time(nullptr);
  return now > 100000; // Basic sanity check
}
unsigned long NTPClient::getCurrentTime() {
  if (!isSynchronized()) {
    return 0; // Return 0 if time is not synchronized
  }

  // Always return UTC time - timezone conversion handled by frontend
  return time(nullptr);
}

String NTPClient::getFormattedTime(const char *format) {
  if (!isSynchronized()) {
    return "Time not synchronized";
  }

  // Use std::chrono for modern C++ time handling
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);

  struct tm timeinfo;
  gmtime_r(&time_t_now, &timeinfo);

  // Use stringstream with std::put_time for formatting
  std::stringstream ss;
  ss << std::put_time(&timeinfo, format);
  return String(ss.str().c_str());
}

unsigned long NTPClient::getTimeSinceLastSync() {
  if (_lastSyncTime == 0) {
    return 0;
  }
  return millis() - _lastSyncTime;
}

void NTPClient::setNTPServer(const char *server) {
  _ntpServer = server;
  if (_initialized) {
    // Reconfigure NTP with new server (always in UTC)
    configTime(0, 0, _ntpServer);
  }
}

void NTPClient::setUpdateInterval(unsigned long interval) {
  _updateInterval = interval;
}
