#include "types/redirect_types.h"

// PROGMEM storage pool for redirect strings to avoid heap fragmentation
// Maximum 16 redirects with up to 64 chars each for from/to paths
static char redirectStringPool[16 * 2 * 64] = {0};
static int redirectStringPoolIndex = 0;

// Helper function to copy String to string pool and return pointer
static const char *copyToRedirectStringPool(const String &str) {
  if (redirectStringPoolIndex + str.length() + 1 >=
      sizeof(redirectStringPool)) {
    // Fallback to heap allocation (not ideal but prevents crash)
    char *heapCopy = (char *)malloc(str.length() + 1);
    if (heapCopy) {
      memcpy(heapCopy, str.c_str(), str.length());
      heapCopy[str.length()] = '\0';
    }
    return heapCopy;
  }

  char *poolPtr = redirectStringPool + redirectStringPoolIndex;
  memcpy(poolPtr, str.c_str(), str.length());
  poolPtr[str.length()] = '\0';
  redirectStringPoolIndex += str.length() + 1;
  return poolPtr;
}

// Implementation of backward compatibility constructor for RedirectRule
RedirectRule::RedirectRule(const String &from, const String &to)
    : fromPath(copyToRedirectStringPool(from)),
      toPath(copyToRedirectStringPool(to)) {}