---
name: embedded-hal-architecture
description: >-
  Structure and integration rules for the embedded_system Hardware Abstraction
  Layer: IHAL interfaces, Common and Platform driver layout, driver naming
  taxonomy, interface vs implementation responsibilities, unit-test location,
  and the legacy-API deviation policy. General C++ coding rules live in the
  embedded-cpp-code-conventions skill.
---

# Skill: HAL Architecture

Use this skill when adding, moving, reviewing, or integrating anything under
`embedded_system/Source/HAL/`, or when deciding how a new driver must be named,
layered, and tested. This skill defines **structure only**. All general C++
coding rules (formatting, error handling, RAII, logging, concurrency, Doxygen)
are owned by `embedded_system/.agents/skills/embedded-cpp-code-conventions/SKILL.md`
and are not repeated here.

## 1. Directory layout

```
embedded_system/Source/HAL/
├── IHAL/                  # Platform-independent abstract interfaces (public API)
│   ├── IHal.h             # Master aggregation header (IHAL_IO/COM/MEM/CPX/SYS)
│   └── IHal_<Area>_<Device>.h
├── Common/                # Cross-platform concrete drivers (no SDK includes)
│   ├── io_gpio_expander   # Example: MCP23X17-backed expander wrapper
│   └── cpx_mcp23x17
└── Platform/<Name>/       # Per-SDK implementations (ESP32 today)
    ├── <driver>.cpp/.hpp
    ├── Library/           # SDK glue used by drivers (logImpl, errorTranslateImpl)
    └── Tests/Unit_Tests/  # Fake SDK headers + unit tests for platform drivers
```

Responsibilities:

- **`IHAL/`** — Pure abstract interfaces. No SDK includes, no platform types,
  no implementation state. This is the only layer applications and processes
  may depend on.
- **`Common/`** — Concrete drivers portable across platforms. May depend on
  `IHAL/` and `System/` but must not include platform SDK headers.
- **`Platform/<Name>/`** — SDK-backed implementations of `IHAL/` interfaces
  (e.g. `cpx_wifi` implements `IHal_Cpx_Wifi.h` on ESP32). May include SDK
  headers. `Library/` holds shared SDK glue; `Tests/Unit_Tests/` holds the
  fake SDK headers and unit tests that make platform drivers host-testable.

Dependency direction is strictly one-way:
`Application/Process → IHAL → Common → Platform/SDK`. Nothing in `IHAL/` may
include from `Common/`, `Platform/`, or any SDK.

## 2. Driver naming taxonomy

File, class, and interface names encode the HAL area via a lowercase type
prefix. This convention is **unique to HAL drivers** and intentionally differs
from the PascalCase rule for application-layer concrete classes (see the
conventions skill, section 2).

| Prefix | Area                                  | Interface header  | ESP32 implementation                 |
| :----- | :------------------------------------ | :---------------- | :----------------------------------- |
| `io_`  | Digital/analogue I/O                  | `IHal_Io_Gpio.h`  | `io_gpio`                            |
| `com_` | Communication buses                   | `IHal_Com_i2c.h`  | `com_i2c`                            |
| `cpx_` | Complex peripherals / protocol stacks | `IHal_Cpx_Wifi.h` | `cpx_wifi`, `cpx_credentialsManager` |
| `mem_` | Memory and persistence                | `IHal_Mem_Ota.h`  | `mem_nvs`, `mem_ota`                 |
| `sys_` | Platform/system services              | `IHal_Sys_Wdt.h`  | `sys_wdt`                            |

Rules:

- **Interface files:** `I_` prefix, then camelCase starting uppercase:
  `I_ExampleInterface` (per `Documentation/name_conventions.txt`). The current
  `IHal_<Area>_<Device>.h` naming is the accepted in-tree variant for the
  aggregation headers listed in section 1.
- **HAL driver classes/files:** lowercase area prefix + camelCase:
  `io_gpio`, `com_i2c`, `cpx_wifi`, `mem_nvs`. No suffix.
- **Struct/enum typedefs:** `_t` suffix, camelCase starting lowercase:
  `exampleStruct_t`, `exampleEnum_t` (e.g. `wifiMode_t`, `sys_error_t`).
- **Header guards** (interfaces and any file not using `#pragma once`):
  ALL_CAPS with `_H` suffix, e.g. `MY_HEADER_H`.
- **Master aggregation:** `IHal.h` is the single include consumers use to reach
  the whole HAL surface; area headers remain individually includable.

## 3. Interface vs implementation contract

- An `IHAL/` interface declares the complete public contract: lifecycle
  (`init`/`deInit`), operations, and callbacks — fully documented with Doxygen
  per the conventions skill, section 8.
- Platform implementations document only deviations or additions; methods
  marked `override` do not duplicate base-interface Doxygen.
- New capabilities are added to the interface **first**, then implemented per
  platform. Never expose platform-only types through an `IHAL/` header.
- Platform-specific extensions that are not part of the portable contract
  (e.g. `cpx_credentialsManager`) live beside the platform drivers under
  `Platform/<Name>/` and are not promoted to `IHAL/` until a second consumer
  or platform needs them.

## 4. Legacy-API and deviation policy

The tree currently contains known deviations. They are **documented debt,
not licence to copy the pattern**: do not reproduce them in new code, and do
not "fix" them opportunistically — schedule them as dedicated cleanup tasks.

Known deviations (audit of this tree, no code fixes applied):

1. `cpx_wifi.h` carries C++ content under a `.h` extension; guard is
   `CPX_WIFI_HPP` while the Doxygen `@file` tag says `cpx_wifi.hpp`.
2. Header-guard style is split: `#ifndef` guards in `io_gpio`, `com_i2c`,
   `mem_nvs`, `cpx_wifi`; `#pragma once` in `sys_wdt`, `mem_ota`,
   `cpx_credentialsManager`.
3. `IHal_Io_Gpio.h` and `IHal_Com_i2c.h` lack file-level Doxygen headers.
4. `IHal.h` `@file` tag says `ihal.h` (name mismatch).
5. Five dead helper declarations in `IHal_Cpx_Wifi.h`
   (`hal_wifi_*_to_string` / `hal_wifi_validate_*`) have no definition.
6. Six legacy `cpx_wifi` APIs (`setWifiMode`, `getWifiMode`, `scan(void*)`,
   `getWifiEventGroup`, queue-based `getScanResults`, `wifiApRecord_t`) have
   zero consumers project-wide.
7. `Platform/ESP32/hal_watchdog.hpp` is an empty (0-byte) file.
8. `#undef INTERFACE` artifacts in `IHal.h` / `IHal_Io_Gpio.h`
   (`INTERFACE` is never defined).
9. Enum style is split: C `typedef enum` + `_t` (wifi, i2c) vs `enum class`
   (gpio, ota).
10. `System/system.h` uses C headers `<stdio.h>` / `<string.h>` instead of
    `<cstdio>` / `<cstring>`.

## 5. Testing

- Platform driver unit tests live in `Platform/<Name>/Tests/Unit_Tests/`,
  alongside `Fake/` SDK header shims that stub the SDK for host builds.
- Common drivers are testable without fakes (no SDK dependency).
- New drivers must ship with tests in the matching location; see the
  conventions skill for general test-code rules.
