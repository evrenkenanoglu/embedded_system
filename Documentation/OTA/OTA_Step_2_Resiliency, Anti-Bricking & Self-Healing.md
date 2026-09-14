### AI Implementation Prompt

```text
You are an expert Embedded Systems Reliability and RTOS Engineer specializing in fault-tolerant firmware architectures for ESP-IDF. Your objective is to implement the "Resiliency, Anti-Bricking & Self-Healing" domain for the ESP32-S3 OTA framework.

### SYSTEM CONTEXT & ARCHITECTURE
- Architecture: 3-Tier Layered Architecture (HAL: mem_ota -> PAL: OtaService, OtaHttpTransport, HttpsClient -> App: OtaManager).
- Environment: ESP32-S3 (Dual-Core Xtensa LX7), ESP-IDF v5.x+, FreeRTOS, C++17/C++20 with -fno-exceptions.
- Partition Scheme: A/B dual-boot layout with otadata partition tracking active boot slots.
- Error Handling: Standard sys_error_t codes, multi-line macros (RETURN_IF_ERROR, RETURN_ON_ERROR) with right-aligned comments, and TRANSLATE_ERROR() for ESP-IDF APIs.
- Code Standards: Strict RAII (Rule of Five on hardware/connection abstractions), Allman brace formatting, 4-space indentation, and Doxygen method documentation.

### CORE OBJECTIVES
1. Provisional Boot Validation & App Self-Test Engine:
   - Transition the bootloader to enforce ESP_OTA_IMG_PENDING_VERIFY upon flashing new images.
   - Refactor OtaManager::validateCurrentFirmware() from a stub into an active self-healing state machine.
   - Implement an extensible self-test callback pipeline (NVS validation, peripheral checks, network link verification).
   - Enforce automatic rollback via esp_ota_mark_app_invalid_rollback_and_reboot() on self-test failure, runtime panic, or watchdog expiration.
   - Commit boot partition via esp_ota_mark_app_valid_cancel_rollback() only after all self-tests succeed.
2. Resumable Chunk Downloads (HTTP Range Requests):
   - Extend IHttpClient, HttpsClient, and OtaHttpTransport to support HTTP 'Range: bytes=offset-' headers.
   - Implement an NVS Checkpoint Manager that persists download progress (written byte offset, target SHA-256, version tag, transient URL).
   - Ensure flash writes on resumed sessions align with flash erase sector boundaries (4096 bytes) and avoid duplicate writes to uncleared sectors.
   - Add checkpoint invalidation logic if target version, hash, or file size changes between check requests.
3. Task Watchdog Timer (TWDT) & CPU Starvation Prevention:
   - Subscribe OTA worker tasks to the ESP-IDF Task Watchdog Timer (TWDT).
   - Inject cooperative RTOS yields (vTaskDelay(pdMS_TO_TICKS(1))) and TWDT feeds inside blocking loops (progressive hashing in OtaService::_calculatePartitionHash, stream writes, and delta decompression).
4. Post-Rollback Diagnostics & Telemetry:
   - Persist crash/rollback diagnostics in a dedicated NVS namespace before rollback execution.
   - On fallback boot into the previous partition, transmit a specialized failure telemetry report to /api/v1/ota/status detailing the exact rollback cause.

### DELIVERABLES REQUIRED
- Refactored C++ header and source files for OtaManager, OtaService, OtaHttpTransport, HttpsClient, and mem_ota.
- New NVS Checkpoint abstraction class (OtaCheckpointManager) with clean PAL/HAL boundaries.
- Concrete self-test test harness implementation.
- Unit/mock tests simulating power cuts, corrupted chunks, HTTP dropouts, and self-test assertion failures.
```

---

### Resiliency, Anti-Bricking & Self-Healing: Implementation Checklist

#### 1. Provisional Boot & Runtime Self-Healing Engine
- [ ] **Bootloader Provisional State Enforcement:**
  - [ ] Configure `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in `sdkconfig`.
  - [ ] Verify that `mem_ota::setBootPartition()` leaves the newly written image in state `ESP_OTA_IMG_PENDING_VERIFY`.
- [ ] **Application Self-Test Framework:**
  - [ ] Define `OtaSelfTestHook_t` function pointer type in `IOtaManager.hpp` returning `sys_error_t`.
  - [ ] Implement core diagnostic checks:
    - [ ] `checkNvsIntegrity()`: Validates read/write access to non-volatile namespaces.
    - [ ] `checkPeripherals()`: Verifies local I2C, SPI, and GPIO expander initialization.
    - [ ] `checkNetworkConnectivity()`: Confirms Wi-Fi connection and gateway reachability.
    - [ ] `checkTaskHealth()`: Confirms all system tasks spawned successfully without stack overflows.
- [ ] **Provisional Validation State Machine (`OtaManager.cpp`):**
  - [ ] Replace `OtaManager::validateCurrentFirmware()` stub:
    - [ ] Query running partition state via `esp_ota_get_state_partition()`.
    - [ ] If state is `ESP_OTA_IMG_PENDING_VERIFY`, execute registered self-test routines.
    - [ ] On failure: Call `mem_ota::markAppInvalid()` (`esp_ota_mark_app_invalid_rollback_and_reboot()`).
    - [ ] On success: Call `mem_ota::markAppValid()` (`esp_ota_mark_app_valid_cancel_rollback()`).
- [ ] **Watchdog Rollback Protection:**
  - [ ] Set rollback watchdog timeout window via `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`.
  - [ ] Ensure fatal runtime exceptions (Guru Meditation / panics) automatically trigger hardware reset to initiate bootloader rollback.

---

#### 2. Resumable Chunked Downloads (HTTP Range Requests)
- [ ] **HTTP Client Range Header Extension:**
  - [ ] Update `HttpClientOptions_t` / `IHttpClient::sendRequest` to accept an optional byte offset range.
  - [ ] Modify `HttpsClient::_prepare_request` to append `Range: bytes=<offset>-` when resuming an active session.
  - [ ] Handle HTTP status `206 Partial Content` as a valid success code alongside `200 OK`.
- [ ] **NVS OTA Checkpoint Manager:**
  - [ ] Create `OtaCheckpointManager` class to encapsulate resume state:
    - [ ] `saveCheckpoint(version, targetSize, bytesWritten, hashContext, url)`
    - [ ] `loadCheckpoint(outCheckpoint)`
    - [ ] `clearCheckpoint()`
  - [ ] Implement validation logic: discard checkpoints if firmware version, target size, or image hash mismatch the server manifest.
- [ ] **Flash Partition Resumption Safety (`mem_ota.cpp`):**
  - [ ] Modify `mem_ota::begin(size_t imageSize, size_t startOffset)` to support starting at a non-zero byte boundary.
  - [ ] Ensure partial downloads do not erase already written sectors if resuming within valid partition bounds.
  - [ ] Verify that `startOffset` aligns with physical flash write block constraints (4 KB boundaries).

---

#### 3. Task Watchdog Timer (TWDT) & CPU Starvation Prevention
- [ ] **TWDT Registration:**
  - [ ] Subscribe the OTA execution task to the Task Watchdog Timer using `esp_task_wdt_add(NULL)`.
  - [ ] Unsubscribe task upon update completion or error teardown (`esp_task_wdt_delete(NULL)`).
- [ ] **Progressive Hash Yielding (`OtaService.cpp`):**
  - [ ] In `OtaService::_calculatePartitionHash`, insert cooperative yielding and watchdog feeding inside the 4 KB read loop:
    ```cpp
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1));
    ```
- [ ] **Stream Processing Yielding (`OtaService.cpp` & `mem_ota.cpp`):**
  - [ ] Inject watchdog resets in `_handleTransportChunk` during high-throughput network stream processing.
  - [ ] Insert watchdog feeds inside delta decompression feed cycles (`esp_delta_ota_feed_patch`).

---

#### 4. Power Interruption & Fault Recovery
- [ ] **Atomic Boot Flag Management:**
  - [ ] Verify that `otadata` partition writes are atomic and resilient to mid-write brownouts.
  - [ ] Validate that an incomplete or corrupted `otadata` sequence causes the ROM bootloader to fall back to the last known valid partition slot (`ota_0` or `ota_1`).
- [ ] **Brownout Detector Integration:**
  - [ ] Enable hardware brownout detector in `sdkconfig` (`CONFIG_ESP_BROWNOUT_DET=y`).
  - [ ] Configure brownout reset threshold to prevent flash corruption during voltage dips caused by Wi-Fi/flash peak currents.

---

#### 5. Rollback Diagnostics & Telemetry
- [ ] **Crash Reason Persistence:**
  - [ ] Implement an NVS crash log recorder (`saveRollbackDiagnostic(reasonCode, subErrorCode)`).
  - [ ] Record self-test failure IDs or panic addresses prior to invoking `markAppInvalid()`.
- [ ] **Post-Rollback Reporting Loop:**
  - [ ] During `OtaManager::init()`, inspect NVS for pending rollback diagnostic flags.
  - [ ] If a rollback flag is detected:
    - [ ] Construct telemetry payload to `/api/v1/ota/status` (`status: "rollback"`, `failed_version`, `reason_code`).
    - [ ] Transmit payload once network connectivity is established.
    - [ ] Clear diagnostic flag from NVS upon successful server acknowledgement.