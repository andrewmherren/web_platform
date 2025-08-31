#include "../../include/auth/auth_utils.h"

#ifdef ESP8266

// PBKDF2 implementation for ESP8266 using BearSSL
// This provides a secure password hashing function for production use

namespace AuthUtils {

void pbkdf2_sha256(const uint8_t *password, size_t password_len,
                   const uint8_t *salt, size_t salt_len,
                   uint32_t iterations,
                   uint8_t *output, size_t output_len) {
  
  br_hmac_key_context hmac_key_ctx;
  br_hmac_key_init(&hmac_key_ctx, &br_sha256_vtable, password, password_len);
  
  size_t blocks_needed = (output_len + 31) / 32; // SHA256 produces 32-byte blocks
  
  for (size_t block = 1; block <= blocks_needed; block++) {
    br_hmac_context hmac_ctx;
    uint8_t u[32]; // SHA256 output size
    uint8_t block_result[32];
    
    // Initialize HMAC with password
    br_hmac_init(&hmac_ctx, &hmac_key_ctx, 32);
    
    // First iteration: HMAC(password, salt || block_number)
    br_hmac_update(&hmac_ctx, salt, salt_len);
    
    // Add block number in big-endian format
    uint8_t block_bytes[4];
    block_bytes[0] = (uint8_t)(block >> 24);
    block_bytes[1] = (uint8_t)(block >> 16);
    block_bytes[2] = (uint8_t)(block >> 8);
    block_bytes[3] = (uint8_t)(block);
    
    br_hmac_update(&hmac_ctx, block_bytes, 4);
    br_hmac_out(&hmac_ctx, u);
    
    // Copy first iteration result
    memcpy(block_result, u, 32);
    
    // Remaining iterations
    for (uint32_t i = 1; i < iterations; i++) {
      br_hmac_init(&hmac_ctx, &hmac_key_ctx, 32);
      br_hmac_update(&hmac_ctx, u, 32);
      br_hmac_out(&hmac_ctx, u);
      
      // XOR with accumulated result
      for (int j = 0; j < 32; j++) {
        block_result[j] ^= u[j];
      }
    }
    
    // Copy to output buffer
    size_t bytes_to_copy = min(32, (int)(output_len - (block - 1) * 32));
    memcpy(output + (block - 1) * 32, block_result, bytes_to_copy);
  }
}

} // namespace AuthUtils

#endif // ESP8266