# DataNuke

**Makes data powerless** | Secure data deletion through encryption and key destruction.

DataNuke implements the official [BSI (Bundesamt für Sicherheit in der Informationstechnik)](https://www.bsi.bund.de/) recommendation for secure data deletion: **Encrypt data with strong encryption (AES-256-CBC), then securely delete all keys**. This method provides reliable protection against unauthorized recovery — provided the key is actually deleted, not just marked as deleted.

> *"Wenn Sie die Daten auf dem Datenträger oder Gerät verschlüsselt haben, reicht es aus, alle Schlüssel sicher zu löschen."*  
> — [BSI CON.6](https://www.bsi.bund.de/SharedDocs/Downloads/DE/BSI/Grundschutz/IT-GS-Kompendium_Einzel_PDFs_2023/03_CON_Konzepte_und_Vorgehensweisen/CON_6_Loeschen_und_Vernichten_Edition_2023.pdf?__blob=publicationFile&v=3) (German Federal Office for Information Security)

[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-blue.svg)](https://www.linux.org/)
[![Platform: macOS](https://img.shields.io/badge/Platform-macOS-lightgrey.svg)](https://www.apple.com/macos/)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-0078D6.svg)](https://www.microsoft.com/windows/)
[![Platform: BSD](https://img.shields.io/badge/Platform-BSD-red.svg)](https://www.bsd.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Language: C](https://img.shields.io/badge/Language-C-00599C.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![BSI Compliant](https://img.shields.io/badge/BSI-Compliant-green.svg)](https://www.bsi.bund.de/)

---

> [!CAUTION]
> **READ AND UNDERSTAND THIS ENTIRE DOCUMENT BEFORE USING THIS TOOL!**
>
> This tool **permanently destroys data** by encrypting it and then securely deleting the encryption key. If you don't save the key when it's displayed, **your data is gone forever** - no recovery is possible, not even with data recovery services.
>
> - **This is NOT a backup tool** - make backups first
> - **This is NOT reversible** - unless you save the encryption key
> - **Test on non-critical data first** - understand how it works before using on important data
> - **Always double-check the target** - verify you're encrypting the correct file/device before confirming

---

## What It Does

Encrypt → Trash → Gone

**Implements the BSI method: "Daten verschlüsseln und Schlüssel wegwerfen" - Encrypt data and throw away the key**

Encrypts files or entire devices in-place with AES-256-CBC, displays the encryption key once, then permanently wipes the key from RAM. Without the key, the encrypted data is permanently irrecoverable!

Based on [BSI (Bundesamt für Sicherheit in der Informationstechnik)](https://www.bsi.bund.de/DE/Themen/Verbraucherinnen-und-Verbraucher/Informationen-und-Empfehlungen/Cyber-Sicherheitsempfehlungen/Daten-sichern-verschluesseln-und-loeschen/Daten-endgueltig-loeschen/daten-endgueltig-loeschen_node.html) recommendations: **"Encrypt-then-Delete-Key"** (BSI CON.6).

## Why This Method?

- **Cryptographically secure**: AES-256, not just pattern overwriting
- **Mathematical certainty**: Without the key, decryption is equivalent to solving a mathematically hard problem. Current estimates suggest breaking AES-256 would require more energy than exists in the observable universe
- **Fast**: Single pass vs. multi-pass wiping (10-20x faster)
- **SSD-safe**: No wear leveling issues
- **Universal**: Works on all storage types

## Core Principles

- **Keep it simple**: POSIX-compliant, minimal dependencies
- **Security first**: Focus on correct implementation of BSI method
- **No bloat**: Reject features that don't serve core mission
- **DataNuke follows the Unix philosophy: Do one thing and do it well**

## Installation

### Linux
```bash
# Debian/Ubuntu
sudo apt-get install build-essential libssl-dev cmake

# Fedora/RHEL/CentOS
sudo dnf install gcc make openssl-devel cmake

# Arch Linux
sudo pacman -S base-devel openssl cmake

# Gentoo
sudo emerge dev-libs/openssl dev-util/cmake

# Build and install
make
sudo make install
```

### macOS
```bash
brew install openssl cmake
make
sudo make install
```

### Windows
```bash
# Install dependencies: Visual Studio, OpenSSL, CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
```

### Manual build (all platforms)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Linux/macOS (requires sudo for /usr/bin):
sudo cmake --install build

# Windows (no sudo needed):
cmake --install build
```

## Usage

```bash
# Encrypt a file
sudo datanuke <file>

# Encrypt a block device (entire drive/partition)
sudo datanuke <device>
```
> [!NOTE]
> **You can safely format, delete, reuse, or physically destroy the file/device.**  
> **After encryption, the file/device is gibberish - worthless without the key.**
>
> To complete secure deletion:
> 1. Remove the encrypted file with normal methods (rm).
> 2. Forget the key if you don't need the data.

### Example: File Encryption

```bash
echo "Sensitive data" > secret.txt
sudo datanuke secret.txt
```

<details>
  <summary>Output:</summary>

```
DataNuke v1.0.0 - Secure Data Deletion (BSI-compliant)

Target: secret.txt
Type:   Regular File
Method: Encrypt-then-Delete-Key
---
ENCRYPTION KEY - SAVE NOW OR LOSE FOREVER

Key: 7ee6c8b5eb89d025e79fb6990d1ea0f78cbe9dd7070159023e94a39a68c399e6
IV:  0486802bd91a4596272e8051ceb42bd5

Key is stored in RAM only and will be wiped immediately.
Write it down now if you need to decrypt later. (both hex values below)
---
OPERATION SUCCESSFUL

Target:         secret.txt
Status:         ENCRYPTED (AES-256-CBC)
Encryption key: SECURELY WIPED FROM MEMORY

The file/device is now encrypted and permanently unrecoverable - worthless without the key.

To complete secure deletion process:
 1) You can safely remove the encrypted file with normal methods.
 2) Forget the key if you do not need to recover the data.
```
</details>

### Example: Block Device Encryption

> [!NOTE]
> USE LIVE SYSTEM (boot from USB) when wiping OS drive!
> Cannot wipe drive with running OS!   
> Unmount before encrypting: `sudo umount /dev/sdb1`

```bash
# Create a test partition or use existing device
sudo datanuke /dev/sdb1
```
<details>
  <summary>Output:</summary>

```
DataNuke v1.0.0 - Secure Data Deletion (BSI-compliant)

Target: /dev/sdb1
Type:   Block Device
Method: Encrypt-then-Delete-Key

Device size: 8.00 GB (8589934592 bytes)

WARNING: This will DESTROY all data on /dev/sdb1!
Type YES to confirm: YES

Encrypting device...

Progress: 8.00 GB / 8.00 GB (100.0%)  

---
ENCRYPTION KEY - SAVE NOW OR LOSE FOREVER

Key: 9f2c1d8e7b6a5f4e3d2c1b0a9f8e7d6c5b4a3f2e1d0c9b8a7f6e5d4c3b2a1f0e
IV:  1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d

Key is stored in RAM only and will be wiped immediately.
Write it down now if you need to decrypt later. (both hex values below)
---
OPERATION SUCCESSFUL

Target:         /dev/sdb1
Status:         ENCRYPTED (AES-256-CBC)
Encryption key: SECURELY WIPED FROM MEMORY

The file/device is now encrypted and permanently unrecoverable - worthless without the key.

To complete secure deletion process:
 1) You can safely remove the encrypted file with normal methods.
 2) Forget the key if you do not need to recover the data.
```
</details>

**What is happened:**
- `secret.txt` was encrypted in-place (same filename, different content)
- `/dev/sdb1` was encrypted in-place (raw sectors overwritten with encrypted data)
- Algorithm: AES-256-CBC with random 256-bit key and 128-bit IV
- Key generation: OpenSSL RAND_bytes() (cryptographically secure)
- Encryption key was displayed on screen (one-time only)
- Key was wiped from RAM with 7-pass Gutmann method (0x00, 0xFF, random, 0x00, volatile)
- Memory protection: POSIX mlock() prevented key from swapping to disk
- Device is now gibberish - can be formatted, reused, or physically destroyed
- Without the key, data recovery is computationally infeasible

## Data Recovery

If you saved the key while it was displayed, decrypt with OpenSSL:

```bash
openssl enc -d -aes-256-cbc \
  -K <your_saved_key_hex> \
  -iv <your_saved_iv_hex> \
  -in secret.txt \
  -out secret_recovered.txt
```

**For permanent deletion:** Don't save the key.

> [!WARNING]
> **Key Storage Security**
>
> If you save the encryption key on a compromised system, your data may NOT be securely deleted. An attacker with access to the system could recover both the encrypted data and the key, defeating the entire purpose of secure deletion.
>
> - **Never save the key on the same system** you're trying to securely delete data from
> - **Store keys offline** (paper, secure external storage) if recovery is needed
> - **For true secure deletion:** Don't save the key at all - just let it be destroyed

> [!NOTE]
> **Security Notes**
>
> - **File is encrypted in-place** - same filename, encrypted content
> - **Works with files and entire devices** - any file type (text, images, videos, databases, archives) or block device (SSD, HDD, USB drives, partitions)
> - **Key stored in RAM only** - never touches disk
> - **Key displayed once** - write it down or lose it forever
> - **7-pass Gutmann wipe** - key is destroyed from memory
> - **AES-256-CBC** - computationally infeasible to break
> - **Encrypted file is unreadable** without the key - permanently destroyed data

## Use Cases
- Selling, gifting, or trading in devices
- Disposing of old hard drives and SSDs
- Irrevocable deletion of sensitive information
- GDPR compliance (Art. 17 - Right to erasure)

**File Encryption - Selling laptop/PC:**
```bash
# Encrypt all files in Documents folder
find ~/Documents -type f -exec sudo datanuke {} \;

# Encrypt specific file types
find ~/Pictures -type f \( -name "*.jpg" -o -name "*.png" \) -exec sudo datanuke {} \;
```

**File Encryption - GDPR compliance (right to erasure):**
```bash
# Works with any file type: CSV, PDF, databases, etc.
sudo datanuke customer_data.csv
sudo datanuke invoices.pdf
sudo datanuke database.sqlite
rm customer_data.csv invoices.pdf database.sqlite
```

**Device Encryption - Wiping entire drive before sale/disposal:**
```bash
# Examples (requires root):
sudo datanuke /dev/sdb        # Entire drive
sudo datanuke /dev/sdb1       # Single partition
sudo datanuke /dev/nvme0n1    # NVMe drive
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines.

## License

MIT License - see [LICENSE](LICENSE) file.

## References

- [BSI: Daten endgültig löschen](https://www.bsi.bund.de/DE/Themen/Verbraucherinnen-und-Verbraucher/Informationen-und-Empfehlungen/Cyber-Sicherheitsempfehlungen/Daten-sichern-verschluesseln-und-loeschen/Daten-endgueltig-loeschen/daten-endgueltig-loeschen_node.html)
- [BSI CON.6: Löschen und Vernichten](https://www.bsi.bund.de/)
- [NIST FIPS 197: AES Standard](https://csrc.nist.gov/publications/detail/fips/197/final)
- [OpenSSL Documentation](https://www.openssl.org/docs/)
