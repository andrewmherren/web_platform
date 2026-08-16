#include "../include/web_platform.h"

void WebPlatform::printUnifiedRoutes() const { router.printUnifiedRoutes(); }

size_t WebPlatform::getRouteCount() const {
  return router.getEnabledRouteCount();
}
