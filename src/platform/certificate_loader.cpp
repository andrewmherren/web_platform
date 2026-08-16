#include "platform/certificate_loader.h"

#include "utilities/debug_macros.h"
#include <cstring>

namespace CertificateLoader {

bool getEmbeddedCertificates(const uint8_t **cert_data, size_t *cert_len,
                             const uint8_t **key_data, size_t *key_len) {
#ifdef ESP_PLATFORM
  // Check for embedded certificates - these symbols may not exist if
  // certificates weren't embedded. Relies on ELF weak-symbol semantics
  // (undefined weak externs resolve to null instead of a link error) -
  // supported by the ESP-IDF/Xtensa toolchain this runs on for real, but
  // NOT portable to every linker (see the #else branch).
  extern const uint8_t server_cert_pem_start[] asm(
      "_binary_src_server_cert_pem_start") __attribute__((weak));
  extern const uint8_t server_cert_pem_end[] asm(
      "_binary_src_server_cert_pem_end") __attribute__((weak));
  extern const uint8_t server_key_pem_start[] asm(
      "_binary_src_server_key_pem_start") __attribute__((weak));
  extern const uint8_t server_key_pem_end[] asm(
      "_binary_src_server_key_pem_end") __attribute__((weak));

  // Check if certificates are available (weak symbols may be NULL)
  if (!server_cert_pem_start || !server_cert_pem_end || !server_key_pem_start ||
      !server_key_pem_end) {
    return false;
  }

  // Calculate sizes and set pointers
  // NOSONAR: These are linker-generated symbols for embedded binary data
  *cert_len = server_cert_pem_end - server_cert_pem_start;
  *key_len = server_key_pem_end - server_key_pem_start;
  *cert_data = server_cert_pem_start;
  *key_data = server_key_pem_start;

  // Basic sanity check
  return (*cert_len > 100 && *key_len > 100);
#else
  // Native builds have no embedded-binary-resource linker step, so there's
  // never anything to resolve here - and MinGW's linker hard-fails on an
  // undefined weak extern instead of resolving it to null the way ESP-IDF's
  // toolchain does, so the #ifdef ESP_PLATFORM branch above isn't portable
  // to native at all. Always unavailable, which is the correct answer.
  (void)cert_data;
  (void)cert_len;
  (void)key_data;
  (void)key_len;
  return false;
#endif
}

bool isValidPemFormat(const uint8_t *cert_data, size_t cert_len,
                      const uint8_t *key_data, size_t key_len) {
  static const char *const CERT_HEADER = "-----BEGIN CERTIFICATE-----";
  static const size_t CERT_HEADER_LEN = 27; // deliberately matches the
                                             // 27-byte prefix window below,
                                             // not strlen(CERT_HEADER)
  static const char *const KEY_HEADER_FRAGMENT = "-----BEGIN";
  static const size_t KEY_HEADER_FRAGMENT_LEN = 10;

  if (cert_len <= CERT_HEADER_LEN || key_len <= CERT_HEADER_LEN) {
    return false;
  }

  // Cert must start with the exact 27-byte PEM certificate header.
  if (std::memcmp(cert_data, CERT_HEADER, CERT_HEADER_LEN) != 0) {
    return false;
  }

  // Key just needs "-----BEGIN" to appear somewhere in its first 27 bytes
  // (covers "-----BEGIN PRIVATE KEY-----", "-----BEGIN RSA PRIVATE
  // KEY-----", etc. without caring which).
  for (size_t i = 0; i + KEY_HEADER_FRAGMENT_LEN <= CERT_HEADER_LEN; i++) {
    if (std::memcmp(key_data + i, KEY_HEADER_FRAGMENT,
                    KEY_HEADER_FRAGMENT_LEN) == 0) {
      return true;
    }
  }
  return false;
}

bool areCertificatesAvailable() {
  const uint8_t *cert_data, *key_data;
  size_t cert_len, key_len;

  if (!getEmbeddedCertificates(&cert_data, &cert_len, &key_data, &key_len)) {
    DEBUG_PRINTLN("WebPlatform: No embedded certificates found");
    return false;
  }

  if (isValidPemFormat(cert_data, cert_len, key_data, key_len)) {
    DEBUG_PRINTF("WebPlatform: SSL certificates validated (cert: %d bytes, "
                 "key: %d bytes)\n",
                 cert_len, key_len);
    return true;
  }

  DEBUG_PRINTLN("WebPlatform: Invalid certificate format");
  return false;
}

} // namespace CertificateLoader
