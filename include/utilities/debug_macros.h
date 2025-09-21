#ifndef WEB_PLATFORM_DEBUG_MACROS_H
#define WEB_PLATFORM_DEBUG_MACROS_H

/**
 * Debug Macros for WebPlatform
 * 
 * Provides compile-time switchable debug output to reduce heap usage and fragmentation
 * on ESP32/ESP8266 devices. When DEBUG_ENABLED is 0, all debug calls are completely
 * removed from compiled code, consuming zero resources.
 * 
 * Usage:
 *   DEBUG_PRINTLN("Starting WiFi connection...");
 *   DEBUG_PRINTF("Connected to %s with IP %s\n", ssid.c_str(), WiFi.localIP().toString().c_str());
 *   DEBUG_PRINT("Raw data: "); DEBUG_PRINTLN(data);
 * 
 * To disable all debug output, set DEBUG_ENABLED to 0 and recompile.
 */

// Debug can be enabled via PlatformIO build flag: -DWEB_PLATFORM_DEBUG
// Or by defining WEB_PLATFORM_DEBUG before including this header
#ifndef WEB_PLATFORM_DEBUG
#define WEB_PLATFORM_DEBUG 0  // Default to disabled for production builds
#endif

#define DEBUG_ENABLED WEB_PLATFORM_DEBUG

#if DEBUG_ENABLED
  /**
   * Initialize Serial for debug output
   * @param baud Baud rate for Serial communication
   */
  #define DEBUG_BEGIN(baud) Serial.begin(baud)
  
  /**
   * Print debug message without newline
   * @param x Message to print (String, const char*, or any printable type)
   */
  #define DEBUG_PRINT(x) Serial.print(x)
  
  /**
   * Print debug message with newline
   * @param x Message to print (String, const char*, or any printable type)
   */
  #define DEBUG_PRINTLN(x) Serial.println(x)
  
  /**
   * Print formatted debug message (printf style)
   * @param fmt Format string
   * @param ... Variable arguments for formatting
   */
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
  
#else
  // When DEBUG_ENABLED is 0, all debug macros compile to nothing
  #define DEBUG_BEGIN(baud)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x) 
  #define DEBUG_PRINTF(fmt, ...)
#endif

/**
 * Always-enabled critical error reporting (not affected by DEBUG_ENABLED)
 * Use these for errors that should always be reported regardless of debug mode
 */
#define ERROR_PRINT(x) Serial.print("[ERROR] "); Serial.print(x)
#define ERROR_PRINTLN(x) Serial.print("[ERROR] "); Serial.println(x)
#define ERROR_PRINTF(fmt, ...) Serial.printf("[ERROR] " fmt, ##__VA_ARGS__)

/**
 * Always-enabled warning reporting (not affected by DEBUG_ENABLED)
 * Use these for warnings that should always be reported regardless of debug mode
 */
#define WARN_PRINT(x) Serial.print("[WARN] "); Serial.print(x)
#define WARN_PRINTLN(x) Serial.print("[WARN] "); Serial.println(x)
#define WARN_PRINTF(fmt, ...) Serial.printf("[WARN] " fmt, ##__VA_ARGS__)

#endif // WEB_PLATFORM_DEBUG_MACROS_H