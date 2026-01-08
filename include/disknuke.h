#ifndef DISKNUKE_H
#define DISKNUKE_H

#include <stdint.h>
#include <stddef.h>

#define DISKNUKE_VERSION "1.0.0"
#define AES_KEY_SIZE 32  // 256 bits
#define AES_BLOCK_SIZE 16

// Return codes
#define DISKNUKE_SUCCESS 0
#define DISKNUKE_ERROR_IO -1
#define DISKNUKE_ERROR_CRYPTO -2
#define DISKNUKE_ERROR_MEMORY -3
#define DISKNUKE_ERROR_PLATFORM -4

// Encryption context
typedef struct {
    uint8_t key[AES_KEY_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    void* cipher_ctx;
} crypto_context_t;

// Function prototypes
// crypto.c
int crypto_init(crypto_context_t* ctx);
int crypto_generate_key(uint8_t* key, size_t key_size);
int crypto_encrypt_file(const char* input_path, const char* output_path, 
                        crypto_context_t* ctx);
void crypto_display_key(const crypto_context_t* ctx);
int crypto_secure_wipe_key(crypto_context_t* ctx);
void crypto_cleanup(crypto_context_t* ctx);

// secure_delete.c
int secure_delete_file(const char* path);
int secure_delete_device(const char* device_path);
int secure_overwrite(const char* path, size_t passes);

// platform.c
int platform_get_device_size(const char* device_path, uint64_t* size);
int platform_is_device(const char* path);
int platform_lock_memory(void* addr, size_t len);
int platform_unlock_memory(void* addr, size_t len);

#endif // DISKNUKE_H
