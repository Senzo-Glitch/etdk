# ETDK Developer Guide

Technical documentation for contributors.

## Architecture

```
main.c  → Entry point, CLI handling
crypto.c → AES-256-CBC encryption, key generation, key wiping
platform.c → Memory locking (mlock/VirtualLock)
```

## Project Structure

```
src/
├── main.c       # CLI + workflow
├── crypto.c     # Encryption + key management
└── platform.c   # OS-specific memory operations

include/
└── etdk.h   # Public API
```

## Build

### Quick Build (Debug with full symbols)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
```

### Build with GDB Debugging
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-g -O0 -Wall -Wextra -Wpedantic"
cmake --build . -j4
```

### Run with GDB
```bash
gdb ./etdk
(gdb) run test.txt
(gdb) break crypto_encrypt_file
(gdb) continue
```

### Build with Valgrind Memory Analysis
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
valgrind --leak-check=full --show-leak-kinds=all ./etdk test.txt
```

### Build with Address Sanitizer (Memory errors)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-g -O1 -fsanitize=address -fno-omit-frame-pointer"
cmake --build . -j4
./etdk test.txt  # Will report memory issues
```

### Build with Code Coverage
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-g -O0 --coverage"
cmake --build . -j4
./etdk test.txt
gcov src/crypto.c src/platform.c src/main.c
```

### Compile Commands for IDE (VS Code, CLion, etc.)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# Creates compile_commands.json for IDE code analysis
```

### Testing
```bash
# Run automated test script
bash ../test_etdk.sh

# Manual encryption test
echo "test secret data" > test.txt
./etdk test.txt
hexdump -C test.txt  # Verify it's encrypted
```

## Core Functions

### crypto.c

**Key Management:**
- `crypto_init()` (line 51) - Initialize context, generate random key/IV with RAND_bytes()
- `crypto_generate_key()` (line 82) - Generate cryptographically secure random key
- `crypto_display_key()` (line 182) - Display key once (POSIX-style plain text, 3-second pause)
- `crypto_secure_wipe_key()` (line 219) - 5-pass secure key wipe
- `crypto_cleanup()` (line 270) - Free OpenSSL context and wipe all sensitive data

**Encryption:**
- `init_cipher_context()` (line 25) - Helper: Initialize EVP cipher context (reduces duplication)
- `crypto_encrypt_file()` (line 103) - AES-256-CBC file encryption (4KB chunks)
- `crypto_encrypt_device()` (line 284) - AES-256-CBC block device encryption (1MB chunks)

### main.c

**Workflow (main function, line 44):**
1. Parse args including --help/-h/help flags (line 45-53)
2. Check if target is file or block device with `platform_is_device()` (line 58)
3. Lock crypto context in RAM with `platform_lock_memory()` (line 85)
4. Encrypt file or device with AES-256-CBC (line 93 or 74)
5. Display key once - plain text, no countdown (line 106)
6. Wipe key from memory with 5-pass method (line 109)
7. Unlock memory with `platform_unlock_memory()` (line 129)
8. Output: POSIX-compliant, no ANSI colors

### platform.c

**Memory Protection:**
- `platform_lock_memory()` - mlock (Unix) / VirtualLock (Windows) - Prevents key swapping to disk
- `platform_unlock_memory()` - munlock / VirtualUnlock - Allows memory to be swapped again
- `platform_get_device_size()` - Get size of block device in bytes
- `platform_is_device()` - Check if path is a block device vs regular file

## Key Security

**Key Lifecycle (Encrypt-then-Delete-Key Method):**
1. Generated with `RAND_bytes()` (CSPRNG) → `crypto_init()` line 64
2. Locked in RAM with `mlock()` (no swap) → `main.c` line 85
3. Used for encryption (file or device) → `crypto_encrypt_file()` or `crypto_encrypt_device()`
4. Displayed once (plain text, save now or lose forever) → `crypto_display_key()` line 182-207
5. 3-second pause for user to save key → `sleep(3)` line 207
6. Wiped with 5-pass secure method → `crypto_secure_wipe_key()` line 219-263
7. Memory unlocked → `main.c` line 129

Without the key, encrypted data is permanently irrecoverable (BSI method).

**Secure Key Wipe Implementation (`crypto_secure_wipe_key()`):**

```c
// Pass 1: Overwrite with zeros (0x00) - line 226-228
memset(ctx->key, 0x00, AES_KEY_SIZE);
memset(ctx->iv, 0x00, AES_BLOCK_SIZE);

// Pass 2: Overwrite with ones (0xFF) - line 233-235
memset(ctx->key, 0xFF, AES_KEY_SIZE);
memset(ctx->iv, 0xFF, AES_BLOCK_SIZE);

// Pass 3: Overwrite with random data - line 240-242
RAND_bytes(ctx->key, AES_KEY_SIZE);
RAND_bytes(ctx->iv, AES_BLOCK_SIZE);

// Pass 4: Final overwrite with zeros - line 247-249
memset(ctx->key, 0x00, AES_KEY_SIZE);
memset(ctx->iv, 0x00, AES_BLOCK_SIZE);

// Pass 5: Volatile pointer overwrite (prevents compiler optimization) - line 254-261
volatile uint8_t *vkey = (volatile uint8_t *)ctx->key;
volatile uint8_t *viv = (volatile uint8_t *)ctx->iv;
for (size_t i = 0; i < AES_KEY_SIZE; i++) {
    vkey[i] = 0;
}
for (size_t i = 0; i < AES_BLOCK_SIZE; i++) {
    viv[i] = 0;
}
```

**Why 5 passes are sufficient (not 35 like Gutmann):**

1. **RAM has no magnetic remanence** - Unlike HDDs, RAM cells don't retain "ghost" data after overwrite
2. **No data recovery from modern RAM** - Once overwritten, data in DRAM/SRAM is immediately lost
3. **Random data prevents pattern analysis** - Pass 3 uses `RAND_bytes()` (cryptographic RNG) which makes any residual electrical patterns unpredictable
4. **Volatile pointers defeat compiler optimization** - Pass 5 ensures the compiler can't optimize away the writes (critical!)
5. **BSI recommendations** - BSI (German Federal Office for Information Security) recommends multi-pass with random data for volatile memory
6. **Performance vs security balance** - 5 passes provide cryptographic-level security without unnecessary overhead

**Why NOT 35 passes like Gutmann?**
- Gutmann's 35-pass method was designed for **magnetic media** (HDDs) with data remanence
- Modern SSDs and RAM have **no magnetic properties**
- Cold boot attacks are mitigated by the random data pass
- Additional passes beyond 5 provide no security benefit for RAM

**Security guarantee:** After 5 passes with random data and volatile overwrite, key recovery is computationally infeasible, even with physical memory access

## POSIX-Style Output

**Design Principles:**
- No ANSI escape codes (no colors)
- No Unicode box-drawing characters
- No emojis or fancy formatting
- Plain text output only
- Compatible with all POSIX terminals

**Key Display:**
```c
printf("---\n");
printf("ENCRYPTION KEY - SAVE NOW OR LOSE FOREVER\n\n");
printf("Key: %s\n", key_hex);
printf("IV:  %s\n\n", iv_hex);
printf("Key is stored in RAM only and will be wiped immediately.\n");
printf("Write it down now if you need to decrypt later.\n");
printf("---\n");
sleep(3);  // Silent pause, no countdown
```

## Device Support

**Block Device Encryption:**
- Detects devices with `platform_is_device()`
- Gets device size with `platform_get_device_size()`
- Requires "YES" confirmation before encryption
- Cannot encrypt mounted devices
- Cannot encrypt device with running OS

**Supported Devices:**
- `/dev/sdb`, `/dev/sdc`, etc. (entire drives)
- `/dev/sdb1`, `/dev/sdb2`, etc. (partitions)
- `/dev/nvme0n1`, `/dev/nvme0n1p1` (NVMe drives)

## Contributing

1. Fork repo
2. Create feature branch
3. Make changes
4. Test with: `bash test_etdk.sh`
5. Run Codacy analysis (see below)
6. Test device mode (requires root): `sudo ./build/etdk /dev/loop0`
7. Submit PR

## Pre-Commit Checks

Before pushing your changes, run these quality checks:

### Automated Test Script
```bash
bash test_etdk.sh
# Runs encryption test with hexdump comparison
```

### Code Quality Analysis (Codacy)
```bash
# Install Codacy CLI (optional but recommended)
npm install -g @codacy/codacy-cli

# Run local analysis
codacy-cli analyze --tool eslint
```

### Memory Safety (Valgrind)
```bash
cd build
valgrind --leak-check=full --show-leak-kinds=all ./etdk ../test_file.txt
# Check for memory leaks before pushing
```

### Compiler Warnings
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build .
# All warnings must be fixed (treated as errors)
```

## Performance Profiling

### Profile with Perf (Linux)
```bash
cd build
perf record -g ./etdk large_file.iso
perf report
# Analyze performance hotspots
```

### Benchmark File Encryption
```bash
cd build
# Create test file
dd if=/dev/urandom of=test_1gb.bin bs=1M count=1024

# Time the encryption
time ./etdk test_1gb.bin
hexdump -C test_1gb.bin | head  # Verify encrypted
```

### Check Memory Footprint
```bash
/usr/bin/time -v ./etdk large_file.bin
# Shows max RSS, page faults, etc.
```

## Environment Setup

### macOS
```bash
# Install dependencies
brew install cmake openssl

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DOPENSSL_DIR=$(brew --prefix openssl@3)
cmake --build . -j4
```

### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get install cmake libssl-dev build-essential

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
```

### Windows (MSVC)
```bash
# Install OpenSSL from https://slproweb.com/products/Win32OpenSSL.html
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 16 2019"
cmake --build . --config Debug
```

## Testing

```bash
# Manual test
echo "test" > test.txt
sudo ./build/etdk test.txt
hexdump -C test.txt  # Should be encrypted

# Decrypt with saved key
openssl enc -d -aes-256-cbc -K <key> -iv <iv> -in test.txt -out recovered.txt
```

## Code Style

- C11 standard
- Linux kernel style (indent -linux)
- Doxygen comments for functions
- Max 80 chars per line

## Common Issues & Troubleshooting

### Build Fails: "Cannot find OpenSSL"
```bash
# macOS
cmake .. -DOPENSSL_DIR=$(brew --prefix openssl@3)

# Linux - install libssl-dev
sudo apt-get install libssl-dev

# Windows - download from https://slproweb.com/products/Win32OpenSSL.html
```

### Build Fails: "CMakeCache.txt conflict"
```bash
# Clean build directory
rm -rf build build-release
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
```

### Encryption Too Slow (Large Files)
```bash
# Use Release build instead of Debug
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
# Release build is 5-10x faster due to -O3 optimization
```

### Memory Errors with Valgrind
```bash
# If suppressions needed, create valgrind.supp:
valgrind --leak-check=full --suppressions=valgrind.supp ./etdk test.txt
```

### Device Not Found
```bash
# Verify device exists and permissions
ls -la /dev/sdb*
sudo ./etdk /dev/sdb  # May require root

# Use loop device for testing (no data loss)
sudo losetup /dev/loop0 test_file.iso
sudo ./etdk /dev/loop0
sudo losetup -d /dev/loop0
```

### GDB Debugging Tips
```bash
# Start GDB
gdb ./etdk

# Set breakpoint
(gdb) break crypto_encrypt_file
(gdb) break main

# Run with arguments
(gdb) run test.txt

# Print variables
(gdb) print ctx->key[0]

# Continue execution
(gdb) continue

# Step through code
(gdb) step
(gdb) next

# Print stack trace
(gdb) backtrace
```

## Security Testing Checklist

Before any release, verify:

- [ ] No keys printed to logs
- [ ] No keys in error messages
- [ ] Memory properly locked with mlock
- [ ] Secure wipe uses volatile pointers
- [ ] OpenSSL errors checked everywhere
- [ ] No buffer overflows (use fixed sizes)
- [ ] Input validation on paths
- [ ] No hardcoded test keys/IVs in code
- [ ] Valgrind shows no leaks
- [ ] Address Sanitizer shows no errors

## Documentation

### Building API Docs (Doxygen)
```bash
# Install Doxygen
sudo apt-get install doxygen graphviz  # Linux
brew install doxygen graphviz          # macOS

# Generate docs
doxygen Doxyfile
# Output in docs/html/index.html
```

### Function Documentation Format
All functions use Doxygen-style comments:
```c
/**
 * @brief Short one-line description
 *
 * Detailed explanation of what the function does,
 * implementation details, algorithm description.
 *
 * @param param_name Description of parameter
 * @return Return value description
 */
```

### Key Documentation Files
- `etdk.h` - Public API with full Doxygen comments for all exported functions
- `crypto.c` - Crypto implementation with detailed algorithm explanations
- `main.c` - CLI workflow with step-by-step comments
- `platform.c` - Platform-specific implementations with OS differences documented

### Update README
- Keep installation instructions current
- Document all command-line options
- Add usage examples
- Update supported platforms

### Update DEVELOPER_GUIDE
- Add new build options
- Document new functions
- Add troubleshooting for new issues
- Keep references current

## Release Process

```bash
# 1. Ensure all tests pass
bash test_etdk.sh

# 2. Update version in include/etdk.h
#define ETDK_VERSION "1.0.1"

# 3. Update CHANGELOG
# Add new features, bugfixes, security updates

# 4. Commit and tag
git add -A
git commit -m "Release v1.0.1"
git tag -a v1.0.1 -m "Release version 1.0.1"
git push origin master --tags

# 5. Build Release binary
rm -rf build-release
mkdir build-release && cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
strip etdk  # Remove debug symbols

# 6. Create GitHub Release with binary
```

## Performance Optimization

### Profile-Guided Optimization
```bash
# Build with PGO
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-fprofile-generate"
cmake --build . -j4

# Run representative workload
./etdk large_test_file.bin

# Rebuild with profile data
cmake .. -DCMAKE_C_FLAGS="-fprofile-use -fprofile-correction"
cmake --build . -j4
```

### Compiler Optimizations
- `-O3` - Aggressive optimization (Release build)
- `-O2` - Standard optimization
- `-O1` - Light optimization (with Address Sanitizer)
- `-march=native` - CPU-specific optimizations (performance)

## References

- [OpenSSL EVP API](https://www.openssl.org/docs/man3.0/man7/evp.html)
- [BSI CON.6](https://www.bsi.bund.de/)
- [NIST AES](https://csrc.nist.gov/publications/detail/fips/197/final)

---

For issues: [GitHub Issues](https://github.com/damachine/etdk/issues)
