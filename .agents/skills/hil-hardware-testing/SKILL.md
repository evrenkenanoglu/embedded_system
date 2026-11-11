---
name: hil-hardware-testing
description: >-
  Authoritative specification for production Hardware-in-the-Loop (HIL) testing.
  Enforces clean two-tier test separation (generic platform-agnostic HAL/PAL regression
  in embedded_system vs. board/application integration in CI_CD), on-target orchestration
via Proc_Hil_Test, host execution via the pyserial hil_capture fixture, fail-safe
  hardware quarantine, and zero test framework dependencies in firmware.
version: 4.1.0
target_runtime: Python 3.13 / pytest-embedded / ESP-IDF v5.4.1 / FreeRTOS
target_silicon: ESP32-C6 (ESP32-C6-WROOM-1)
---

# Skill: Production Hardware-in-the-Loop (HIL) Testing Engine

Use this skill when designing, authoring, reviewing, or running Hardware-in-the-Loop (HIL) test suites, on-target test processes, or CI/CD test automation.

---

## 1. Architectural Philosophy: The Two-Tier Separation

To keep `embedded_system` completely reusable across different microcontrollers (ESP32, STM32, nRF52, etc.) while thoroughly testing product-specific board hardware, HIL is divided into two decoupled tiers:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        Master SSoT & Task CLI                          │
│               (configs/config_project.yaml, CI_CD/config_hil.yaml)     │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
         ┌──────────────────────────┴──────────────────────────┐
         ▼                                                     ▼
┌─────────────────────────────────────┐       ┌─────────────────────────────────────┐
│ TIER 1: Generic HAL/PAL Regression  │       │ TIER 2: Board & Application HIL     │
│ Location: embedded_system/Tests/hil/│       │ Location: main/ & CI_CD/tests/hil/  │
├─────────────────────────────────────┤       ├─────────────────────────────────────┤
│ • 100% Platform & Board Agnostic    │       │ • Target Process: main/Proc_Hil_Test│
│ • Pure C++ functions returning      │       │   (Binds main/hwConfig/ to generic  │
│   sys_error_t                       │       │   tests and emits [HIL_REPORT] logs)│
│ • Zero test framework dependencies  │       │ • Host Runner: CI_CD/tests/hil/     │
│   (NO unity.h, NO ceedling, NO JSON)│       │   (pytest-embedded drives serial,   │
│ • Zero board pin assumptions        │       │   validates reports, tests Matter,  │
│ • Zero test code in Source/         │       │   relays, thermal safety, and OTA)  │
└─────────────────────────────────────┘       └─────────────────────────────────────┘
```

### Absolute Layering Invariants
1. ❌ **ZERO Test Files in `Source/`:** `embedded_system/Source/` contains **strictly production firmware**. All generic HAL/PAL test functions reside in `embedded_system/Tests/hil/cases/`.
2. ❌ **ZERO Framework Headers in Firmware HIL:** Generic test cases must **never** include `unity.h`, Ceedling headers, or custom JSON-RPC UART parsers. They are pure C++ functions returning `sys_error_t`.
3. ❌ **ZERO Board Pins in `embedded_system`:** `embedded_system/` must never contain pin numbers, `sdkconfig`, or board layouts. Hardware instances are injected into test functions by reference.
4. ✔️ **Board Ownership in `main/`:** `main/Proc_Hil_Test.cpp` pulls pin configurations from `main/hwConfig/`, instantiates drivers, calls generic test functions, and formats test report strings over UART.
5. ✔️ **Host Assertions in `CI_CD/tests/hil/`:** Test assertions execute on the host PC. The `hil_capture` fixture reads the serial stream; fixtures and module-level skips gate dependent suites.
6. ✔️ **Security Gating:** `Proc_Hil_Test.cpp` must be compiled **only** when HIL testing is enabled (`-DENABLE_HIL_TEST_PROCESS=ON`). It must never be included in production release binaries.

---

## 2. Directory Layout & File Ownership

```text
Embedded_IoT_BT_WIFI_Base_Project
├── CI_CD/
│   ├── config_hil.yaml                       # SSoT bench settings (ports, baudrates, loopbacks)
│   └── tests/
│       └── hil/                              # Host-side HIL test engine (pytest)
│           ├── conftest.py                   # hil_capture fixture, case discovery, profiles
│           ├── suites/
│           │   ├── test_00_telemetry_contract.py  # @TLM framing, CRC, id table
│           │   ├── test_01_hal_regression.py      # Generic HAL cases from [HIL_REPORT]
│           │   ├── test_02_board_relays.py        # Placeholder skip until relay tests exist
│           │   ├── test_03_board_thermal.py       # Placeholder skip; NTC not implemented
│           │   ├── test_crypto_security.py        # Signature, x509, partition flags
│           │   ├── test_fleet_operations.py       # Host OTA fleet logic
│           │   ├── test_ota_resiliency.py         # Host OTA checkpoint logic
│           │   ├── test_matter_hil.py             # Application firmware (skip)
│           │   ├── test_serial.py                 # Application firmware (skip)
│           │   └── test_passive_monitoring.py     # Application firmware (skip)
│           └── requirements.txt              # pytest, pytest-embedded, pytest-html
│
├── embedded_system/                          # Reusable Platform-Agnostic Middleware
│   ├── Source/                               # PURE PRODUCTION CODE (Zero test files here!)
│   └── Tests/
│       └── hil/
│           └── cases/                        # Generic HAL/PAL Regression Contract Functions
│               ├── hil_test_sys_rng.hpp/.cpp
│               ├── hil_test_sys_rtc.hpp/.cpp
│               ├── hil_test_sys_resetReason.hpp/.cpp
│               ├── hil_test_com_i2c.hpp/.cpp
│               ├── hil_test_io_gpio.hpp/.cpp
│               └── hil_test_mem_nvs.hpp/.cpp
│
└── main/                                     # Target Product / Board Implementation
    ├── hwConfig/                             # Concrete Board Pins (i2c.hpp, pins.hpp, switches.hpp)
    ├── Proc_Hil_Test.hpp                     # Board HIL Test Process Interface
    ├── Proc_Hil_Test.cpp                     # Binds hwConfig to Generic Tests & emits reports
    ├── main.cpp                              # Conditional process mounting (Production vs HIL)
    └── CMakeLists.txt
```

---

## 3. Tier 1: Generic HAL/PAL Regression Contract (`embedded_system/Tests/hil/cases/`)

Every generic test is a self-contained C++ function returning `sys_error_t`. It tests abstract `IHAL_*` interface compliance using dependency injection and repository error macros.

### Implementation Rules:
* Single-line function signatures in header and implementation.
* Check each call explicitly with `if (err != ERROR_SUCCESS) { return err; }`. The HIL cases do not use `RETURN_IF_ERROR` (0 uses in `Tests/hil/cases/`), so each failure returns its own code to the report line.
* Mismatches return the contract error (`ERROR_FAIL`, `ERROR_INVALID_ARG`, `ERROR_NULL_POINTER`).
* Allman braces and 80-column section separator blocks.
* Procedural steps documented with `///`; private notes use `//`.

### Canonical Example: `hil_test_sys_rng.hpp` / `.cpp`
```cpp
/** @file       hil_test_sys_rng.hpp
 *  @brief      Platform-agnostic regression test contract for IHal_Sys_Rng.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Rng.h"

sys_error_t hil_test_sys_rng_contract(IHal_Sys_Rng& rng);
```

```cpp
/** @file       hil_test_sys_rng.cpp
 *  @brief      Platform-agnostic regression test contract for IHal_Sys_Rng.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_sys_rng.hpp"
#include "System/error_macros.h"

/** FUNCTIONS *****************************************************************/
sys_error_t hil_test_sys_rng_contract(IHal_Sys_Rng& rng)
{
    sys_error_t err = rng.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    uint8_t buffer[16] = {0};

    /// Reject a null output pointer and a zero-length request
    err = rng.getRandomBytes(nullptr, sizeof(buffer));
    if (err != ERROR_NULL_POINTER)
    {
        return ERROR_FAIL;
    }

    err = rng.getRandomBytes(buffer, 0U);
    if (err != ERROR_INVALID_ARG)
    {
        return ERROR_FAIL;
    }

    /// ... remaining checks: non-zero output and range bounds (see the file)
    return ERROR_SUCCESS;
}
```

---

## 4. Tier 2: Target Board Process Orchestrator (`main/Proc_Hil_Test`)

`Proc_Hil_Test` implements `IProcess`. It bridges the concrete board hardware (`main/hwConfig/`) to the generic test routines and streams standardized test reports over UART.

### Reporting Wire Protocol:
Output lines are standard ASCII, easily parsed by Pytest:
```text
[HIL_REPORT] BEGIN_HAL_REGRESSION
[HIL_REPORT] TEST: sys_rng_contract -> PASS
[HIL_REPORT] TEST: com_i2c_contract -> PASS
[HIL_REPORT] TEST: io_gpio_contract -> SKIP (no wired sense pin on this board)
[HIL_REPORT] END_HAL_REGRESSION: <n> PASSED, <m> FAILED
```
Reports are emitted with `SYS_LOG_I` / `SYS_LOG_E`. The host matches the `[HIL_REPORT]` text, so the ESP-IDF log prefix does not matter. A setup failure emits `[HIL_REPORT] ABORT: ...` instead.

### Canonical Implementation Pattern (`main/application/Proc_Hil_Test.cpp`):
```cpp
void Proc_Hil_Test::_runBattery()
{
    vTaskDelay(pdMS_TO_TICKS(kSettleDelayMs));
    _passCount = 0U;
    _failCount = 0U;

    /// Telemetry subscribes before any result is produced so the host receives the full STATE snapshot.
    /// On setup failure, emit ABORT and return (ScopeGuards undo partial setup).
    SYS_LOG_I("[HIL_REPORT] BEGIN_HAL_REGRESSION");

    {
        sys_rng rng;
        _reportResult("sys_rng_contract", hil_test_sys_rng_contract(rng));
    }

    {
        com_i2c bus(HW::I2C::i2cConfig);
        _reportResult("com_i2c_contract", hil_test_com_i2c_contract(bus, kI2cExpectedAddress));
    }

    /// io_gpio is reported as SKIP until a second GPIO is wired to the drive pin.
    SYS_LOG_I("[HIL_REPORT] TEST: io_gpio_contract -> SKIP (no wired sense pin on this board)");

    {
        mem_nvs nvs("hil_scratch");
        _reportResult("mem_nvs_contract", hil_test_mem_nvs_contract(nvs));
    }

    SYS_LOG_I("[HIL_REPORT] END_HAL_REGRESSION: %lu PASSED, %lu FAILED", static_cast<unsigned long>(_passCount), static_cast<unsigned long>(_failCount));
}
```
Adding a case means one new `hil_test_*` file in `embedded_system/Tests/hil/cases/` plus one `_reportResult` block here. Keep the test names `<area>_<unit>_contract` so the host regex matches.

---

## 5. Host Pytest Architecture & Dependency Gating (`CI_CD/tests/hil/`)

Host tests read the target through the `hil_capture` session fixture in `CI_CD/tests/hil/conftest.py`, not `pytest-embedded`'s `dut`. The fixture opens the port with DTR and RTS low so opening it does not reset the board, reads until `END_HAL_REGRESSION`, and pulses reset only if the boot marker is still absent after 8 s. `pytest-dependency` is not installed, so `@pytest.mark.dependency` is inert; order work with fixtures and module-level skips instead.

### Harness rules
1. **Cases come from firmware.** `discover_hal_cases()` parses `Tests/hil/cases/hil_test_*.cpp`, and `test_hal_case` is parametrized per case. A new firmware case gets a host test automatically; never hard-code case names.
2. **Image modes.** `--hil-image={hil,app}` (default `hil`) selects the firmware image under test. Each suite that needs one image carries `@pytest.mark.image("hil" | "app")`; `pytest_collection_modifyitems` skips the other image's items with a reason. Unmarked tests run in both images. `test_00_telemetry_contract.py::test_image_matches_selection` fails if the board boots a different image than the one selected.
3. **No eFuse operations.** No test reads or burns eFuses, and there is no production profile. Do not reintroduce `espefuse` calls, `--hil-profile`, or `production_profile` markers.
4. **Signature freshness.** `test_signature_tamper_detection` checks `build/*.bin` against `build/provisioning/target_signature.sig`. Run `inv es.crypto.provision-sign` after every firmware build, before the HIL run. The sign step is host-only (hash, sign, manifest catalog).
5. **Every skip states its reason.** Silent skips are not allowed.
6. **Collection errors fail the stage.** `CI_CD/config_hil.yaml` runs the whole `CI_CD/tests/hil` directory, so one broken import blocks every suite.

### Suite map
| File | Scope |
|------|-------|
| `test_00_telemetry_contract.py` | `@TLM` framing, CRC, and ID table on hardware |
| `test_01_hal_regression.py` | Generic HAL cases; `io_gpio` skips without a jumpered sense pin |
| `test_crypto_security.py` | Host-side: signature structure and tamper detection (needs `inv es.crypto.provision-sign`), x509 chain, partition flags |
| `test_fleet_operations.py`, `test_ota_resiliency.py` | Host OTA server tests (any image) |
| `test_passive_monitoring.py` | `image("app")`: `@TLM` boot and Wi-Fi terminal-state checks |
| `test_02_board_relays.py`, `test_03_board_thermal.py` | `image("app")` placeholders: skip with a reason until relay and NTC support exist |
| `test_matter_hil.py` | `image("app")` placeholder: skip until Matter state IDs are published |

```bash
mkdir -p Out/test_reports && .venv/bin/python -m pytest CI_CD/tests/hil --html=Out/test_reports/hil_report.html --self-contained-html --port=/dev/ttyACM0 --baud=115200 -v -rs -p no:cacheprovider -W ignore
```
Reports go to `{paths.output_dir}/test_reports/` (`CI_CD/config_hil.yaml`). Do not use `-W ignore` in verification runs; run `--strict-markers --collect-only` first so marker errors are not hidden.

---

## 5A. Serial Telemetry & Automatic Log Extraction (`embedded_system/Tests/hil/telemetry/`)

Host-side extraction of device state, faults, and logs from the serial stream. It is application-agnostic: firmware emits a fixed wire format, and the host decodes it with no per-application parser.

### Tiers
| Tier | Source on device | Host decoding |
|------|------------------|---------------|
| T0 | Every raw line | `RawLine` (host capture time, text) |
| T1 | ESP-IDF `L (ms) TAG: msg` and `[HIL_REPORT]` lines | `LogLine`, `HilReportLine` |
| T2 | `@TLM v=1 ...` records from `TelemetrySerialSubscriber` (notification bus) | `TelemetryRecord`, `DropRecord` (CRC-verified) |

### Wire Format v1 (single line, at most 120 bytes)
```text
@TLM v=1 seq=42 ms=10432 kind=STATE id=0x21 cat=0x02 sev=W act=1 pl=0x00000000 crc=0x2E2E
@TLM v=1 kind=DROP n=7
```
* `crc` is CRC-16/CCITT-FALSE over the bytes before ` crc=`. Check value for `"123456789"` is `0x29B1`. Firmware reuses `Crc16` with `Profile::CcittFalse()` from `Source/Library/Common/Crc.hpp`.
* `DROP` records carry no CRC. They report records the firmware could not write.
* `STATE` records replay to new subscribers, so a boot snapshot is available after the subscriber attaches.

### Rules
1. **Single emission point:** all `@TLM` output goes through `TelemetrySerialSubscriber` using one `printf` per record. Do not print `@TLM` from other code.
2. **Subscriber constraints:** subscribers run in the dispatcher task and must not subscribe, unsubscribe, or publish (`INotificationSubscriber.hpp`).
3. **Gated build:** the emitter, and `ENABLE_SERIAL_TELEMETRY`, exist only with `ENABLE_HIL_TEST_PROCESS=ON`. Production binaries do not include them.
4. **Parser ownership:** the host parser is the only consumer of the wire format. It counts `corrupt_crc`, `malformed`, `oversize`, `dropped`, `gaps`, `lost`, `out_of_order`, and `unknown_ids`. It never assumes lines are complete; use `feed_chunk` on raw serial bytes.
5. **Extending ids or kinds:** add a notification id in `Source/App/Notification/NotificationTypes.hpp`, then update `Tests/hil/telemetry/ids.py`. `test_id_table_matches_notification_types_header` must pass. Bump `v=` for any incompatible wire change.
6. **Serial ownership:** capture with `es.platform.monitor --opts="--no-reset"` (ERR-006). Only one process may own the port.
7. **Authorization:** telemetry capture on hardware is a HIL run. Do not flash or capture without explicit authorization.

### Validation
```bash
.venv/bin/python -m pytest embedded_system/Tests/hil/telemetry -q
```

---

## 5B. Naming, Structure & Style Standards (`CI_CD/tests/hil/` and `Tests/hil/`)

1. **Files:** `test_<NN>_<topic>.py` with a two-digit ordering prefix, where the topic names the behavior (`test_01_hal_regression.py`). Legacy topic-only names (`test_serial.py`) are allowed only for application-firmware suites that are module-skipped.
2. **Tests:** `test_<behavior>` in `snake_case`. Parametrized HAL cases get their id from the firmware case name (`hil_test_<area>_<unit>` → `<area>_<unit>_contract`).
3. **Firmware cases:** `hil_test_<area>_<unit>.{hpp,cpp}`, with the function `hil_test_<area>_<unit>_contract`. `discover_hal_cases()` relies on this exact pattern.
4. **Header block:** every Python suite and C++ file starts with `@file`, `@brief`, and `@copyright` (`(c) 2026- Evren Kenanoglu - All Rights Reserved`).
5. **Skip reasons:** every `pytest.skip` and `pytest.mark.skip` has a `reason=` that names what is missing.
6. **Registered markers only:** `target`, `env` (from pytest-embedded), and `production_profile` (from `conftest.py`). Strict collection must pass.
7. **Pinned dependencies:** `CI_CD/tests/hil/requirements.txt` lists only what the suites import. Do not list unused plugins.
8. **Placeholders:** a placeholder is a skip-only test with a clear reason and no fixtures. No pointer stubs and no duplicate test names across files.
9. **Ownership:** host assertions in `CI_CD/tests/hil/`; firmware-side cases in `embedded_system/Tests/hil/cases/`; board binding in `main/application/Proc_Hil_Test.cpp`.

---

## 6. Hardware Safety, Quarantine & Flash Isolation

1. **Relay Fail-Safe De-Energization:** Any test interacting with physical relays must guarantee that output pins are forced `LOW` (contacts open / load de-energized) on any failure path or abort.
2. **Ephemeral NVS Namespace (`hil_scratch`):** HIL tests are strictly forbidden from writing to production `fctry` or `nvs_keys` partitions. Storage tests operate exclusively in `hil_scratch`; `hil_test_mem_nvs` erases its own probe key.
3. **No Task Watchdog Feeding (not implemented):** `Proc_Hil_Test` does not configure or feed a task watchdog. A hung transaction is bounded on the host by `CAPTURE_DEADLINE_S` in `conftest.py`, and the run fails with the missing `END_HAL_REGRESSION` line. Do not claim WDT recovery until it exists.
4. **Panic Trace Decoding (manual):** `pytest-embedded-idf` is not installed. On a panic, decode the backtrace against the `build/` ELF with `addr2line` or the ESP-IDF monitor decoder.

---

## 7. CLI Task Invocations (`embedded_system/Tasks/ci_cd.py`)

All HIL operations are driven via PyInvoke CLI tasks:

```bash
# 1. Pipeline stage (dry run first; a real run flashes the board)
inv es.ci-cd.hil --dry-run

# 2. Build the image under test. Pass the define explicitly every time: build/CMakeCache.txt
#    persists the last value, so a bare build can silently keep the other image.
inv es.platform.build --defines="ENABLE_HIL_TEST_PROCESS=ON"    # --hil-image=hil
inv es.platform.build --defines="ENABLE_HIL_TEST_PROCESS=OFF"   # --hil-image=app

# 3. Re-sign the freshly built binary (required by the signature test)
inv es.crypto.provision-sign

# 4. Full flash on the first run of each image (developer bench, user decision). Never
#    burn eFuses, and never flash a production profile, from HIL tooling.
inv es.platform.flash --port=/dev/ttyACM0

# 5. Collect-only with strict markers (no hardware needed)
.venv/bin/python -m pytest CI_CD/tests/hil --collect-only -q --strict-markers

# 6. Full directory run with HTML report (the board must be on /dev/ttyACM0)
mkdir -p Out/test_reports && .venv/bin/python -m pytest CI_CD/tests/hil --hil-image=hil --html=Out/test_reports/hil_hil_report.html --self-contained-html --port=/dev/ttyACM0 --baud=115200 -v -rs -p no:cacheprovider --strict-markers
.venv/bin/python -m pytest CI_CD/tests/hil --hil-image=app --html=Out/test_reports/hil_app_report.html --self-contained-html --port=/dev/ttyACM0 --baud=115200 -v -rs -p no:cacheprovider --strict-markers
```

Verified on hardware (ESP32-C6, unprovisioned dev board, full flash, signed before each run): `hil` 27 passed / 6 skipped; `app` 20 passed / 13 skipped; both exit 0.

`mem_ota_slots_contract` (in `embedded_system/Tests/hil/cases/`) is the `ota_1` coverage. It runs read-only through `IHal_Mem_Partition` and checks that both OTA slots resolve, are 64 KB aligned, and do not overlap. It also checks that at least one slot holds an image (`0xE9`) and that any other slot is erased. Partition-table reads from host flash are ciphertext, so this check runs on the device.

---

## 8. Pre-Execution Quality Checklist

Before running or committing HIL test changes:
- [ ] **Directory Purity:** Confirmed that **zero** test files exist in `embedded_system/Source/`.
- [ ] **Zero Test Frameworks in Firmware:** Confirmed zero `#include "unity.h"` or custom JSON RPC parsers in firmware source code.
- [ ] **Interface Contracts:** Generic tests in `embedded_system/Tests/hil/cases/` take `IHal_*` references and return `sys_error_t`.
- [ ] **Board Decoupling:** Generic test cases contain no pin numbers or `main/hwConfig/` references.
- [ ] **Image Gating:** `Proc_Hil_Test.cpp` is gated by `ENABLE_HIL_TEST_PROCESS` and excluded from release firmware. Each image-specific suite has an `image` marker.
- [ ] **No eFuse Operations:** `grep -rniE "espefuse|efuse|production_profile" CI_CD/tests/hil` finds only comments.
- [ ] **Signature Fresh:** `inv es.crypto.provision-sign` ran after the last build.
- [ ] **Gating, Not Dependency Markers:** Suites gate via module-level skips or fixtures with a reason. `@pytest.mark.dependency` is not used because `pytest-dependency` is not installed.
- [ ] **Strict Collection:** `pytest CI_CD/tests/hil --collect-only -q --strict-markers` reports zero errors.
- [ ] **Storage Safety:** Flash tests operate strictly on `hil_scratch` and never write to the manufacturing `fctry` partition.
- [ ] **Reports:** Full runs use `--html=Out/test_reports/hil_report.html`; the console log is kept alongside it.

---

## 9. Error Registry (Post-Mortem Lessons & Preventive Rules)

Scoped to `hil-hardware-testing`:

- **ERR-001 — Strict-marker failure hidden by warning filter:**
  *Symptom:* The HIL collection looked clean in a default run, but `--strict-markers` failed with `'dependency' not registered`.
  *Root Cause:* `@pytest.mark.dependency` referenced a plugin that is not installed; `-W ignore` hid the unknown-marker warning.
  *Preventive Rule:* Remove markers for unavailable plugins and run `--collect-only --strict-markers` before every HIL run.

- **ERR-002 — Fixture `dut` missing from the HIL harness:**
  *Symptom:* Suites requesting `dut` errored at setup on the board.
  *Root Cause:* The `hil_capture` harness reads serial directly and does not provide `dut`.
  *Preventive Rule:* Request only fixtures the harness defines (`hil_capture`, `hil_pinout`). Application suites that still use `dut` must stay module-skipped until they are ported.

- **ERR-003 — Production-only checks failing on dev boards:**
  *Symptom:* eFuse and secure-boot checks failed on an unprovisioned development board.
  *Root Cause:* Provisioned-unit assertions ran unconditionally.
  *Preventive Rule:* No eFuse-touching tests exist, and the production profile was removed by request. Never burn eFuses to make a dev board pass.

- **ERR-005 — Signature test failed after a rebuild:**
  *Symptom:* `test_signature_tamper_detection` raised `InvalidSignature` on the app image after a rebuild.
  *Root Cause:* `target_signature.sig` was made for the previous binary; the test checks the current `build/*.bin`.
  *Preventive Rule:* Re-run `inv es.crypto.provision-sign` after every build, before flashing. Do not edit the test to hide a stale signature.

- **ERR-006 — Telemetry compiled out of the application image:**
  *Symptom:* The app image booted but emitted no `@TLM` records; `test_app_boot_and_ready_states` had no data.
  *Root Cause:* `ENABLE_SERIAL_TELEMETRY` was defined only inside the HIL branch of `main/CMakeLists.txt`, so `TelemetrySerialSubscriber` was not built for the app image.
  *Preventive Rule:* Gate telemetry on its own option (default ON) and set compile definitions with `target_compile_definitions` after `idf_component_register`. Do not use `add_compile_definitions` there; it fails in the IDF requirements pass.

- **ERR-004 — Duplicate suites from pointer stubs:**
  *Symptom:* Two files (`test_04_matter_hil.py`, `test_05_ota_pipeline.py`) duplicated the real suites and reported the same tests twice.
  *Root Cause:* Stub files were kept as numbered placeholders during a refactor.
  *Preventive Rule:* One suite per topic. Delete pointer stubs instead of keeping them; a placeholder needs its own skip reason and no duplicate tests.
