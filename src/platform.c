/*
 * DiskNuke - Secure Data Deletion Tool
 * Platform-specific functions for Windows, Linux, and macOS
 */

#include "disknuke.h"
#include <stdio.h>
#include <sys/stat.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef PLATFORM_LINUX
#include <linux/fs.h>
#endif
#ifdef PLATFORM_MACOS
#include <sys/disk.h>
#endif
#endif

int platform_get_device_size(const char* device_path, uint64_t* size) {
    if (!device_path || !size) {
        return DISKNUKE_ERROR_PLATFORM;
    }
    
    #ifdef PLATFORM_WINDOWS
    HANDLE hDevice = CreateFileA(device_path, GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return DISKNUKE_ERROR_IO;
    }
    
    LARGE_INTEGER li;
    if (!GetFileSizeEx(hDevice, &li)) {
        CloseHandle(hDevice);
        return DISKNUKE_ERROR_IO;
    }
    
    *size = li.QuadPart;
    CloseHandle(hDevice);
    
    #elif defined(PLATFORM_LINUX)
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return DISKNUKE_ERROR_IO;
    }
    
    if (ioctl(fd, BLKGETSIZE64, size) < 0) {
        // If not a block device, try regular file
        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            return DISKNUKE_ERROR_IO;
        }
        *size = st.st_size;
    }
    
    close(fd);
    
    #elif defined(PLATFORM_MACOS)
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return DISKNUKE_ERROR_IO;
    }
    
    uint32_t block_size;
    uint64_t block_count;
    
    if (ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) < 0 ||
        ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) < 0) {
        // If not a block device, try regular file
        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            return DISKNUKE_ERROR_IO;
        }
        *size = st.st_size;
    } else {
        *size = (uint64_t)block_size * block_count;
    }
    
    close(fd);
    #endif
    
    return DISKNUKE_SUCCESS;
}

int platform_is_device(const char* path) {
    if (!path) return 0;
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    #ifdef PLATFORM_WINDOWS
    // On Windows, check if it's a physical drive
    return (strncmp(path, "\\\\.\\PhysicalDrive", 17) == 0);
    #else
    return S_ISBLK(st.st_mode);
    #endif
}

int platform_lock_memory(void* addr, size_t len) {
    #ifdef PLATFORM_WINDOWS
    return VirtualLock(addr, len) ? DISKNUKE_SUCCESS : DISKNUKE_ERROR_PLATFORM;
    #else
    return (mlock(addr, len) == 0) ? DISKNUKE_SUCCESS : DISKNUKE_ERROR_PLATFORM;
    #endif
}

int platform_unlock_memory(void* addr, size_t len) {
    #ifdef PLATFORM_WINDOWS
    return VirtualUnlock(addr, len) ? DISKNUKE_SUCCESS : DISKNUKE_ERROR_PLATFORM;
    #else
    return (munlock(addr, len) == 0) ? DISKNUKE_SUCCESS : DISKNUKE_ERROR_PLATFORM;
    #endif
}
