#ifndef SYSTEM_STATUS_HELPERS_H
#define SYSTEM_STATUS_HELPERS_H

#include <Arduino.h>

// Pure pieces of getSystemStatusApiHandler()'s health-color thresholding
// (src/handlers/web_platform_restful_handlers.cpp) - moved out because
// they're genuinely testable in isolation, unlike the rest of that
// handler, which just reads ESP.*/WiFi.* and builds a response.
namespace SystemStatusHelpers {

// Clamps a percentage value into [0, 100].
int clampPercent(int value);

// "good" >= 40% free, "warning" >= 20% free, "danger" below that.
String heapHealthColor(int freeHeapPercent);

// "good" <= 60% used, "warning" <= 80% used, "danger" above that.
String storageHealthColor(int usedSpacePercent);

} // namespace SystemStatusHelpers

#endif // SYSTEM_STATUS_HELPERS_H
