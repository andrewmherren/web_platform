#include "handlers/system_status_helpers.h"

namespace SystemStatusHelpers {

int clampPercent(int value) {
  if (value < 0)
    return 0;
  if (value > 100)
    return 100;
  return value;
}

String heapHealthColor(int freeHeapPercent) {
  if (freeHeapPercent < 20) {
    return "danger";
  } else if (freeHeapPercent < 40) {
    return "warning";
  }
  return "good";
}

String storageHealthColor(int usedSpacePercent) {
  if (usedSpacePercent > 80) {
    return "danger";
  } else if (usedSpacePercent > 60) {
    return "warning";
  }
  return "good";
}

} // namespace SystemStatusHelpers
