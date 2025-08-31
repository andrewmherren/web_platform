#ifndef CRYPTO_PLATFORM_H
#define CRYPTO_PLATFORM_H

// Platform-specific crypto library includes
#ifdef ESP32
  // ESP32 uses built-in mbedTLS
  #include "mbedtls/pkcs5.h"
  #include "mbedtls/sha256.h"
  #include "mbedtls/md.h"
  #include "esp_random.h"
#elif defined(ESP8266)
  // ESP8266 uses BearSSL for crypto operations
  #include "bearssl/bearssl.h"
  // For true random number generation on ESP8266
  extern "C" {
    uint32_t esp_random();
  }
#else
  // Other platforms - basic functionality only
  #include <Arduino.h>
  #warning "Platform-specific crypto not implemented - using basic fallbacks"
#endif

#endif // CRYPTO_PLATFORM_H