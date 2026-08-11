To move this modular OTA framework from a verified prototype to a completely secure, reliable, and professional manufacture-ready IoT product, you must address the remaining gaps across three major domains: **Hardware Security**, **Anti-Bricking/Self-Healing**, and **Production-Grade Infrastructure**.

---

### 1. Hardware Security & Cryptographic Integrity

#### Hardware Secure Boot V2 & Flash Encryption
*   **Gap**: Currently, signature checks occur strictly at the C++ application level (`MbedTlsCryptoEngine`). If an attacker physically accesses the chip, they can flash a malicious binary directly, completely bypassing your application verification code.
*   **Action**: Enable ESP32-S3 **Hardware Secure Boot V2** and **Flash Encryption** in the eFuses during factory provisioning. Once enabled, the physical ROM bootloader enforces signature verification of the bootloader, partition table, and application binary before booting, while keeping all data in flash encrypted with an on-chip, hardware-locked AES-256 key.

#### Monotonic Anti-Rollback (eFuse-Backed)
*   **Gap**: While your control plane validates HSVN (`options.currentHsvn`), a physical attacker can still manually flash an older, officially signed firmware version that contains known vulnerabilities.
*   **Action**: Enable ESP-IDF's hardware-backed anti-rollback mechanism (`CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK`). The bootloader validates the security version number of the binary against a secure monotonic counter in the S3's eFuses. If the binary's version is lower, the bootloader rejects the boot.

#### Secure Storage of Trust Anchors
*   **Gap**: Hardcoding CA certs like `OTA_SERVER_CERT` directly in the binary plaintext is insecure and makes rotation highly rigid.
*   **Action**: Utilize a dedicated read-only partition or the ESP-IDF **Digital Signature (DS) Peripheral** / **Secure NVS** to store public keys, server certificates, and client-side private keys. This isolates critical PKI materials from application space.

---

### 2. Resiliency, Anti-Bricking & Self-Healing

#### Provisional Boot Validation State Machine
*   **Gap**: Currently, `OtaManager::validateCurrentFirmware()` is a stub, and the boot partition is permanently set on flash complete. If the new firmware boots but immediately crashes due to an unhandled runtime exception, the device is permanently bricked ("soft-brick").
*   **Action**: Configure the bootloader to boot the new image *provisionally* using the state `ESP_OTA_IMG_PENDING_VERIFY`. Implement the self-healing state machine in `OtaManager`:

```cpp
// In OtaManager::validateCurrentFirmware():
sys_error_t OtaManager::validateCurrentFirmware()
{
    // Perform essential health self-checks (e.g., test NVS, verify peripherals)
    if (runLocalSelfTests() != ERROR_SUCCESS)
    {
        // Self-tests failed: explicitly trigger rollback and reboot
        esp_ota_mark_app_invalid_rollback_and_reboot();
        return ERROR_FAIL;
    }

    // Cancel pending rollback state and commit this partition permanently
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    RETURN_IF_ERROR(
        (err != ESP_OK),
        TRANSLATE_ERROR(err),
        SYS_LOG_E("Failed to cancel rollback state: %s", esp_err_to_name(err))
    );

    return ERROR_SUCCESS;
}
```

#### Resumable Downloads (HTTP Range Requests)
*   **Gap**: If a 2MB firmware download drops at 95% over flaky Wi-Fi or cellular connections, `_memOta.abort()` is called, wiping the staging partition. This causes severe battery drain and data waste.
*   **Action**: Update `OtaHttpTransport` and `IHttpClient` to support HTTP `Range` headers. Track progress in non-volatile storage (NVS). On resume, query the current byte offset from NVS and send `Range: bytes=offset-` to resume downloading and writing from the interrupted sector.

#### Watchdog Feed and Task Scheduling
*   **Gap**: Calculating hashes over a large partition (`_calculatePartitionHash`) or performing continuous raw block writes locks the CPU. On the ESP32-S3, if a loop occupies a core for too long without yielding, the **Task Watchdog Timer (TWDT)** triggers a system reset.
*   **Action**: Introduce explicit yields and watchdog feeding inside large execution loops:

```cpp
// Inside OtaService::_calculatePartitionHash and OtaService::_handleTransportChunk loops:
#include <esp_task_wdt.h>
// Feed the task watchdog explicitly or yield execution to other tasks
vTaskDelay(pdMS_TO_TICKS(1)); // Yields CPU time and feeds the watchdog
```

---

### 3. Fleet Operations & Manufacturing Readiness

#### Production PKI Lifecycle
*   **Gap**: Developing and signing firmware with the same CA or private keys creates massive security risks if a developer machine is compromised.
*   **Action**: Enforce a multi-tier PKI model:
    1.  **Root CA**: Kept strictly offline (air-gapped or stored in a Hardware Security Module - HSM).
    2.  **Subordinate/Intermediate CA**: Used to issue short-lived **Developer Signing Certificates** (`signing.crt`).
    3.  Enforce certificate revocation checks (CRL or OCSP) at the server level during Phase 1 queries.

#### Binary Delta Decompression Constraints
*   **Gap**: Running delta decompression (such as `esp_delta_ota`) inside the dual-core memory space of the ESP32-S3 can exceed the available internal SRAM (~512KB) and trigger Out-Of-Memory (OOM) fatal errors.
*   **Action**: Map all transient decompression dynamic allocation blocks to external **PSRAM (SPIRAM)** using heap caps allocation parameters (`MALLOC_CAP_SPIRAM`), ensuring internal SRAM remains clear for critical system tasks.