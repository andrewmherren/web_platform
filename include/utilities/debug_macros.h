#ifndef WEB_PLATFORM_DEBUG_MACROS_H
#define WEB_PLATFORM_DEBUG_MACROS_H

/**
 * Debug Macros for WebPlatform
 *
 * This file now includes the unified debug macros from web_platform_interface
 * to ensure consistent macro definitions across the entire platform.
 *
 * Usage:
 *   DEBUG_PRINTLN("Starting WiFi connection...");
 *   DEBUG_PRINTF("Connected to %s with IP %s\n", ssid.c_str(),
 * WiFi.localIP().toString().c_str()); DEBUG_PRINT("Raw data: ");
 * DEBUG_PRINTLN(data);
 */

// Include the canonical debug macros from the interface
#include <interface/debug_macros.h>

#endif // WEB_PLATFORM_DEBUG_MACROS_H