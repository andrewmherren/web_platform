#ifndef PLATFORM_SERVICE_H
#define PLATFORM_SERVICE_H

#include <interface/web_request.h>

class IPlatformService {
public:
  virtual ~IPlatformService() = default;

  // Platform information
  virtual String getDeviceName() const = 0;
  virtual bool isHttpsEnabled() const = 0;
};

// Global accessor for modules to use
extern IPlatformService *getPlatformService();

extern IPlatformService *getPlatformService();
extern IPlatformService *g_platformService;

#endif