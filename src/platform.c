/*
 * ETDK - Encrypt-then-Delete-Key
 * Platform-specific functions for Windows, Linux, and macOS
 */

#include "etdk.h"
// cppcheck-suppress-begin missingIncludeSystem
#include <stdio.h>
#include <sys/stat.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#ifdef PLATFORM_LINUX
#include <linux/fs.h>
#endif
#ifdef PLATFORM_MACOS
#include <sys/disk.h>
#endif
#endif
// cppcheck-suppress-end missingIncludeSystem

/**
 * @brief Get the size of a device or file in bytes
 *
 * Platform-specific implementation:
 * - Windows: Uses GetFileSizeEx() with CreateFileA()
 * - Linux: Uses ioctl() with BLKGETSIZE64 for block devices, fstat() for files
 * - macOS: Uses ioctl() with DKIOCGETBLOCKSIZE/DKIOCGETBLOCKCOUNT, fstat() for files
 *
 * @param device_path Path to the device or file
 * @param size Pointer where size will be stored (in bytes)
 * @return ETDK_SUCCESS on success, error code on failure
 */
int platform_get_device_size(const char *device_path, uint64_t *size) {
    if (!device_path || !size) {
        return ETDK_ERROR_PLATFORM;
    }

#ifdef PLATFORM_WINDOWS
    /* Windows: Open device handle and get file size
     * GENERIC_READ allows reading device properties
     * FILE_SHARE_* flags allow other processes to access device
     */
    HANDLE hDevice =
        CreateFileA(device_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        return ETDK_ERROR_IO;
    }

    LARGE_INTEGER li;
    if (!GetFileSizeEx(hDevice, &li)) {
        CloseHandle(hDevice);
        return ETDK_ERROR_IO;
    }

    *size = li.QuadPart;
    CloseHandle(hDevice);

#elif defined(PLATFORM_LINUX)
    /* Linux: Use ioctl to get block device size
     * BLKGETSIZE64 returns the size in bytes
     * Falls back to fstat() for regular files
     */
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return ETDK_ERROR_IO;
    }

    if (ioctl(fd, BLKGETSIZE64, size) < 0) {
        /* Not a block device, try regular file
         * Some paths may be regular files, not block devices
         */
        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            return ETDK_ERROR_IO;
        }
        *size = st.st_size;
    }

    close(fd);

#elif defined(PLATFORM_MACOS)
    /* macOS: Get block size and count, then multiply
     * DKIOCGETBLOCKSIZE gets bytes per block
     * DKIOCGETBLOCKCOUNT gets total number of blocks
     */
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return ETDK_ERROR_IO;
    }

    uint32_t block_size;
    uint64_t block_count;

    if (ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) < 0 || ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) < 0) {
        // If not a block device, try regular file
        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            return ETDK_ERROR_IO;
        }
        *size = st.st_size;
    } else {
        *size = (uint64_t)block_size * block_count;
    }

    close(fd);
#endif

    return ETDK_SUCCESS;
}

/**
 * @brief Check if a path points to a block device
 *
 * Platform-specific implementation:
 * - Windows: Checks if path starts with "\\\\.\\PhysicalDrive"
 * - Unix: Uses stat() and checks if mode indicates block device (S_ISBLK)
 *
 * @param path Path to check
 * @return 1 if path is a block device, 0 otherwise
 */
int platform_is_device(const char *path) {
    if (!path)
        return 0;

    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }

#ifdef PLATFORM_WINDOWS
    /* Windows: Check for PhysicalDrive prefix
     * Windows uses "\\\\.\\PhysicalDrive0", "\\\\.\\PhysicalDrive1", etc.
     */
    return (strncmp(path, "\\\\.\\PhysicalDrive", 17) == 0);
#else
    /* Unix: Use stat to check file mode
     * S_ISBLK() macro checks if the mode indicates a block device
     */
    return S_ISBLK(st.st_mode);
#endif
}

/**
 * @brief Lock memory pages to prevent swapping to disk
 *
 * Prevents sensitive data (like encryption keys) from being written
 * to swap space where it could be recovered later.
 *
 * Platform-specific implementation:
 * - Windows: Uses VirtualLock()
 * - Unix: Uses mlock()
 *
 * @param addr Address of memory to lock
 * @param len Length of memory region in bytes
 * @return ETDK_SUCCESS on success, ETDK_ERROR_PLATFORM on failure
 */
int platform_lock_memory(void *addr, size_t len) {
#ifdef PLATFORM_WINDOWS
    return VirtualLock(addr, len) ? ETDK_SUCCESS : ETDK_ERROR_PLATFORM;
#else
    return (mlock(addr, len) == 0) ? ETDK_SUCCESS : ETDK_ERROR_PLATFORM;
#endif
}

/**
 * @brief Unlock previously locked memory pages
 *
 * Allows locked memory pages to be swapped again.
 * Should be called after sensitive data has been securely wiped.
 *
 * Platform-specific implementation:
 * - Windows: Uses VirtualUnlock()
 * - Unix: Uses munlock()
 *
 * @param addr Address of memory to unlock
 * @param len Length of memory region in bytes
 * @return ETDK_SUCCESS on success, ETDK_ERROR_PLATFORM on failure
 */
int platform_unlock_memory(void *addr, size_t len) {
#ifdef PLATFORM_WINDOWS
    return VirtualUnlock(addr, len) ? ETDK_SUCCESS : ETDK_ERROR_PLATFORM;
#else
    return (munlock(addr, len) == 0) ? ETDK_SUCCESS : ETDK_ERROR_PLATFORM;
#endif
}
