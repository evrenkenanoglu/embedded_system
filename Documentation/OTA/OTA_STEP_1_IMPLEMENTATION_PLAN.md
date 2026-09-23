### Implementation Plan: Missing Hardware Security & Cryptographic Integrity Features

---

### 1. Firmware HAL & PAL Implementations (C++)

| Target File | Missing Feature | Implementation Requirement |
| :--- | :--- | :--- |
| `embedded_system/Source/HAL/Platform/ESP32/mem_ota.cpp` | **Hardware eFuse Anti-Rollback Guard** | In `_validateIncomingImageHeader()`, query `esp_efuse_read_secure_version()` via `#include <esp_efuse.h>`. Compare against `esp_app_desc_t::secure_version` of incoming binary chunk. Abort partition write and return error immediately if `new_app_desc.secure_version < hw_burned_version`. |
| `embedded_system/Source/PAL/Platform/Esp32/Security/Pal_DsEngine.hpp` / `.cpp` | **Hardware Digital Signature (DS) Driver** | Create PAL wrapper around `esp_ds.h` to execute hardware-accelerated ECDSA/RSA operations using silicon eFuse keys (`BLOCK_KEY2` / `BLOCK_KEY3`) without exposing private keys to application RAM. |
| `embedded_system/Source/HAL/Platform/ESP32/cpx_credentialsManager.cpp` | **Dynamic NVS Root CA & Backup Slots** | Read `sec_pki/root_ca_pem` and `sec_pki/root_ca_backup_pem` from encrypted `fctry` NVS partition using `nvs_flash_read_security_cfg()` and `nvs_open_from_partition()`. Cache in secure heap. |
| `embedded_system/Source/PAL/Platform/Esp32/Protocol/OTA/OtaService.cpp` | **Exact Target-Size Progressive Hash** | Modify `_calculatePartitionHash` to stream strictly `target_size` bytes from flash. Terminate read loop at `target_size` to avoid hashing trailing erased sector padding (`0xFF`). |
| `embedded_system/Source/PAL/Platform/Esp32/Security/MbedTlsCryptoEngine.cpp` | **Dual Trust-Anchor & Revocation Checks** | 1. Initialize `mbedtls_x509_crt` with both primary Root CA and fallback backup Root CA.<br>2. Parse certificate serial number and enforce expiration bounds (`not_before` / `not_after`) prior to binary download.<br>3. Enforce strict DER-to-raw IEEE P1363 (64-byte $R \parallel S$) stack bound validations. |

---

### 2. Kconfig & Build System Hardening

Update `sdkconfig.defaults` and `configs/config_project.yaml` to switch from development simulation to hardware release locks.

| Configuration Symbol | Target Value | Purpose |
| :--- | :--- | :--- |
| `CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE` | `y` | Permanent hardware silicon lock. Prevents switching back to development mode. |
| `CONFIG_SECURE_FLASH_UART_BOOTLOADER_ALLOW_ENCRYPT` | `n` | Prohibits the ROM bootloader from encrypting arbitrary code over UART. |
| `CONFIG_SECURE_FLASH_UART_BOOTLOADER_ALLOW_DECRYPT` | `n` | Prohibits UART bootloader from reading flash contents in plaintext. |
| `CONFIG_SECURE_FLASH_UART_BOOTLOADER_ALLOW_CACHE` | `n` | Disables instruction cache when running in UART bootloader mode. |
| `CONFIG_SECURE_BOOT_INSECURE_ALLOW_UNUSED_DIGEST_SLOTS` | `n` | Revokes unused digest slots to prevent attackers burning alternative keys. |
| `CONFIG_SECURE_ENABLE_SECURE_ROM_DL_MODE` | `y` | Restricts ROM download mode to signed operations only. |
| `CONFIG_SECURE_BOOT_ALLOW_JTAG` | `n` | Hardens Secure Boot against software-based JTAG re-enablement. |

---

### 3. Provisioning Tooling & eFuse Enforcement

Update `embedded_system/Source/Scripts/provisioning/factory/provision_hardware.py` and `nvs_ota_template.csv`.

#### A. NVS Template Extension (`nvs_ota_template.csv`)
Add rotation slot to the template schema:
```csv
sec_pki,namespace,,
root_ca_pem,file,string,{ROOT_CA_PATH}
root_ca_backup_pem,file,string,{ROOT_CA_BACKUP_PATH}
```

#### B. Hardware Provisioning Script Updates (`provision_hardware.py`)
Add explicit read/write disabling commands after key burning:
```python
def lock_silicon_efuses(port: str, baud: int, dry_run: bool) -> None:
    """Enforces permanent silicon locking of keys and debug ports."""
    locks = [
        # Write-protect key blocks (prevent overwriting)
        ["write_protect_efuse", "BLOCK_KEY0"],
        ["write_protect_efuse", "BLOCK_KEY1"],
        # Read-protect Flash Encryption key (hardware AES only)
        ["read_protect_efuse", "BLOCK_KEY0"],
    ]
    for action, target in locks:
        cmd = [
            sys.executable, "-m", "espefuse",
            "--port", port, "--baud", str(baud),
            "--do-not-confirm", action, target
        ]
        run_command(cmd, f"Locking eFuse: {action} on {target}", dry_run)
```

#### C. Two-Step Detached HSM Signing (`hsm_sign_digest.py`)
Separate binary hashing from key usage to allow offline HSM/KMS integration:
* `--stage digest`: Computes and exports binary SHA-256 digest to `.bin`.
* `--stage assemble`: Ingests detached signature file (`.sig`), developer certificate (`.crt`), and injects them into the release manifest.

---

### 4. Automated Verification & CI/HIL Test Suite

Implement automated test cases in `CI_CD/tests/hil/test_crypto_security.py` to run against physical target hardware or QEMU:

```
CI_CD/tests/hil/test_crypto_security.py
├── test_verify_efuse_locks()              # Asserts JTAG disabled and key blocks read-protected
├── test_reject_unsigned_binary()          # Asserts bootloader halts on unsigned image
├── test_reject_tampered_signature()       # Flashes 1-byte corrupted binary; asserts signature failure
├── test_reject_downgraded_hsvn()          # Flashes image with secure_version < HW; asserts rejection
└── test_reject_plaintext_flash_write()   # Writes plaintext directly via esptool; asserts boot crash
```

---

### 5. Implementation Execution Sequence

```text
[Phase 1: Configuration & SSoT]
    ├── Update sdkconfig.defaults with Release Mode eFuse flags
    └── Add root_ca_backup_pem to config_project.yaml and nvs_ota_template.csv

[Phase 2: Firmware Drivers & Verification]
    ├── Implement eFuse HSVN check in mem_ota.cpp
    ├── Update MbedTlsCryptoEngine.cpp with boundary checks and dual-anchor parsing
    ├── Patch OtaService.cpp to enforce strict target_size hashing
    └── Implement Pal_DsEngine hardware abstraction

[Phase 3: Provisioning Pipeline Hardening]
    ├── Add eFuse read/write protection steps to provision_hardware.py
    └── Decouple hsm_sign_digest.py into detached digest/assembly stages

[Phase 4: Validation]
    ├── Run dry-run validation: inv es.crypto.provision-hardware --simulate
    └── Execute HIL suite: inv es.ci-cd.hil --config configs/config_hil.yaml
```