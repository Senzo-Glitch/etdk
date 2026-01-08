/*
 * DiskNuke - Secure Data Deletion Tool
 * According to BSI recommendations (Bundesamt für Sicherheit in der Informationstechnik)
 * 
 * Method: Encrypt data with AES-256, then securely delete encryption key
 * This implements the BSI-recommended "Encrypt and throw away key" method
 */

#include "disknuke.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage(const char* program_name) {
    printf("DiskNuke v%s - Secure Data Deletion Tool\n", DISKNUKE_VERSION);
    printf("Based on BSI recommendations (Germany)\n\n");
    printf("Usage: %s [OPTIONS] <target>\n\n", program_name);
    printf("Options:\n");
    printf("  -f <file>      Delete a single file securely\n");
    printf("  -d <device>    Wipe entire device (DANGEROUS!)\n");
    printf("  -e <file>      Encrypt file and delete key\n");
    printf("  -p <passes>    Number of overwrite passes (default: 3)\n");
    printf("  -h             Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s -f secret.txt              # Securely delete a file\n", program_name);
    printf("  %s -e document.pdf            # Encrypt then delete key\n", program_name);
    printf("  %s -d /dev/sdb                # Wipe entire device\n", program_name);
    printf("  %s -f data.bin -p 7           # 7 overwrite passes\n\n", program_name);
    printf("WARNING: This tool permanently destroys data!\n");
    printf("         Make backups before use!\n");
}

int main(int argc, char* argv[]) {
    int opt;
    char* file_path = NULL;
    char* device_path = NULL;
    char* encrypt_file = NULL;
    int passes = 3;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "f:d:e:p:h")) != -1) {
        switch (opt) {
            case 'f':
                file_path = optarg;
                break;
            case 'd':
                device_path = optarg;
                break;
            case 'e':
                encrypt_file = optarg;
                break;
            case 'p':
                passes = atoi(optarg);
                if (passes < 1 || passes > 100) {
                    fprintf(stderr, "Error: Passes must be between 1 and 100\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║         DiskNuke v%s                 ║\n", DISKNUKE_VERSION);
    printf("║  Secure Data Deletion (BSI-compliant)   ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
    
    int result = DISKNUKE_SUCCESS;
    
    // Encrypt file and delete key
    if (encrypt_file) {
        printf("Mode: Encrypt and delete key (BSI recommended method)\n");
        printf("Target: %s\n\n", encrypt_file);
        
        crypto_context_t ctx;
        if (crypto_init(&ctx) != DISKNUKE_SUCCESS) {
            fprintf(stderr, "Failed to initialize cryptography\n");
            return 1;
        }
        
        // Lock key in memory to prevent swapping
        platform_lock_memory(&ctx, sizeof(ctx));
        
        char encrypted_path[512];
        snprintf(encrypted_path, sizeof(encrypted_path), "%s.encrypted", encrypt_file);
        
        printf("Step 1: Encrypting file with AES-256...\n");
        result = crypto_encrypt_file(encrypt_file, encrypted_path, &ctx);
        
        if (result == DISKNUKE_SUCCESS) {
            printf("Encryption successful!\n\n");
            
            printf("Step 2: Displaying encryption key (ONLY ONCE)...\n");
            crypto_display_key(&ctx);
            
            printf("Press ENTER to continue and permanently delete the key...");
            getchar();
            
            printf("\nStep 3: Securely deleting encryption key...\n");
            result = crypto_secure_wipe_key(&ctx);
            
            if (result == DISKNUKE_SUCCESS) {
                printf("Key securely deleted!\n\n");
                
                printf("Step 4: Deleting original file...\n");
                result = secure_delete_file(encrypt_file);
                
                if (result == DISKNUKE_SUCCESS) {
                    printf("\n");
                    printf("╔══════════════════════════════════════════╗\n");
                    printf("║          OPERATION SUCCESSFUL            ║\n");
                    printf("╚══════════════════════════════════════════╝\n");
                    printf("\n");
                    printf("Original file: DELETED\n");
                    printf("Encrypted file: %s\n", encrypted_path);
                    printf("Encryption key: SECURELY DELETED\n");
                    printf("\nThe encrypted file is now permanently unrecoverable.\n");
                }
            }
        }
        
        platform_unlock_memory(&ctx, sizeof(ctx));
        crypto_cleanup(&ctx);
    }
    // Secure file deletion
    else if (file_path) {
        printf("Mode: Secure file deletion with overwriting\n");
        printf("Target: %s\n", file_path);
        printf("Passes: %d\n\n", passes);
        
        result = secure_overwrite(file_path, passes);
    }
    // Device wipe
    else if (device_path) {
        printf("Mode: Complete device wipe\n");
        printf("Target: %s\n\n", device_path);
        
        if (!platform_is_device(device_path)) {
            fprintf(stderr, "Warning: %s does not appear to be a block device\n", 
                    device_path);
        }
        
        result = secure_delete_device(device_path);
    }
    else {
        fprintf(stderr, "Error: No operation specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    printf("\n");
    
    if (result != DISKNUKE_SUCCESS) {
        fprintf(stderr, "Operation failed with error code: %d\n", result);
        return 1;
    }
    
    return 0;
}
