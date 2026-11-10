Here is the complete, updated **`embedded_system/.agents/skills/hil-hardware-testing/SKILL.md`** incorporating the two-tier architecture, pure C++ contract regression tests, the `Proc_Hil_Test` board process, and `pytest-embedded` host orchestration.

---

### File: `embedded_system/.agents/skills/hil-hardware-testing/SKILL.md`

```markdown
---
name: hil-hardware-testing
description: >-
  Authoritative specification for production Hardware-in-the-Loop (HIL) testing.
  Enforces clean two-tier test separation (generic platform-agnostic HAL/PAL regression
  in embedded_system vs. board/application integration in CI_CD), on-target orchestration
  via Proc_Hil_Test, host execution via pytest-embedded + pytest-dependency, fail-safe
  hardware quarantine, and zero test framework dependencies in firmware.
version: 4.0.0
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
5. ✔️ **Host Assertions in `CI_CD/tests/hil/`:** Test assertions and dependency pruning execute on the host PC via `pytest-embedded` [1, 2] and `pytest-dependency`.
6. ✔️ **Security Gating:** `Proc_Hil_Test.cpp` must be compiled **only** when HIL testing is enabled (`-DENABLE_HIL_TEST_PROCESS=ON`). It must never be included in production release binaries.

---

## 2. Directory Layout & File Ownership

```text
Embedded_IoT_BT_WIFI_Base_Project
├── CI_CD/
│   ├── config_hil.yaml                       # SSoT bench settings (ports, baudrates, loopbacks)
│   └── tests/
│       └── hil/                              # Host-Side HIL Test Engine (Pytest)
│           ├── conftest.py                   # Pytest fixtures & serial configuration
│           ├── suites/
│           │   ├── test_01_hal_regression.py # Asserts [HIL_REPORT] lines from Proc_Hil_Test
│           │   ├── test_02_board_relays.py   # Physical relay actuation & sensing
│           │   ├── test_03_board_thermal.py  # NTC temperature & emergency thermal cutoff
│           │   ├── test_04_matter_hil.py     # Matter commissioning & On/Off cluster
│           │   └── test_05_ota_pipeline.py   # Dual-bank OTA updates & rollback
│           └── requirements.txt              # pytest, pytest-embedded, pytest-dependency
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
* Error-predicate formatting: `RETURN_IF_ERROR((cond_is_error), errCode, ...)`.
* Allman braces and 80-column section separator blocks.
* Procedural function steps documented using `///` without `###` or `Step X:` noise. Private notes use `//`.

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
    /// Initialize TRNG driver instance
    RETURN_IF_ERROR(
        (rng.init() != ERROR_SUCCESS),                                // Expression
        ERROR_FAIL                                                    // Error Code
    );

    /// Sample consecutive 32-bit random words
    const uint32_t sampleA = rng.getRandomU32();
    const uint32_t sampleB = rng.getRandomU32();

    /// Assert samples are non-zero and non-identical
    RETURN_IF_ERROR(
        (sampleA == 0U && sampleB == 0U),                             // Expression
        ERROR_INVALID_DATA                                            // Error Code
    );
    RETURN_IF_ERROR(
        (sampleA == sampleB),                                         // Expression
        ERROR_INVALID_DATA                                            // Error Code
    );

    /// Verify range bounding [min, max]
    const uint32_t val = rng.getRandomRange(10U, 20U);
    RETURN_IF_ERROR(
        (val < 10U || val > 20U),                                     // Expression
        ERROR_OUT_OF_RANGE                                            // Error Code
    );

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
[HIL_REPORT] TEST: com_i2c_probe -> PASS
[HIL_REPORT] TEST: io_gpio_loopback -> PASS
[HIL_REPORT] END_HAL_REGRESSION: 3 PASSED, 0 FAILED
```

### Canonical Implementation Pattern:
```cpp
void Proc_Hil_Test::_runBattery()
{
    vTaskDelay(pdMS_TO_TICKS(500U));

    std::printf("[HIL_REPORT] BEGIN_HAL_REGRESSION\n");
    std::fflush(stdout);

    /// Execute Generic TRNG Contract Test
    {
        sys_rng rng;
        _reportResult("sys_rng_contract", hil_test_sys_rng_contract(rng));
    }

    /// Execute Generic I2C Probe Test with Board Pins and MCP23017 Address
    {
        com_i2c bus(hwConfig::I2C_PORT_0, hwConfig::PIN_I2C_SDA, hwConfig::PIN_I2C_SCL);
        _reportResult("com_i2c_probe", hil_test_com_i2c_contract(bus, 0x20U));
    }

    /// Execute Ephemeral Scratchpad NVS Test
    {
        mem_nvs scratchNvs("hil_scratch");
        _reportResult("mem_nvs_scratch", hil_test_mem_nvs_contract(scratchNvs));
    }

    std::printf("[HIL_REPORT] END_HAL_REGRESSION: %lu PASSED, %lu FAILED\n", _passCount, _failCount);
    std::fflush(stdout);
}
```

---

## 5. Host Pytest Architecture & Dependency Gating (`CI_CD/tests/hil/`)

Host testing uses **`pytest-embedded`** [1, 2] for serial capture, panic trace decoding [2, 3], and **`pytest-dependency`** for topological execution ordering.

### Stage 1: HAL Regression Suite (`test_01_hal_regression.py`)
```python
"""
@file       test_01_hal_regression.py
@brief      Pytest verifying target-side generic HAL regression test results.
"""

import pytest


@pytest.mark.target("esp32c6")
@pytest.mark.env("hil")
@pytest.mark.dependency(name="hal_begin")
def test_hal_regression_stream_start(dut):
    """Assert target firmware booted and began HAL regression battery."""
    dut.expect(r"\[HIL_REPORT\] BEGIN_HAL_REGRESSION", timeout=10)


@pytest.mark.target("esp32c6")
@pytest.mark.env("hil")
@pytest.mark.dependency(name="hal_rng", depends=["hal_begin"])
def test_generic_sys_rng(dut):
    """Verify hardware TRNG entropy check passed."""
    dut.expect(r"\[HIL_REPORT\] TEST: sys_rng_contract -> PASS", timeout=3)


@pytest.mark.target("esp32c6")
@pytest.mark.env("hil")
@pytest.mark.dependency(name="hal_i2c", depends=["hal_begin"])
def test_generic_com_i2c(dut):
    """Verify I2C master bus arbitration and device probing passed."""
    dut.expect(r"\[HIL_REPORT\] TEST: com_i2c_probe -> PASS", timeout=3)


@pytest.mark.target("esp32c6")
@pytest.mark.env("hil")
@pytest.mark.dependency(name="hal_summary", depends=["hal_rng", "hal_i2c"])
def test_hal_regression_summary(dut):
    """Verify entire HAL battery completed with 0 failures."""
    dut.expect(r"\[HIL_REPORT\] END_HAL_REGRESSION: \d+ PASSED, 0 FAILED", timeout=5)
```

### Stage 2: Board & Application Integration Suite (`test_02_board_relays.py`)
```python
"""
@file       test_02_board_relays.py
@brief      Board-specific HIL test for MCP23X17 expander and relay actuation.
"""

import pytest


@pytest.mark.target("esp32c6")
@pytest.mark.env("hil")
@pytest.mark.dependency(name="board_relays", depends=["hal_i2c"])
def test_smartplug_relay_actuation(dut):
    """
    Assert relay actuation and status notification.
    If generic test_generic_com_i2c fails, this test is SKIPPED automatically!
    """
    dut.expect(r"Proc_PowerSwitchEventManager: initialized", timeout=5)
```

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

## 6. Hardware Safety, Quarantine & Flash Isolation

1. **Relay Fail-Safe De-Energization:** Any test interacting with physical relays must guarantee that output pins are forced `LOW` (contacts open / load de-energized) on any failure path or abort.
2. **Ephemeral NVS Namespace (`hil_scratch`):** HIL tests are strictly forbidden from writing to production `fctry` or `nvs_keys` partitions. Storage tests operate exclusively in `hil_scratch` and clean up keys upon exit.
3. **Task Watchdog Protection:** `Proc_Hil_Test` feeds the hardware Task Watchdog (`IHal_Sys_Wdt`). If a hardware transaction hangs for $>3000\text{ ms}$, the hardware WDT triggers a panic restart rather than locking the test bench.
4. **Panic Trace Decoding:** If the target silicon panics during HIL, `pytest-embedded-idf` decodes the register dump and GDB backtrace from `build/` ELF symbols automatically [2, 3].

---

## 7. CLI Task Invocations (`embedded_system/Tasks/ci_cd.py`)

All HIL operations are driven via PyInvoke CLI tasks:

```bash
# 1. Run full HIL battery (Stage 1 HAL regression + Stage 2 Board tests)
inv es.ci-cd.hil

# 2. Run only Stage 1 Generic HAL regression
pytest CI_CD/tests/hil/suites/test_01_hal_regression.py --embedded-services idf,serial -s

# 3. Run only Stage 2 Board Integration tests
pytest CI_CD/tests/hil/suites/test_02_board_relays.py --embedded-services idf,serial -s
```

---

## 8. Pre-Execution Quality Checklist

Before running or committing HIL test changes:
- [ ] **Directory Purity:** Confirmed that **zero** test files exist in `embedded_system/Source/`.
- [ ] **Zero Test Frameworks in Firmware:** Confirmed zero `#include "unity.h"` or custom JSON RPC parsers in firmware source code.
- [ ] **Interface Contracts:** Generic tests in `embedded_system/Tests/hil/cases/` take `IHal_*` references and return `sys_error_t`.
- [ ] **Board Decoupling:** Generic test cases contain no pin numbers or `main/hwConfig/` references.
- [ ] **Production Gating:** `Proc_Hil_Test.cpp` is gated under a compile-time option and excluded from release firmware.
- [ ] **Dependency Pruning:** Board tests declare `@pytest.mark.dependency(depends=[...])` referencing upstream HAL tests.
- [ ] **Storage Safety:** Flash tests operate strictly on `hil_scratch` and never write to the manufacturing `fctry` partition.

---

## 9. Error Registry (Post-Mortem Lessons & Preventive Rules)

Scoped to `hil-hardware-testing`:

- **ERR-001 — In-Flight Relay Left Energized on Test Abort:**
  *Symptom:* A test assertion failed and the physical relay coil remained energized indefinitely.
  *Root Cause:* Test logic lacked fail-safe cleanup on early return.
  *Preventive Rule:* Hardware drivers must force safe quiescent states (`LOW` / open-circuit) in destructors or explicit process `stop()` routines.

- **ERR-002 — Production `fctry` Partition Corruption from HIL Tests:**
  *Symptom:* After running a HIL test suite, the device lost its calibrated factory credentials and Root CA certificates.
  *Root Cause:* Test wrote mock data to the default `nvs` partition without scoping to an isolated namespace.
  *Preventive Rule:* Force all generic and board storage tests to use the ephemeral `hil_scratch` namespace; audit that `fctry` is mounted read-only during tests.

- **ERR-003 — Cascade Failure Storm on Bus Faults:**
  *Symptom:* An unplugged I2C expander caused 40 downstream tests to fail, flooding CI logs with secondary errors.
  *Root Cause:* Tests were executed as a flat sequential list without dependency awareness.
  *Preventive Rule:* All board suites must declare `@pytest.mark.dependency(depends=["hal_i2c"])` so dependent tests are marked `SKIPPED` immediately upon bus failure.

- **ERR-004 — Test Harness Leaking into Release Firmware:**
  *Symptom:* Production devices responded to internal test commands over UART in the field.
  *Root Cause:* Test process was registered unconditionally in `main.cpp`.
  *Preventive Rule:* Gate `Proc_Hil_Test` behind the compile-time definition `ENABLE_HIL_TEST_PROCESS` (`-DENABLE_HIL_TEST_PROCESS=ON`) and exclude test files from release builds in CMake.
```
