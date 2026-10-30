# SYSTEM STRUCTURE CONTEXT: `Embedded_IoT_BT_WIFI_Base_Project`

This document defines the structural architecture, module relationships, execution flows, and system context of the **Embedded IoT Smart Plug Platform** (ESP32-C6 target). It serves as the baseline context map for understanding how all firmware layers, configuration nodes, and host tooling interact.

---

## 1. System Overview & Technology Stack

The project is an enterprise-grade IoT firmware and operational ecosystem implementing a smart power switch node with dual-connectivity (Matter over Wi-Fi/BLE), dual-phase resilient OTA, secure provisioning, and local embedded HTTP configuration.

* **Target Silicon:** Espressif ESP32-C6 (ESP32-C6-WROOM-1, 8MB Quad SPI NOR Flash).
* **Frameworks:** ESP-IDF v5.4.1 + ESP-Matter v1.4.2 (Connected Home IP / Matter 1.4).
* **Firmware Language:** C++17 / C++20 with vendor-independent abstract interface contracts.
* **Security Model:** Hardware Root of Trust (ECDSA secp256r1 + RSA-3072 Secure Boot v2, Flash Encryption XTS-AES-128, encrypted NVS factory partitions, and simulated virtual eFuse support).
* **Host Tooling:** Python 3.13, Invoke-based CLI task orchestration (`tasks.py`), Semgrep static analysis, and OpenSSL/cryptography PKI pipelines.

---

## 2. Repository Topography & Directory Layout

```text
Embedded_IoT_BT_WIFI_Base_Project/
├── configs/                     # Centralized SSoT configuration tree
├── certs/ & keys/               # Cryptographic PKI assets, root trust anchors, eFuse digests
├── partitions.csv               # 8MB physical flash memory layout
├── main/                        # Firmware entrypoint, app configurations, top-level processes
├── embedded_system/
│   ├── CMakeLists.txt           # Core CMake library definitions and toolchain links
│   ├── Source/                  # Layered C++ firmware codebase
│   │   ├── System/              # Cross-cutting error codes, logging, macros
│   │   ├── HAL/                 # Hardware Abstraction Layer (Interfaces & Platform drivers)
│   │   ├── PAL/                 # Platform Abstraction Layer (Protocols, Security engines)
│   │   ├── Process/             # Task life-cycle runners and cooperative event loops
│   │   ├── App/                 # Application domain managers (OTA, Wi-Fi, Time, Matter)
│   │   └── Library/             # Common utilities, embedded Web UI, converters
│   ├── Tools/                   # Automation tooling (PKI, OTA server, Provisioning, Code analysis)
│   └── Documentation/           # Architecture diagrams, sequence models, code conventions
├── CI_CD/                       # HIL hardware test suites and test runner pipelines
└── Tools/task-runner/           # Modular task automation and workspace environment resolver
```

---

## 3. Layered Firmware Architecture & Class Relationships

The firmware adheres to strict vertical decoupling. Communication traverses down via pure abstract interfaces and bubbles up via registered callbacks or event queues.

```text
┌────────────────────────────────────────────────────────────────────────┐
│ App Layer (WifiConfigurationManager, TimeManager, OtaManager, OnOff)   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ consumes
┌───────────────────────────────────▼────────────────────────────────────┐
│ Process Layer (Proc_App, Proc_TimeSync, Proc_PowerSwitchEventManager)  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ consumes
┌───────────────────────────────────▼────────────────────────────────────┐
│ PAL Layer (Serv_sntp, OtaService, MatterWrapper, HttpsClient, Crypto)  │
│  - Protocols/       : Pure protocol interfaces (ISntpClient, etc.)     │
│  - Platform/Esp32/  : Native lwIP, mbedTLS, and ESP-Matter bindings    │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ consumes
┌───────────────────────────────────▼────────────────────────────────────┐
│ HAL Layer (Hardware Abstraction Layer)                                 │
│  - IHAL/            : Pure C++ interfaces (IHal_Io_Gpio, IHal_Sys_Rtc)  │
│  - Platform/ESP32/  : Concrete silicon drivers (sys_rtc, cpx_wifi)     │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ calls
┌───────────────────────────────────▼────────────────────────────────────┐
│ ESP-IDF SDK & Silicon Hardware (FreeRTOS, Drivers, eFuse, Radio)       │
└────────────────────────────────────────────────────────────────────────┘
```

### Module Responsibilities:

* **`System/` (Core Primitives):**
  * `system.h`, `error_definitions.h`: Holds `sys_error_t` and `ERROR_SUCCESS` definitions.
  * `error_macros.h`: Provides guard macros (`RETURN_IF_ERROR`).
  * `LogHandler.h`: Centralizes `SYS_LOG_I`, `SYS_LOG_W`, `SYS_LOG_E`, `SYS_LOG_D`.
* **`HAL/IHAL/` (Abstract Silicon Interfaces):**
  * `IHal.h`: Base contracts (`IHAL_IO`, `IHAL_COM`, `IHAL_MEM`, `IHAL_CPX`, `IHAL_SYS`).
  * Specialized interfaces:
    * `IHal_Io_Gpio.h`: Pin configuration, state reads/writes, interrupt registration.
    * `IHal_Com_i2c.h`: Bus transactions, master/slave operation, timeouts.
    * `IHal_Cpx_Ble.h`: GAP advertising, connection events, Matter BTP transmission.
    * `IHal_Cpx_Wifi.h`: STA/AP modes, scanning, association callbacks, RSSI.
    * `IHal_Mem_Ota.h`: Flash partition read/write/resume, slot commits, verification.
    * `IHal_Sys_ResetReason.h`: Categorization of silicon reset causes.
    * `IHal_Sys_Rng.h`: Hardware True Random Number Generation.
    * `IHal_Sys_Rtc.h`: Epoch time, monotonic clock, timezone configuration.
    * `IHal_Sys_Wdt.h`: Task watchdog registration and cooperative yields.
* **`HAL/Platform/ESP32/` (Concrete Silicon Implementations):**
  * Bridges IHAL contracts to ESP-IDF subsystems (`esp_system`, `esp_random`, `esp_timer`, `nvs_flash`, `esp_wifi`).
* **`PAL/` (Platform Abstraction Layer):**
  * Decouples complex multi-step protocols from platform SDKs.
  * `SNTP/`: `ISntpClient.hpp` $\rightarrow$ `Serv_sntp.cpp` (lwIP SNTP integration with step/slew control).
  * `OTA/`: `IOtaService.hpp`, `IOtaTransport.hpp`, `OtaCheckpointManager.hpp` (segmented download, chunk SHA-256 calculation, checkpoint resumption).
  * `MATTER/`: Wraps ESP-Matter cluster management, fabric commissioning, endpoint setup.
  * `HTTP/`: Manages HTTPS client transactions and local embedded Web server URIs.
* **`Process/` (System Orchestration):**
  * Implements `IProcess`. Manages RTOS tasks, period tick execution, event handling, and peripheral supervisor lifecycles.
* **`App/` (Application Domain Managers):**
  * Implements state machines coordinating network, time, power relays, and Matter endpoints.

---

## 4. Single Source of Truth (SSoT) Configuration Structure

The project uses a structured inheritance hierarchy for all variables, driven from `configs/config_project.yaml`. Downstream configurations reference values using `{section.property}` interpolation syntax.

```text
configs/config_project.yaml (Master SSoT)
    │
    ├── configs/config_platform.yaml      --> Chip target, toolchains, serial/monitor ports
    ├── configs/config_pki.yaml           --> CA properties, cert validity, SANs, eFuse keys
    ├── configs/config_provisioning.yaml  --> Factory NVS CSV maps, firmware signing metadata
    ├── configs/config_ota_server.yaml    --> Server port, endpoints, deployment rules, rollback policy
    ├── configs/config_formatter.yaml     --> Clang-format, black, cmake-format inclusions
    └── configs/config_tasks.yaml         --> Task runner CLI namespaces and paths
```

### Key Configuration Nodes:
* **`project`**: Identity, semver string, integer version (`version_number` used for anti-rollback HSVN).
* **`target` / `hardware`**: Silicon model (`esp32c6`), baud rates, flash geometry (8MB DIO @ 80MHz).
* **`security`**: API keys, crypto curve specifications (`ec-secp256r1`), Secure Boot scheme (`rsa3072`), virtual eFuse simulation toggle.
* **`flash_layout`**: Partition offsets and sizes mirroring physical flash boundaries.
* **`ota`**: Rolling chunk sizes, telemetry endpoints, anti-rollback policies, NVS keys.
* **`time`**: Primary/secondary/tertiary NTP hosts, sync intervals, timezone string.
* **`ble`**: Matter commissioning discriminator, setup PIN code, advertising interval.

---

## 5. Physical Flash Layout (`partitions.csv`)

The 8MB physical NOR flash is allocated to support dual-slot execution, rollback protection, hardware encryption, and isolated factory keys:

| Name | Type | Subtype | Size | Functional Description |
| :--- | :--- | :--- | :--- | :--- |
| **`nvs`** | `data` | `nvs` | 128 KB (`0x20000`) | Dynamic operational storage (Wi-Fi credentials, runtime settings). |
| **`otadata`** | `data` | `ota` | 8 KB (`0x2000`) | ESP-IDF OTA boot slot selection and roll-back boot counters. |
| **`phy_init`** | `data` | `phy` | 4 KB (`0x1000`) | Silicon RF calibration parameters. |
| **`efuse_em`** | `data` | `efuse` | 8 KB (`0x2000`) | Virtual eFuse emulation block (active in simulation mode). |
| **`nvs_keys`** | `data` | `nvs_keys`| 4 KB (`0x1000`) | AES keys for decrypting the `fctry` partition. |
| **`fctry`** | `data` | `nvs` | 64 KB (`0x10000`) | Encrypted manufacturing storage (Root CA, certs, Device UUID, HSVN). |
| **`ota_0`** | `app` | `ota_0` | 3.5 MB (`0x380000`) | Primary executable firmware slot (MMU aligned: `0x10000`). |
| **`ota_1`** | `app` | `ota_1` | 3.5 MB (`0x380000`) | Secondary executable firmware slot (MMU aligned: `0x10000`). |

---

## 6. End-to-End System Execution & Data Flows

### A. Boot & Diagnostics Sequence
1. **Reset Reason Inspection:** Silicon powers up $\rightarrow$ `sys_resetReason` decodes register states. If an abnormal reset vector is detected (`PANIC_CRASH`, `TASK_WATCHDOG`, `BROWNOUT`), the diagnostic telemetry state is updated.
2. **Factory Credential Mounting:** `mem_nvs` initialises $\rightarrow$ reads `nvs_keys` $\rightarrow$ mounts encrypted `fctry` partition $\rightarrow$ loads device credentials and the PKI Root CA.
3. **OTA Image Verification:** If booting into a newly flashed slot, the image enters `OtaImageState::PendingVerify`. The system watchdog starts. App self-checks execute before `markAppValid()` is committed.

### B. Connectivity & Time Synchronization Flow
1. **Commissioning/Station Connect:** `cpx_wifi` or `cpx_ble` initializes. If uncommissioned, BLE Matter advertising launches (`0xFFF6` service). Once joined to Wi-Fi, BLE advertising stops to free the radio.
2. **SNTP Time Sync:** `Serv_sntp` queries primary/secondary/tertiary NTP servers $\rightarrow$ upon valid sample receipt, `sys_rtc` updates the POSIX wall clock and sets the timezone $\rightarrow$ marks `isTimeSet()` as valid.

### C. Resilient OTA Update Pipeline
```text
[OTA Server]                              [ESP32 Device]
     │                                           │
     ├── 1. Poll /api/v1/ota/check (with HSVN) ──►│
     │◄── 2. Manifest response (URL, SHA, Sig) ──┤
     │                                           ├── 3. Validate Manifest & Anti-Rollback HSVN
     │◄── 4. GET chunk requests (Range: bytes) ──┤
     ├── 5. 8KB Binary chunks ──────────────────►│── 6. Write to inactive slot via IHal_Mem_Ota
     │                                           ├── 7. Save byte-offset via OtaCheckpointManager
     │                                           ├── 8. Verify SHA-256 & ECDSA signature vs PKI
     │                                           └── 9. Set Boot Partition & Reboot into PendingVerify
```

---

## 7. Operational Tooling & Automation Subsystems

Located in `embedded_system/Tools/` and orchestrated by `tasks.py`:

* **`HW_SECURITY/provisioning`:** Generates binary key slots, creates encrypted `fctry` partition images, and interfaces with `espefuse.py` and `espsecure.py` for eFuse blowing or virtual simulation.
* **`PKI/`:** Generates Root CA, subordinate developer firmware signing certificates, and server TLS certificates using ECDSA secp256r1 keys.
* **`OTA/ota-server/`:** FastAPI-based local/cloud firmware distribution server with manifest catalog generation and canary deployment options.
* **`OTA/release-catalog/`:** Generates diff/delta binary patches and verifies target file hashes.
* **`CODE_ANALYSIS/STATIC_CODE_ANALYSIS/`:** Semgrep, MISRA, and CERT-C++ rule checking engine producing automated static analysis reports.
* **`FORMATTER/`:** Multi-language unified code beautifier covering C/C++ (`clang-format`), Python (`black`), CMake (`cmake-format`), and YAML.
* **`task-runner/`:** Python task execution engine exposing the `inv es.*` CLI task namespaces for building, flashing, provisioning, and testing.
