/*
 * DiskNuke - Secure Data Deletion Tool
 * Crypto Module - AES-256 Encryption according to BSI recommendations
 */

#include "disknuke.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int crypto_init(crypto_context_t* ctx) {
    if (!ctx) return DISKNUKE_ERROR_CRYPTO;
    
    memset(ctx, 0, sizeof(crypto_context_t));
    
    // Generate random key
    if (crypto_generate_key(ctx->key, AES_KEY_SIZE) != DISKNUKE_SUCCESS) {
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    // Generate random IV
    if (RAND_bytes(ctx->iv, AES_BLOCK_SIZE) != 1) {
        fprintf(stderr, "Error generating IV: %s\n", 
                ERR_error_string(ERR_get_error(), NULL));
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    return DISKNUKE_SUCCESS;
}

int crypto_generate_key(uint8_t* key, size_t key_size) {
    if (!key || key_size != AES_KEY_SIZE) {
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    // Use OpenSSL's cryptographically secure random number generator
    if (RAND_bytes(key, key_size) != 1) {
        fprintf(stderr, "Error generating key: %s\n", 
                ERR_error_string(ERR_get_error(), NULL));
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    return DISKNUKE_SUCCESS;
}

int crypto_encrypt_file(const char* input_path, const char* output_path,
                       crypto_context_t* ctx) {
    if (!input_path || !output_path || !ctx) {
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    FILE* input = fopen(input_path, "rb");
    if (!input) {
        perror("Cannot open input file");
        return DISKNUKE_ERROR_IO;
    }
    
    FILE* output = fopen(output_path, "wb");
    if (!output) {
        perror("Cannot open output file");
        fclose(input);
        return DISKNUKE_ERROR_IO;
    }
    
    EVP_CIPHER_CTX* cipher_ctx = EVP_CIPHER_CTX_new();
    if (!cipher_ctx) {
        fprintf(stderr, "Error creating cipher context\n");
        fclose(input);
        fclose(output);
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    // Initialize encryption with AES-256-CBC
    if (EVP_EncryptInit_ex(cipher_ctx, EVP_aes_256_cbc(), NULL, 
                          ctx->key, ctx->iv) != 1) {
        fprintf(stderr, "Error initializing encryption: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        EVP_CIPHER_CTX_free(cipher_ctx);
        fclose(input);
        fclose(output);
        return DISKNUKE_ERROR_CRYPTO;
    }
    
    // Encrypt file in chunks
    unsigned char inbuf[4096];
    unsigned char outbuf[4096 + EVP_MAX_BLOCK_LENGTH];
    int inlen, outlen;
    
    while ((inlen = fread(inbuf, 1, sizeof(inbuf), input)) > 0) {
        if (EVP_EncryptUpdate(cipher_ctx, outbuf, &outlen, inbuf, inlen) != 1) {
            fprintf(stderr, "Error during encryption: %s\n",
                    ERR_error_string(ERR_get_error(), NULL));
            EVP_CIPHER_CTX_free(cipher_ctx);
            fclose(input);
            fclose(output);
            return DISKNUKE_ERROR_CRYPTO;
        }
        fwrite(outbuf, 1, outlen, output);
    }
    
    // Finalize encryption
    if (EVP_EncryptFinal_ex(cipher_ctx, outbuf, &outlen) != 1) {
        fprintf(stderr, "Error finalizing encryption: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        EVP_CIPHER_CTX_free(cipher_ctx);
        fclose(input);
        fclose(output);
        return DISKNUKE_ERROR_CRYPTO;
    }
    fwrite(outbuf, 1, outlen, output);
    
    EVP_CIPHER_CTX_free(cipher_ctx);
    fclose(input);
    fclose(output);
    
    return DISKNUKE_SUCCESS;
}

void crypto_display_key(const crypto_context_t* ctx) {
    if (!ctx) return;
    
    printf("\n");
    printf("========================================\n");
    printf("   ENCRYPTION KEY (Display once only)  \n");
    printf("========================================\n");
    printf("Key (hex): ");
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        printf("%02x", ctx->key[i]);
    }
    printf("\n");
    printf("IV (hex):  ");
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
        printf("%02x", ctx->iv[i]);
    }
    printf("\n");
    printf("========================================\n");
    printf("WARNING: This key will be securely deleted!\n");
    printf("========================================\n\n");
}

int crypto_secure_wipe_key(crypto_context_t* ctx) {
    if (!ctx) return DISKNUKE_ERROR_CRYPTO;
    
    // BSI recommendation: Multiple overwrite passes with different patterns
    // Pass 1: All zeros
    memset(ctx->key, 0x00, AES_KEY_SIZE);
    memset(ctx->iv, 0x00, AES_BLOCK_SIZE);
    
    // Pass 2: All ones
    memset(ctx->key, 0xFF, AES_KEY_SIZE);
    memset(ctx->iv, 0xFF, AES_BLOCK_SIZE);
    
    // Pass 3: Random data
    RAND_bytes(ctx->key, AES_KEY_SIZE);
    RAND_bytes(ctx->iv, AES_BLOCK_SIZE);
    
    // Pass 4: Final zeros
    memset(ctx->key, 0x00, AES_KEY_SIZE);
    memset(ctx->iv, 0x00, AES_BLOCK_SIZE);
    
    // Volatile to prevent compiler optimization
    volatile uint8_t* vkey = (volatile uint8_t*)ctx->key;
    volatile uint8_t* viv = (volatile uint8_t*)ctx->iv;
    for (size_t i = 0; i < AES_KEY_SIZE; i++) {
        vkey[i] = 0;
    }
    for (size_t i = 0; i < AES_BLOCK_SIZE; i++) {
        viv[i] = 0;
    }
    
    return DISKNUKE_SUCCESS;
}

void crypto_cleanup(crypto_context_t* ctx) {
    if (ctx) {
        crypto_secure_wipe_key(ctx);
        memset(ctx, 0, sizeof(crypto_context_t));
    }
}
