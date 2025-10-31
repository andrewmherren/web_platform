#include "../include/platform_provider.h"
#include "../include/web_platform.h"
#include <web_platform_interface.h>

#ifndef MAKER_API_STANDALONE_TEST

// Production platform provider implementation
static WebPlatformProvider* productionProvider = nullptr;

void setupProductionPlatformProvider() {
    if (!productionProvider) {
    productionProvider = new WebPlatformProvider(static_cast<IWebPlatform*>(&webPlatform));
        IWebPlatformProvider::instance = productionProvider;
    }
}

#endif // MAKER_API_STANDALONE_TEST