#pragma once

#include <Arduino.h>

/**
 * @brief NTP Client for synchronizing system time (UTC only)
 *
 * Provides automatic UTC time synchronization from NTP servers.
 * All timestamps are in UTC - timezone conversion is handled by frontend
 * JavaScript. This simplifies the embedded device and provides better
 * reliability.
 */
class NTPClient {
public:
  /**
   * @brief Initialize NTP client in UTC mode
   * @param server Primary NTP server (default: pool.ntp.org)
   * @param updateInterval Update interval in milliseconds (default: 1 hour)
   */
  static void begin(const char *server = "pool.ntp.org",
                    unsigned long updateInterval = 3600000);

  /**
   * @brief Handle NTP updates (call in main loop)
   */
  static void handle();

  /**
   * @brief Force immediate time sync
   * @return true if sync was successful
   */
  static bool forceSync();

  /**
   * @brief Check if time has been synchronized
   * @return true if time is synchronized
   */
  static bool isSynchronized();

  /**
   * @brief Get current Unix timestamp
   * @return Unix timestamp (seconds since epoch)
   */
  static unsigned long getCurrentTime();

  /**
   * @brief Get formatted time string
   * @param format strftime format string (default: ISO 8601)
   * @return Formatted time string
   */
  static String getFormattedTime(const char *format = "%Y-%m-%dT%H:%M:%SZ");

  /**
   * @brief Get time since last successful sync
   * @return Milliseconds since last sync (0 if never synced)
   */
  static unsigned long getTimeSinceLastSync();

  /**
   * @brief Set NTP server
   * @param server NTP server hostname or IP
   */
  static void setNTPServer(const char *server);

  /**
   * @brief Set update interval
   * @param interval Update interval in milliseconds
   */
  static void setUpdateInterval(unsigned long interval);

private:
  static const char *_ntpServer;
  static unsigned long _updateInterval;
  static unsigned long _lastUpdate;
  static unsigned long _lastSyncTime;
  static bool _synchronized;
  static bool _initialized;

  static bool syncTimeFromNTP();
};