/*
 * DiskNuke - Secure Data Deletion Tool
 * Secure Delete Module - BSI-compliant data deletion
 */

#include "disknuke.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/rand.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

int secure_overwrite(const char* path, size_t passes) {
    if (!path || passes == 0) {
        return DISKNUKE_ERROR_IO;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        perror("Cannot stat file");
        return DISKNUKE_ERROR_IO;
    }
    
    FILE* file = fopen(path, "rb+");
    if (!file) {
        perror("Cannot open file for overwriting");
        return DISKNUKE_ERROR_IO;
    }
    
    size_t file_size = st.st_size;
    unsigned char* buffer = malloc(4096);
    if (!buffer) {
        fclose(file);
        return DISKNUKE_ERROR_MEMORY;
    }
    
    printf("Securely overwriting %s (%zu bytes) with %zu passes...\n", 
           path, file_size, passes);
    
    for (size_t pass = 0; pass < passes; pass++) {
        fseek(file, 0, SEEK_SET);
        
        // Different patterns for each pass (BSI recommendation)
        unsigned char pattern;
        switch (pass % 3) {
            case 0: pattern = 0x00; break;  // All zeros
            case 1: pattern = 0xFF; break;  // All ones
            case 2: pattern = 0xAA; break;  // Alternating pattern
        }
        
        size_t remaining = file_size;
        while (remaining > 0) {
            size_t chunk_size = (remaining < 4096) ? remaining : 4096;
            
            if (pass % 3 == 2 && pass > 0) {
                // Use random data for some passes
                RAND_bytes(buffer, chunk_size);
            } else {
                memset(buffer, pattern, chunk_size);
            }
            
            if (fwrite(buffer, 1, chunk_size, file) != chunk_size) {
                perror("Error writing during overwrite");
                free(buffer);
                fclose(file);
                return DISKNUKE_ERROR_IO;
            }
            
            remaining -= chunk_size;
        }
        
        fflush(file);
        
        #ifndef PLATFORM_WINDOWS
        fsync(fileno(file));  // Force write to disk
        #endif
        
        printf("  Pass %zu/%zu completed\n", pass + 1, passes);
    }
    
    free(buffer);
    fclose(file);
    
    // Finally, delete the file
    if (remove(path) != 0) {
        perror("Error removing file");
        return DISKNUKE_ERROR_IO;
    }
    
    printf("File securely deleted.\n");
    return DISKNUKE_SUCCESS;
}

int secure_delete_file(const char* path) {
    // BSI recommends at least 3 overwrite passes for normal cases
    return secure_overwrite(path, 3);
}

int secure_delete_device(const char* device_path) {
    if (!device_path) {
        return DISKNUKE_ERROR_IO;
    }
    
    printf("WARNING: This will permanently destroy all data on %s\n", device_path);
    printf("Are you sure? Type 'YES' to confirm: ");
    
    char confirmation[10];
    if (fgets(confirmation, sizeof(confirmation), stdin) == NULL) {
        return DISKNUKE_ERROR_IO;
    }
    
    // Remove newline
    confirmation[strcspn(confirmation, "\n")] = 0;
    
    if (strcmp(confirmation, "YES") != 0) {
        printf("Operation cancelled.\n");
        return DISKNUKE_ERROR_IO;
    }
    
    uint64_t device_size = 0;
    if (platform_get_device_size(device_path, &device_size) != DISKNUKE_SUCCESS) {
        fprintf(stderr, "Cannot determine device size\n");
        return DISKNUKE_ERROR_PLATFORM;
    }
    
    printf("Device size: %llu bytes (%.2f GB)\n", 
           (unsigned long long)device_size,
           device_size / (1024.0 * 1024.0 * 1024.0));
    
    #ifdef PLATFORM_WINDOWS
    HANDLE hDevice = CreateFileA(device_path, GENERIC_WRITE, 
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Cannot open device\n");
        return DISKNUKE_ERROR_IO;
    }
    #else
    int fd = open(device_path, O_WRONLY | O_SYNC);
    if (fd < 0) {
        perror("Cannot open device");
        return DISKNUKE_ERROR_IO;
    }
    #endif
    
    unsigned char* buffer = malloc(1024 * 1024);  // 1 MB buffer
    if (!buffer) {
        #ifdef PLATFORM_WINDOWS
        CloseHandle(hDevice);
        #else
        close(fd);
        #endif
        return DISKNUKE_ERROR_MEMORY;
    }
    
    printf("Starting secure device wipe...\n");
    
    uint64_t written = 0;
    while (written < device_size) {
        size_t chunk_size = (device_size - written < 1024 * 1024) ? 
                            (device_size - written) : 1024 * 1024;
        
        RAND_bytes(buffer, chunk_size);
        
        #ifdef PLATFORM_WINDOWS
        DWORD bytes_written;
        if (!WriteFile(hDevice, buffer, chunk_size, &bytes_written, NULL)) {
            fprintf(stderr, "Error writing to device\n");
            free(buffer);
            CloseHandle(hDevice);
            return DISKNUKE_ERROR_IO;
        }
        #else
        ssize_t result = write(fd, buffer, chunk_size);
        if (result < 0) {
            perror("Error writing to device");
            free(buffer);
            close(fd);
            return DISKNUKE_ERROR_IO;
        }
        #endif
        
        written += chunk_size;
        
        // Progress indicator
        int percent = (int)((written * 100) / device_size);
        printf("\rProgress: %d%% ", percent);
        fflush(stdout);
    }
    
    printf("\n");
    free(buffer);
    
    #ifdef PLATFORM_WINDOWS
    CloseHandle(hDevice);
    #else
    close(fd);
    #endif
    
    printf("Device securely wiped.\n");
    return DISKNUKE_SUCCESS;
}
