#ifndef CERTIFICATE_LOADER_H
#define CERTIFICATE_LOADER_H

#include <cstddef>
#include <cstdint>

// Embedded TLS certificate loading/validation - moved out of WebPlatform
// (src/platform/web_platform_server.cpp) as a self-contained slice of
// Phase 2c's WebPlatform god-class breakup (see PROJECT_PLAN.md). Unlike
// the server/route-dispatch machinery investigated alongside it, this
// piece touches zero WebPlatform member state - it only reads the weak
// linker symbols embedded certificate data resolves to and validates PEM
// framing - so it lifted out cleanly. The server lifecycle and HTTP(S)
// route dispatch it feeds into stay on WebPlatform: they're genuinely one
// interwoven subsystem with route registration, module handling, and
// auth, not a separable component - forcing that apart would be code
// motion without real decoupling, not a safe extraction.
namespace CertificateLoader {

// Resolves the embedded server certificate/key (linked in via
// server_cert.pem/server_key.pem when present) to raw pointers + lengths.
// Returns false if the certificates weren't embedded in this build (the
// weak linker symbols are null) - this is the normal case for builds
// without HTTPS certificates baked in, not an error.
bool getEmbeddedCertificates(const uint8_t **cert_data, size_t *cert_len,
                             const uint8_t **key_data, size_t *key_len);

// Pure validation: does this cert/key pair look like well-formed PEM data?
// Only checks framing (a "-----BEGIN CERTIFICATE-----"/"-----BEGIN..."
// prefix and a minimum length) - not a real X.509/key parse, matching the
// original implementation's scope exactly.
bool isValidPemFormat(const uint8_t *cert_data, size_t cert_len,
                      const uint8_t *key_data, size_t key_len);

// Convenience: getEmbeddedCertificates() + isValidPemFormat() together,
// discarding the pointers - "is there a usable embedded certificate for
// this build to serve HTTPS with at all."
bool areCertificatesAvailable();

} // namespace CertificateLoader

#endif // CERTIFICATE_LOADER_H
