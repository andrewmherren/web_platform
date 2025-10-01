#ifndef REDIRECT_TYPES_H
#define REDIRECT_TYPES_H

#include <Arduino.h>

// Redirect structure for managing URL redirects (simplified for embedded use)
struct RedirectRule {
  const char *fromPath; // Source path to redirect from (PROGMEM pointer)
  const char *toPath;   // Destination path to redirect to (PROGMEM pointer)

  // Constructor for const char* (PROGMEM-friendly)
  RedirectRule(const char *from, const char *to) : fromPath(from), toPath(to) {}

  // Legacy String constructor (for backward compatibility)
  RedirectRule(const String &from, const String &to);
};

#endif // REDIRECT_TYPES_H