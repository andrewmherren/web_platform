#ifndef AUTH_CONSTANTS_H
#define AUTH_CONSTANTS_H

#include <Arduino.h>

namespace AuthConstants {
// Session duration (24 hours in milliseconds)
constexpr unsigned long SESSION_DURATION_MS = 24 * 60 * 60 * 1000;

// Page token duration (30 minutes in milliseconds)
constexpr unsigned long PAGE_TOKEN_DURATION_MS = 30 * 60 * 1000;

// Default admin credentials
const char *const DEFAULT_ADMIN_USERNAME = "admin";
const char *const DEFAULT_ADMIN_PASSWORD =
    ""; // Empty - prompts for initial setup
} // namespace AuthConstants

#endif // AUTH_CONSTANTS_H