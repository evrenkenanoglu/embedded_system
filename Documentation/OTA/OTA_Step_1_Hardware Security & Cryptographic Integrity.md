### AI Implementation Prompt

```text
You are an expert Embedded Security and ESP-IDF Systems Engineer. Your objective is to implement the "Hardware Security & Cryptographic Integrity" phase for an ESP32-S3 firmware update framework.

### SYSTEM CONTEXT & ARCHITECTURE
- Architecture: 3-Tier Layered Architecture (HAL -> PAL -> App -> System).
- Environment: ESP32-S3, ESP-IDF v5.x+, C++17/C++20 with -fno-exceptions and -fno-rtti.
- Memory: Physical flash with dual-bank (OTA_0 / OTA_1) layout, external PSRAM (SPIRAM).
- Error Handling: Use standard sys_error_t returns, error translation via TRANSLATE_ERROR(), and multi-line macro evaluation (RETURN_IF_ERROR, RETURN_ON_ERROR) with right-aligned parameter comments.
- Resource Safety: Enforce RAII, Rule of Five (= delete for copy/move constructors on hardware/state wrappers), no direct dynamic memory allocation inside ISRs, and zero raw struct casting (use std::memcpy for alignment).
- Code Conventions: Allman brace formatting, 4-space indentation, no 'using namespace' in headers, and comprehensive Doxygen comments.

### CORE OBJECTIVES
1. Hardware Secure Boot V2:
   - Configure ESP-IDF bootloader to enforce signature validation on the bootloader, partition table, and application binaries via eFuse-backed public key digest (ECDSA SECP256R1 / RSA-3072).
   - Implement production provisioning modes, digest locking, and JTAG/ROM-download disable mechanisms.
2. Hardware Flash Encryption:
   - Enable transparent AES-XTS flash encryption across all physical partitions except factory/NVS encryption keys.
   - Configure flash encryption in Release mode with physical UART bootloader access restricted.
3. Monotonic Hardware Anti-Rollback:
   - Activate CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK.
   - Map firmware security version (secure_version) to monotonic hardware eFuse counters.
   - Implement HAL checks ensuring incoming binaries strictly satisfy: target_hsvn >= hardware_burned_hsvn.
4. Secure Storage of Trust Anchors & Dynamic Certificates:
   - Eliminate hardcoded CA certificate strings from application memory.
   - Integrate ESP-IDF Encrypted NVS (NVS Encryption with HMACS/eFuse key) or dedicated read-only signed partitions for Root CA storage.
   - Add hardware Digital Signature (DS) peripheral driver abstraction within PAL/Security.

### DELIVERABLES REQUIRED
- Complete, production-ready C++ source and header files implementing the required interfaces.
- Exact sdkconfig/Kconfig definitions required for build-level enforcement.
- Python provisioning scripts (espefuse.py automation) for factory key burning.
- Comprehensive unit/integration test mocks validating eFuse state transitions and signature rejection behavior.
```

---

### Hardware Security & Cryptographic Integrity: Implementation Checklist

#### 1. Silicon Root of Trust & Bootloader Hardening
- [ ] **Secure Boot V2 Activation (`sdkconfig`):**
  - [ ] Set `CONFIG_SECURE_BOOT=y` and `CONFIG_SECURE_BOOT_V2_ENABLED=y`.
  - [ ] Select signature scheme (`CONFIG_SECURE_BOOT_ECDSA_KEY` or `CONFIG_SECURE_BOOT_RSA_KEY`).
  - [ ] Configure `CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y` with secure signing key path management.
  - [ ] Set `CONFIG_SECURE_BOOT_INSECURE_ALLOW_UNUSED_DIGEST_SLOTS=n` for production.
- [ ] **Flash Encryption Configuration:**
  - [ ] Enable `CONFIG_SECURE_FLASH_ENC_ENABLED=y`.
  - [ ] Select mode: `CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y` (Permanent hardware lock).
  - [ ] Configure `CONFIG_SECURE_FLASH_UART_BOOTLOADER_ALLOW_ENCRYPT=n` and `CONFIG_SECURE_FLASH_UART_BOOTLOADER_ALLOW_DECRYPT=n`.
  - [ ] Configure reserved partitions (`nvs_key`, `otadata`, `storage`) with explicit `encrypted` flags in `partitions.csv`.
- [ ] **eFuse Critical Control Bit Enforcement:**
  - [ ] Burn `DISABLE_PAD_JTAG` and `DISABLE_USB_JTAG` to block hardware debugging.
  - [ ] Burn `DIS_DIRECT_BOOT` to disable direct execution from SPI flash without bootloader.
  - [ ] Burn `DIS_DOWNLOAD_ICACHE` and `DIS_DOWNLOAD_DCACHE` to secure boot ROM caches.
  - [ ] Burn `UART_DOWNLOAD_DIS` or configure `ENABLE_SECURITY_DOWNLOAD_MODE` to lock ROM bootloader commands.
  - [ ] Lock read/write access to key blocks (`BLOCK_KEY0` through `BLOCK_KEY5`) via `RD_DIS` / `WR_DIS`.

---

#### 2. Monotonic Anti-Rollback (Hardware eFuse HSVN)
- [ ] **Bootloader Anti-Rollback Engine:**
  - [ ] Enable `CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK=y`.
  - [ ] Define `CONFIG_BOOTLOADER_APP_SEC_VER` in build targets to match project `currentHsvn`.
  - [ ] Configure automatic burning of `SECURE_VERSION` eFuse counter upon successful application boot validation.
- [ ] **HAL / Driver Validation Updates (`mem_ota.cpp`):**
  - [ ] Enhance `mem_ota::_validateIncomingImageHeader` to query physical eFuses via `esp_efuse_read_secure_version()`.
  - [ ] Explicitly compare incoming `newAppInfo.secure_version` against `esp_efuse_read_secure_version()`.
  - [ ] Return `ERROR_INVALID_STATE` and trigger immediate partition abort if `newAppInfo.secure_version < hwSecVersion`.

---

#### 3. Secure Storage of Trust Anchors & PKI Lifecycle
- [ ] **Encrypted NVS for Trust Anchors:**
  - [ ] Implement `CONFIG_NVS_ENCRYPTION=y`.
  - [ ] Generate partition encryption key block via dedicated `nvs_key` partition stored in eFuse key slot.
  - [ ] Refactor `IOtaManager` to pull `serverCert` (Root CA) dynamically from Encrypted NVS rather than compiled string literals.
- [ ] **Hardware Digital Signature (DS) Peripheral Integration:**
  - [ ] Create PAL driver abstraction `Pal_DsEngine` wrapping `esp_ds.h`.
  - [ ] Implement RSA/ECDSA private key acceleration using eFuse-protected hardware keys for client authentication (mTLS).
- [ ] **Root CA Certificate Key Rotation Model:**
  - [ ] Implement secondary fallback Root CA slot inside encrypted storage to support scheduled CA rotation.
  - [ ] Update `MbedTlsCryptoEngine::verifyCertificateChain` to validate against primary and secondary trust anchors.

---

#### 4. Cryptographic Validation Pipeline Hardening
- [ ] **Progressive Digest Verification Safeguards:**
  - [ ] Audit `OtaService::_calculatePartitionHash` to ensure calculation strictly spans `target_size` bytes without trailing flash filler (`0xFF`).
  - [ ] Validate signature decoding buffer limits (`rawSignature` stack bounds check against standard DER/IEEE 1363 formats).
- [ ] **Pre-Download Certificate Revocation Checks:**
  - [ ] Implement serial number validation and expiration boundary checks on incoming `signing_cert` during Phase 1 check.
  - [ ] Enforce CRL (Certificate Revocation List) evaluation in `MbedTlsCryptoEngine::verifyCertificateChain`.

---

#### 5. Factory Floor & Production Tooling
- [ ] **Automated Factory Provisioning Suite:**
  - [ ] Create Python provisioning wrapper (`provision_hardware.py`) using `espefuse.py` and `espsecure.py`.
  - [ ] Implement 2-step signing: Local build generates unsigned binary digest; offline HSM/KMS signs digest for release.
  - [ ] Generate factory audit logs capturing MAC address, burned eFuse hashes, and public key fingerprint per manufactured unit.
- [ ] **Secure Boot / Flash Encryption Test Protocol:**
  - [ ] Implement automated CI integration test executing on test boards to verify rejection of:
    1. Unsigned firmware binaries.
    2. Binaries signed with an untrusted developer key.
    3. Binaries with a downgraded `secure_version` (Anti-Rollback test).
    4. Modified plaintext images flashed directly via `esptool.py`.