---
name: hardware-security-crypto-integrity
description: >-
  Production-grade specifications, architectural invariants, code conventions,
  and implementation rules for Hardware Security, Secure Boot V2, Flash Encryption,
  Monotonic Anti-Rollback, and Cryptographic PKI on ESP32-C6 firmware systems.
  Activate when implementing, reviewing, or debugging cryptographic engines,
  OTA security pipelines, eFuse configurations, or provisioning toolchains.
---

# Hardware Security & Cryptographic Integrity Skill

This skill defines the mandatory architectural invariants, implementation rules, and coding standards for all hardware security, memory validation, and cryptographic verification subsystems across `Embedded_IoT_BT_WIFI_Base_Project`.

---

## 1. Architectural Invariants (Zero-Tolerance Rules)

### A. Hardware Silicon Root of Trust
* **Permanent Hardware Locks:** In production configurations, Flash Encryption must use Release mode (`CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y`). Once enabled, physical UART bootloader decryption and encryption must be disabled.
* **Key Read/Write Protection:**
  * `BLOCK_KEY0` (Flash Encryption, `XTS_AES_128_KEY`) must be both write-protected and read-protected (`read_protect_efuse`, `write_protect_efuse`).
  * `BLOCK_KEY2` (Secure Boot V2, `SECURE_BOOT_DIGEST1`) must be write-protected and read-enabled.
  * Debug interfaces (`DIS_PAD_JTAG`, `DIS_USB_JTAG`, `DIS_DIRECT_BOOT`, `DIS_DOWNLOAD_ICACHE`, `DIS_DOWNLOAD_DCACHE`) must be permanently burned in silicon.
* **No Secret Exposure in SRAM:** Cryptographic hardware accelerators (AES, SHA, DS peripheral) must be used where available. Private keys must not reside in plain static heap memory longer than the immediate cryptographic transaction.

### B. Monotonic Anti-Rollback (HSVN)
* **Pre-Flash Header Interception:** The physical flash memory driver (`mem_ota.cpp`) must validate `newAppInfo.secure_version >= esp_efuse_read_secure_version()` within the first accumulated 320-byte image chunk.
* **Fail-Fast Boundary:** If `secure_version < hwSecVersion`, writing must abort immediately with `ERROR_INVALID_STATE` before any flash erase or write operation proceeds.

### C. Cryptographic Decoupling (SRP & Layer Isolation)
* **HAL Layer (`Source/HAL/`):** Manages physical sector writing, image header decoding, and partition state transitions (`esp_ota_*`, `esp_partition_*`). Must have zero dependency on X.509 parsing or signature verification logic.
* **PAL Layer (`Source/PAL/`):** Manages protocol state machines (`OtaService`) and cryptographic verification abstraction (`ICryptoEngine`).
* **Platform Independence:** The abstract interface `ICryptoEngine.hpp` must remain free of vendor headers (`mbedtls/*`, `esp_idf/*`). Concrete implementations (`MbedTlsCryptoEngine`) reside exclusively in `Source/PAL/Platform/<Platform>/Security/`.

### D. Progressive Integrity Verification
* **Target-Bounded Hashing:** The progressive SHA-256 digest calculation over a written flash partition must span exactly `target_size` bytes. It is forbidden to hash up to `partition->size`, as unwritten flash contains erased padding (`0xFF`) that invalidates the signature.
* **Dual Trust Anchor Model:** The Root CA store must accommodate a primary and a secondary backup certificate to support planned CA rotation without bricking devices.

---

## 2. Coding Standards & C++ Conventions

All C++ code written for this subsystem must strictly adhere to these rules:

1. **Brace Style:** Allman style exclusively (opening braces on their own line).
2. **Indentation:** Exactly 4 spaces (no tabs).
3. **Namespaces:** Never use `using namespace` in header files.
4. **Exceptions & RTTI:** Built with `-fno-exceptions` and `-fno-rtti`. Use `sys_error_t` return codes and the `RETURN_IF_ERROR` / `RETURN_ON_ERROR` macros.
5. **Memory Safety & RAII:**
   * Rule of Five: Delete copy/move constructors on hardware, state wrappers, and cryptographic contexts.
   * Zero raw struct casting: Use `std::memcpy` when deserializing byte streams to avoid strict-aliasing and alignment faults.
   * Auto-cleanup contexts: Wrap mbedTLS and FreeRTOS handles in RAII structs that call corresponding `free` / `delete` functions in destructors.
6. **Error Translation:** Translate vendor-specific error codes (`esp_err_t`, mbedTLS negative ints) to unified `sys_error_t` using `TRANSLATE_ERROR()`.

---

## 3. Cryptographic Formats & Specifications

| Component | Standard | Notes |
| :--- | :--- | :--- |
| **Secure Boot V2** | RSA-3072 | Enforced by ESP32-C6 ROM bootloader (`SECURE_BOOT_DIGEST1` in `BLOCK_KEY2`) |
| **Firmware Signatures** | ECDSA SECP256R1 | Raw IEEE P1363 format ($R \parallel S$, 64 bytes total) |
| **Digests** | SHA-256 | 32-byte raw binary |
| **Flash Encryption** | AES-128-XTS | 256-bit key burned to `BLOCK_KEY0` |
| **NVS Encryption** | AES-XTS | 64-byte key binary from `nvs_keys` partition |
| **Certificates** | X.509 v3 (PEM format) | Must include `KeyUsage: digitalSignature` |

---

## 4. Verification & Validation Protocol

Before marking any security task complete:
1. **Compilation Check:** Code must compile cleanly with `-Wall -Wextra -Werror` without unused-variable or unhandled-return warnings.
2. **Dry-Run Validation:** Python provisioning scripts must execute without error using `--dry-run` and `--simulate`.
3. **eFuse Verification:** Confirm all eFuse modifications are guarded against accidental execution on real silicon when `force_burn: false`.
4. **Signature Failure Assertions:** Unit tests must assert that modifying any single bit in the binary payload or signature results in `ERROR_AUTH_FAILED`.

## Error Registry

Before hardware security or cryptography work, review this registry and any related failure notes in applicable skills. Record verified, reusable mistakes found during the task before considering the work complete. Each entry must capture the symptom, root cause, and a preventive rule; use `ERR-NNN` IDs scoped to this file. Do not record speculation, transient output, or secrets. If no incident has been recorded yet, add the first entry here.
