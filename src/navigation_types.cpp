#include "navigation_types.h"

// PROGMEM storage pool for navigation strings to avoid heap fragmentation
// Maximum 16 navigation items with up to 64 chars each for name/url/target
static char navStringPool[16 * 3 * 64] = {0};
static int navStringPoolIndex = 0;

// Helper function to copy String to PROGMEM pool and return pointer
static const char *copyToStringPool(const String &str) {
  if (navStringPoolIndex + str.length() + 1 >= sizeof(navStringPool)) {
    // Fallback to heap allocation (not ideal but prevents crash)
    char *heapCopy = (char *)malloc(str.length() + 1);
    if (heapCopy) {
      strcpy(heapCopy, str.c_str());
    }
    return heapCopy;
  }

  char *poolPtr = navStringPool + navStringPoolIndex;
  strcpy(poolPtr, str.c_str());
  navStringPoolIndex += str.length() + 1;
  return poolPtr;
}

// Implementation of backward compatibility constructors for NavigationItem
NavigationItem::NavigationItem(const String &n, const String &u)
    : name(copyToStringPool(n)), url(copyToStringPool(u)), target(""),
      visibility(NavAuthVisibility::ALWAYS) {}

NavigationItem::NavigationItem(const String &n, const String &u,
                               const String &t)
    : name(copyToStringPool(n)), url(copyToStringPool(u)),
      target(copyToStringPool(t)), visibility(NavAuthVisibility::ALWAYS) {}

NavigationItem::NavigationItem(const String &n, const String &u,
                               NavAuthVisibility vis)
    : name(copyToStringPool(n)), url(copyToStringPool(u)), target(""),
      visibility(vis) {}

NavigationItem::NavigationItem(const String &n, const String &u,
                               const String &t, NavAuthVisibility vis)
    : name(copyToStringPool(n)), url(copyToStringPool(u)),
      target(copyToStringPool(t)), visibility(vis) {}