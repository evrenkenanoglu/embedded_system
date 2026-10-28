# Skill: Universal Fleet OTA & Dual-Phase Update Protocol

Platform-independent architectural specification, interface contracts, cryptographic invariants, and runtime state machines for resilient Over-The-Air (OTA) firmware update orchestration across embedded systems.

---

## 1. Architectural Abstraction Hierarchy

The system enforces strict inversion of control. Business logic, scheduling, canary routing, and state persistence operate purely against abstract interfaces without compile-time hardware dependencies:

```
[ Application Layer (Platform-Independent) ]
       │
       ▼
   IOtaManager ◄───► Concrete: OtaManager (Orchestrator, Jitter, Canary, Fallback)
       │
       ├──► IOtaService (State Machine Coordinator: Idle, Stream, Verify, Apply)
       │       │
       │       ├──► IOtaTransport (Stream Adapter: Chunking, Range Resumption)
       │       │       └──► IHttpClient (Generic Transport Socket & TLS Engine)
       │       │
       │       ├──► ICryptoEngine (Progressive SHA-256, X.509 Parsing, Signature Validation)
       │       │
       │       └──► IHal_Mem_Ota (Dual-Bank Flash Writer, Bank Selector, Image Validator)
       │
       ├──► IHal_Mem (Key-Value / Block Storage: Factory Anchors, Checkpoints, Logs)
       │
       └──► IHal_Sys_Wdt (Watchdog Timer: Task Starvation & Anti-Hang Defense)
```

### Interface Role Matrix

| Abstract Interface  | Core Responsibilities                                                                                                     | Expected Implementations                                       |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| **`IOtaManager`**   | Handshake execution, version comparison (SemVer), anti-rollback checks, self-test verification, and reboot triggers.      | `OtaManager`                                                   |
| **`IOtaService`**   | Stream byte counting, progressive partition hashing bounded to `target_size`, watchdog feeding, and state coordination.   | `OtaService`                                                   |
| **`IOtaTransport`** | URL parsing, HTTP `Range:` header emission, chunk streaming, and dynamic server trust anchor injection.                   | `OtaHttpTransport`                                             |
| **`IHttpClient`**   | Platform socket connection, HTTP verb execution, and TLS transport context management.                                    | `HttpsClient`, `CurlClient`, `PosixHttpClient`                 |
| **`ICryptoEngine`** | Progressive streaming digest calculation, X.509 chain validation, and asymmetric signature verification.                  | `MbedTlsCryptoEngine`, `WolfCryptEngine`, `OpenSslEngine`      |
| **`IHal_Mem_Ota`**  | Flash partition geometry abstraction, A/B dual-bank writing, boot slot selection, and provisional trial-state management. | `mem_ota` (ESP-IDF), `zephyr_ota_mem`, `stm32_flash_dual_bank` |
| **`IHal_Mem`**      | Dynamic storage reader/writer for Root CAs, 4 KB sector checkpoints, and rollback diagnostic records.                     | `mem_nvs` (NVS), `posix_fs`, `eeprom_driver`                   |
| **`IHal_Sys_Wdt`**  | Hardware/RTOS watchdog kicking during heavy hashing and stream processing to prevent starvation resets.                   | `sys_wdt` (Task WDT), `zephyr_wdt`                             |

---

## 2. Server Control Plane & REST Transport Protocol

The backend is a passive, zero-key delivery daemon. Private signing keys are prohibited on the delivery server.

### Canonical Endpoints

| Method         | Endpoint                      | Purpose                                                    | Authentication                             |
| -------------- | ----------------------------- | ---------------------------------------------------------- | ------------------------------------------ |
| **POST / GET** | `/api/v1/ota/check`           | Update discovery, HSVN gating, and canary evaluation.      | Device API Key Header (`X-Device-API-Key`) |
| **GET**        | `/api/v1/ota/download/{file}` | Serves full images or pre-computed 1-hop patches.          | Presigned HMAC-SHA256 Token (`?token=`)    |
| **POST**       | `/api/v1/ota/status`          | Dispatches post-update execution outcomes and diagnostics. | Device API Key Header (`X-Device-API-Key`) |

### Client Query Headers (`/api/v1/ota/check`)

* `X-Device-API-Key`: Static or derived device authentication token.
* `x-Device-Version`: Running firmware version (e.g. `1.0.0`).
* `x-Device-Hardware`: Hardware model profile string (e.g. `MODEL-X1-REV2`).
* `x-Device-ID`: Globally unique device identifier (e.g. MAC address, UUID).
* `x-Device-Channel`: Assigned cohort deployment channel (`stable`, `beta`, `dev`).
* `x-Device-HSVN`: Current monotonic Hardware Security Version Number.

### Deterministic Stateless Canary Formula

The delivery daemon groups devices into canary tiers without storing client state:

$$\text{Inclusion Bucket} = \left( \text{MD5}(\text{DeviceID} + \text{":"} + \text{TargetVersion}) \pmod{100} \right)$$

$$\text{Serve Update if: } \text{Inclusion Bucket} < \text{CanaryPercentage}$$

* Expanding a canary ring from 10% to 25% deterministically retains the initial 10% cohort and adds the next 15% without state tracking.

### Presigned Transient Download Tokens

Direct binary links are protected by time-limited HMAC-SHA256 signatures:

$$\text{Token} = \text{HMAC-SHA256}(\text{Key}=\text{API\_KEY}, \text{Msg}=\text{Filename} + \text{":"} + \text{ExpirationTimestamp})$$

* Default expiration window: $T_{\text{current}} + 300\text{ seconds}$.
* Enforces single-session lifetime and prevents URL sharing across unauthorized clients.

### Closed-Loop Fleet Protection & Auto-Rollback

The server aggregates execution results submitted to `/api/v1/ota/status` over a trailing sliding window ($T = 3600\text{ s}$):

$$\text{Failure Rate} = \frac{\text{Failures}}{\text{Successes} + \text{Failures}} \times 100$$

* **Trigger Condition:** If $\text{Total Reports} \ge N_{\text{min}}$ (e.g., $\ge 5$) and $\text{Failure Rate} \ge R_{\text{max}}$ (e.g., $\ge 10.0\%$):
  1. Target version is automatically demoted from `active` to `soft-rolled-back` in `manifest.json`.
  2. The release channel pointer (`latest_version`) reverts to the last known healthy release.
  3. Subsequent `/check` requests automatically serve the fallback firmware.

---

## 3. Cryptographic Chain of Trust & PKI Hierarchy

Authenticity, transport confidentiality, and anti-downgrade validation operate on distinct layers:

```
                       [ Master Root CA ] (Offline / HSM)
                               │
               ┌───────────────┴───────────────┐
               ▼                               ▼
    [ TLS Server Certificate ]    [ Code-Signing Certificate ]
        (Expires: 1 Year)              (Expires: 3–5 Years)
               │                               │
               │ (Transport Session)           │ (Asymmetric Verification)
               ▼                               ▼
      HTTPS Server Session             Target Image Signature
```

### Cryptographic Security Contract

* **Trust Anchor Storage (Zero `.rodata` Invariant):** The public Master Root CA certificate is never baked into application read-only data (`.rodata`). It resides in dedicated, authenticated, or encrypted non-volatile factory storage (`fctry`).
* **Key Delegation:** The Master Root CA issues subordinate developer signing certificates (`signing.crt`). The embedded client verifies that `signing.crt` chains back to the pre-provisioned Root CA before parsing the public key.
* **Detached Signature Format:** Firmware images carry a detached raw asymmetric signature (e.g., IEEE P1363 $R \parallel S$, 64 bytes for `ec-secp256r1`).
* **Strict Hashing Boundary:** The SHA-256 partition hash is calculated across **strictly `target_size` bytes**. Any trailing flash sector padding (`0xFF`) is excluded to guarantee mathematical reproducibility.
* **Hardware Anti-Rollback (HSVN):** Incoming update manifests advertise a target `hsvn`. The update is rejected if:

$$\text{target\_hsvn} < \text{current\_hsvn}$$

---

## 4. Manifest Schema (v1.0.0) Contract

The artifact repository (`release-catalog/manifest.json`) structures releases, channels, and differential diffs:

```json
{
  "schema_version": "1.0.0",
  "project_name": "Project_Unified",
  "channels": {
    "stable": {
      "latest_version": "1.1.0",
      "hardware_device": "DEVICE-PROFILE-A"
    }
  },
  "releases": {
    "1.0.0": {
      "hardware": "DEVICE-PROFILE-A",
      "channel": "stable",
      "status": "active",
      "hsvn": 1,
      "canary_percentage": 100,
      "min_loader_version": "1.0.0",
      "binary": {
        "file_name": "Project_Unified_1.0.0.bin",
        "size_bytes": 1773568,
        "sha256": "4c994049c7db1396a095bad3951e27aa655c68be539844f1cfc3bebc54ca960d",
        "signature": "7cc54f05838770aeedb944e370eeda45efa346eb2e0b6e0f7abf28c60527e855..."
      },
      "signing_cert": "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n",
      "release_notes": "Initial factory baseline.",
      "patches": {}
    },
    "1.1.0": {
      "hardware": "DEVICE-PROFILE-A",
      "channel": "stable",
      "status": "active",
      "hsvn": 2,
      "canary_percentage": 100,
      "min_loader_version": "1.0.0",
      "binary": {
        "file_name": "Project_Unified_1.1.0.bin",
        "size_bytes": 1774080,
        "sha256": "8a32d1f9a88c7f...",
        "signature": "e4ec6041c6157b5d..."
      },
      "signing_cert": "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n",
      "release_notes": "Security and performance patch.",
      "patches": {
        "1.0.0": {
          "file_name": "patches/patch_1.0.0_to_1.1.0.bin",
          "size_bytes": 48210,
          "sha256": "d9812a34...",
          "signature": ""
        }
      }
    }
  }
}
```

---

## 5. 1-Hop Delta Compression Model

To prevent exponential complexity in differential matrix management, delta updates are strictly constrained to **immediate predecessor versions** ($V_{N-1} \to V_N$):

```
       1-Hop Available:                                  No Delta Available (>1 Hop):
 [ v1.0.0 ] ─── Delta Patch (~48 KB) ───► [ v1.1.0 ]     [ v0.9.0 ] ──── Full Download (~1.77 MB) ────► [ v1.1.0 ]
```

### Delta Rules
1. **Engine:** Built using `detools` or `bsdiff` binary diffing against raw binary files.
2. **Determinism:** Named strictly as `patches/patch_{prev_version}_to_{target_version}.bin`.
3. **Collision Guard:** If $\text{SHA256}(V_{N-1}) == \text{SHA256}(V_N)$, delta creation is bypassed.
4. **Verification Rule:** Client verifies the final reconstructed image against the full `target_hash` and `target_signature`. Delta patches remain unsigned on the delivery server.
5. **Fallback:** If a device runs any version older than $V_{N-1}$, the server serves the full monolithic binary (`update_type: full`).

---

## 6. Dual-Phase Resilient Runtime State Machine

The client update cycle executes across four phases with self-healing anti-bricking guards:

```
  [ IDLE / NORMAL APPLICATION EXECUTION ]
      │
      ▼ (Scheduled Timer with Randomized Jitter: T_base ± Offset)
  [ PHASE 1: GATEWAY QUERY & POLICY GATES ]
      │ ──► POST /api/v1/ota/check
      │ ──► Verify API Key & Identity Headers
      │ ──► Check Canary Placement & Monotonic HSVN
      │ ──► Two-Stage Guard: Verify Battery > 80%, External Power, Maintenance Window
      ▼
  [ PHASE 2: RESUMABLE STREAM & IN-PLACE FLASHING ]
      │ ──► Query IHAL_MEM for Checkpoint ("ota_chkpt")
      │ ──► Resume from last verified 4 KB sector (HTTP 'Range: bytes=offset-')
      │ ──► First Chunk Guard: Parse application descriptor header
      │       └─► Assert: incoming_version != running_version (Reject self-flashing)
      │ ──► Stream chunks to inactive memory bank (Bank B)
      │ ──► Feed Watchdog (IHal_Sys_Wdt) per chunk
      │ ──► Commit progress to Checkpoint per sector boundary
      ▼
  [ PHASE 3: CRYPTOGRAPHIC VERIFICATION & COMMIT ]
      │ ──► Progressive SHA-256 calculation across strictly target_size bytes
      │ ──► Validate X.509 Signing Cert against Primary & Backup Root CAs
      │ ──► Verify asymmetric signature against computed SHA-256 digest
      │ ──► Clear Checkpoint ("ota_chkpt")
      │ ──► Set active boot pointer to Bank B (State: PendingVerify / Trial Boot)
      │ ──► Dispatch success/failure telemetry to /api/v1/ota/status
      ▼
  [ PHASE 4: PROVISIONAL BOOT & SELF-HEALING TRIAL ]
      │ ──► System Reboot
      │ ──► Bootloader boots Bank B provisionally with running watchdog
      │ ──► Execute Registered Self-Tests:
      │       1. Storage / NVS Read-Write Test
      │       2. Peripheral Bus / Sensor Test
      │       3. Network Transport Gateway Check
      ├──► All Passed: Call markAppValid() (Commit permanent boot flag)
      └──► Any Failed:
              1. Persist failure context to "ota_rollback"
              2. Call markAppInvalid() (Reboot immediately to Bank A)
              3. Bank A boots -> processPendingRollbackTelemetry() dispatches failure
```

---

## 7. Solved Critical Failure Modes & Implementation Invariants

### 1. FreeRTOS Task Lifecycle Panic (Store Access Fault)
* **Failure:** Task functions running the update routine exit via `return;`. C++ stack unwinding calls destructors on active objects (e.g. `cpx_credentialsManager`), invoking `vTaskDelete()` on a terminated task handle, causing `uxListRemove` kernel panics.
* **Invariant:** Worker tasks must never return from their top-level function. Unrecoverable states must block inside a sleep loop (`for (;;) { vTaskDelay(pdMS_TO_TICKS(5000)); }`) or suspend explicitly.

### 2. NVS Type System Fallback (String vs. Blob)
* **Failure:** Searching for a key stored as a string (`file,string` in CSV) via `nvs_get_blob()` returns `ESP_ERR_NVS_NOT_FOUND` (`0x1102`), **not** `ESP_ERR_NVS_TYPE_MISMATCH`. Handlers checking only for type mismatch fail to fall back to `nvs_get_str()`.
* **Invariant:** Storage adapters must fall back to string readout whenever binary blob readout returns non-success:
  ```cpp
  err = nvs_get_blob(handle, key, data, &reqLen);
  if (err != ESP_OK) {
      reqLen = length;
      err = nvs_get_str(handle, key, reinterpret_cast<char*>(data), &reqLen);
  }
  ```

### 3. Destruction of Plaintext Flash via Secure Mounting
* **Failure:** Initializing an unencrypted or miskeyed partition using secure encryption APIs (`nvs_flash_secure_init_partition()`) causes the AES-XTS engine to decrypt invalid bytes. The driver detects CRC failure and auto-erases the physical partition bitmap (`0xAA` $\to$ `0x00`).
* **Invariant:** Unencrypted storage partitions must mount via `nvs_flash_init_partition()`. Encrypted partitions must ensure the encryption key on flash exactly matches the host image key before initialization.

### 4. HTTP Client URL Path Wipe
* **Failure:** Transitioning from the `/check` route to the `/download` route via `esp_http_client_set_url()` clears pre-configured port, host, and TLS certificates.
* **Invariant:** Relative path transitions must be updated via `esp_http_client_set_path()`.

### 5. Recursive Mutex Protection
* **Failure:** High-level APIs (`checkForUpdates()`) calling internal telemetry methods (`processPendingRollbackTelemetry()`) using standard `std::mutex` trigger self-deadlocks.
* **Invariant:** Subsystem synchronization must use `std::recursive_mutex`.

### 6. Dynamic Flash Geometry Inversion
* **Failure:** Hardcoding flash sector offsets (e.g., `0x36000`, `0x39000`) breaks when partition tables adjust for MMU alignment boundaries (e.g. 64 KB app alignment) or when intermediate partitions are removed.
* **Invariant:** Flash tools must dynamically resolve partition offsets from `partitions.csv` using hardware layout constraints passed from SSoT configuration files.

---

## 8. Platform Adaptation Layer (PAL/HAL) Integration Reference

To adapt this framework to a new silicon target or RTOS, implement the concrete bridges below:

```
┌────────────────────────────────────────────────────────────────────────┐
│                              IOtaManager                               │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
       ┌────────────────────────────┼────────────────────────────┐
       ▼                            ▼                            ▼
┌──────────────┐             ┌──────────────┐             ┌──────────────┐
│ IOtaService  │             │   IHal_Mem   │             │ IHal_Mem_Ota │
└──────┬───────┘             └──────┬───────┘             └──────┬───────┘
       │ (Platform Adapter)         │ (Platform Adapter)         │ (Platform Adapter)
       ▼                            ▼                            ▼
┌──────────────┐             ┌──────────────┐             ┌──────────────┐
│  ESP-IDF /   │             │  NVS Driver  │             │ esp_ota_ops  │
│ FreeRTOS /   │             │ Flash Driver │             │ Bank Selector│
│  MbedTLS     │             │ EEPROM Store │             │ Image Verify │
└──────────────┘             └──────────────┘             └──────────────┘
```

1. **Memory Bank Adapter (`IHal_Mem_Ota`):** Map methods `begin()`, `write()`, `resume()`, `end()`, `setBootPartition()`, `getRunningImageState()`, `markAppValid()`, and `markAppInvalid()` to native bootloader APIs.
2. **Network Transport Adapter (`IHttpClient`):** Wrap platform socket APIs to execute HTTP GET/POST with range chunking and custom TLS context injection.
3. **Cryptographic Adapter (`ICryptoEngine`):** Wrap platform cryptography libraries to execute progressive SHA-256 (`init`, `update`, `finish`), X.509 certificate parsing, and raw ECDSA signature verification.
4. **Storage Adapter (`IHal_Mem`):** Implement raw key-value or sector read/write/erase for Root CAs, 4 KB checkpoints, and diagnostic records.
5. **Watchdog Adapter (`IHal_Sys_Wdt`):** Bind `feed()` and `yield()` to RTOS task scheduler and hardware timer watchdogs.
```


```markdown
## 9. Operational CLI Task Reference (OTA Workflow)

### Initial Baseline Provisioning (Factory Setup)

Executed once per device to establish the hardware baseline, partition table, and Root CA trust anchors.

| Order | Task                          | Command                                       | Purpose                                                                                                                                                         |
| ----- | ----------------------------- | --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **1** | **Generate SSoT Artifacts**   | `inv es.build-config.all`                     | Compiles `partitions.csv`, `sdkconfig.hardware`, and `ota_generated_config.h` from `configs/config_project.yaml`.                                               |
| **2** | **Compile Baseline Firmware** | `inv es.platform.build`                       | Compiles `bootloader.bin`, `partition-table.bin`, and baseline application (`ota_0`).                                                                           |
| **3** | **Flash Base Firmware**       | `inv es.platform.flash`                       | Flashes bootloader, partition table, and application (`ota_0`) to offsets dynamically resolved from `partitions.csv`.                                         |
| **4** | **Provision Factory Storage** | `inv es.crypto.provision-nvs`                 | Renders CSV, generates `nvs_keys.bin`, and creates encrypted `fctry` containing Root CA.                                                                        |
| **5** | **Flash Factory Storage**     | `inv es.crypto.flash-nvs`                     | Flashes `nvs_keys.bin` and `nvs_encrypted.bin` to partition offsets dynamically resolved from `partitions.csv`.                                                |

---

### OTA Release & Deployment Lifecycle (Publishing Updates)

Executed each time `project.version` is incremented in `configs/config_project.yaml`.

| Order | Task                               | Command                                       | Purpose                                                                                                                                                         |
| ----- | ---------------------------------- | --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **1** | **Regenerate SSoT Headers**        | `inv es.build-config.all`                     | Updates version macros and gateway endpoints in `ota_generated_config.h`.                                                                                       |
| **2** | **Package & Sign Release**         | `inv es.ota.release`                          | Compiles new binary, computes SHA-256 digest, signs with developer key, generates 1-hop delta patch (`diff_worker.py`), and registers entry in `manifest.json`. |
| **3** | **Sync Windows Host** *(Optional)* | `inv es.util.sync-ota-windows`                | Mirrors `release-catalog/` and TLS certificates from WSL to Windows host if the server runs natively on Windows.                                                |
| **4** | **Start Delivery Server**          | `inv es.ota.server`                           | Boots the FastAPI distribution daemon serving `/check`, `/download`, and `/status`.                                                                             |
| **5** | **Monitor Client Update**          | `inv es.platform.monitor --opts="--no-reset"` | Attaches serial console without asserting DTR/RTS reset to observe live update and provisional trial boot.                                                      |

---

### Single Chained Commands

#### Baseline Setup One-Liner
```bash
inv es.build-config.all && inv es.platform.build && inv es.platform.flash && inv es.crypto.provision-nvs && inv es.crypto.flash-nvs && inv es.platform.monitor

#### Release Deployment One-Liner
```bash
inv es.build-config.all && inv es.ota.release && inv es.util.sync-ota-windows
```
```
