#include "../../include/platform/ntp_client.h"

#ifdef ESP32
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#endif

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

  Serial.println("[NTP] Initializing NTP client (UTC mode)...");
  Serial.print("[NTP] Server: ");
  Serial.println(_ntpServer);

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
    Serial.println("[NTP] Cannot sync: not initialized or WiFi not connected");
    return false;
  }

  Serial.println("[NTP] Forcing time synchronization...");
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
    Serial.print(".");
  }

  if (now > 100000) { // Successfully got time
    _synchronized = true;
    _lastSyncTime = millis();

    Serial.println();
    Serial.print("[NTP] Time synchronized: ");
    Serial.println(getFormattedTime());

    return true;
  } else {
    Serial.println();
    Serial.println("[NTP] Failed to synchronize time");
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

  time_t now = time(nullptr);
  struct tm *timeinfo = gmtime(&now);

  char buffer[64];
  strftime(buffer, sizeof(buffer), format, timeinfo);
  return String(buffer);
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
