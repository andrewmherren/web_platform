#include "platform/certificate_loader.h"
#include <cstring>
#include <unity.h>

using namespace CertificateLoader;

namespace {

// Builds a buffer starting with `header`, padded with 'X' out to `len`
// bytes total - mirrors real PEM data's shape (a fixed header followed by
// base64 body) without needing a real certificate.
void buildBuffer(uint8_t *out, size_t len, const char *header,
                 size_t headerOffset = 0) {
  memset(out, 'X', len);
  memcpy(out + headerOffset, header, strlen(header));
}

} // namespace

void test_is_valid_pem_format_accepts_matching_cert_and_key(void) {
  uint8_t cert[64];
  uint8_t key[64];
  buildBuffer(cert, sizeof(cert), "-----BEGIN CERTIFICATE-----");
  buildBuffer(key, sizeof(key), "-----BEGIN PRIVATE KEY-----");

  TEST_ASSERT_TRUE(
      isValidPemFormat(cert, sizeof(cert), key, sizeof(key)));
}

void test_is_valid_pem_format_accepts_any_begin_style_key_header(void) {
  uint8_t cert[64];
  uint8_t key[64];
  buildBuffer(cert, sizeof(cert), "-----BEGIN CERTIFICATE-----");
  buildBuffer(key, sizeof(key), "-----BEGIN RSA PRIVATE KEY-----");

  TEST_ASSERT_TRUE(
      isValidPemFormat(cert, sizeof(cert), key, sizeof(key)));
}

void test_is_valid_pem_format_rejects_wrong_cert_header(void) {
  uint8_t cert[64];
  uint8_t key[64];
  buildBuffer(cert, sizeof(cert), "-----BEGIN PRIVATE KEY-----"); // wrong
  buildBuffer(key, sizeof(key), "-----BEGIN PRIVATE KEY-----");

  TEST_ASSERT_FALSE(
      isValidPemFormat(cert, sizeof(cert), key, sizeof(key)));
}

void test_is_valid_pem_format_rejects_missing_key_header(void) {
  uint8_t cert[64];
  uint8_t key[64];
  buildBuffer(cert, sizeof(cert), "-----BEGIN CERTIFICATE-----");
  memset(key, 'X', sizeof(key)); // no header at all

  TEST_ASSERT_FALSE(
      isValidPemFormat(cert, sizeof(cert), key, sizeof(key)));
}

void test_is_valid_pem_format_rejects_cert_too_short(void) {
  uint8_t cert[20]; // shorter than the 27-byte header
  uint8_t key[64];
  memset(cert, 'X', sizeof(cert));
  buildBuffer(key, sizeof(key), "-----BEGIN PRIVATE KEY-----");

  TEST_ASSERT_FALSE(
      isValidPemFormat(cert, sizeof(cert), key, sizeof(key)));
}

void test_is_valid_pem_format_rejects_key_too_short(void) {
  uint8_t cert[64];
  uint8_t key[20];
  buildBuffer(cert, sizeof(cert), "-----BEGIN CERTIFICATE-----");
  memset(key, 'X', sizeof(key));

  TEST_ASSERT_FALSE(
      isValidPemFormat(cert, sizeof(cert), key, sizeof(key)));
}

void test_get_embedded_certificates_false_when_none_linked_in(void) {
  // Native builds never link in _binary_src_server_cert_pem_start/etc (no
  // embedded-resource build step), so the weak symbols stay null - this is
  // the only branch of getEmbeddedCertificates() reachable natively.
  const uint8_t *certData = nullptr;
  const uint8_t *keyData = nullptr;
  size_t certLen = 0;
  size_t keyLen = 0;

  TEST_ASSERT_FALSE(
      getEmbeddedCertificates(&certData, &certLen, &keyData, &keyLen));
}

void test_are_certificates_available_false_when_none_embedded(void) {
  TEST_ASSERT_FALSE(areCertificatesAvailable());
}

void register_certificate_loader_tests(void) {
  RUN_TEST(test_is_valid_pem_format_accepts_matching_cert_and_key);
  RUN_TEST(test_is_valid_pem_format_accepts_any_begin_style_key_header);
  RUN_TEST(test_is_valid_pem_format_rejects_wrong_cert_header);
  RUN_TEST(test_is_valid_pem_format_rejects_missing_key_header);
  RUN_TEST(test_is_valid_pem_format_rejects_cert_too_short);
  RUN_TEST(test_is_valid_pem_format_rejects_key_too_short);
  RUN_TEST(test_get_embedded_certificates_false_when_none_linked_in);
  RUN_TEST(test_are_certificates_available_false_when_none_embedded);
}
