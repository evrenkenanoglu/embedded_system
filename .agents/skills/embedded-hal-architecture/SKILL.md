---
name: embedded-hal-architecture
description: >-
  Structure and integration rules for the embedded_system Hardware Abstraction
  Layer: IHAL interfaces, Common and Platform driver layout, driver naming
  taxonomy, interface vs implementation responsibilities, and the legacy-API deviation
  policy. General C++ coding rules live in the embedded-cpp-code-conventions skill.
---

# Skill: HAL Architecture

Use this skill when adding, moving, reviewing, or integrating anything under
`embedded_system/Source/HAL/`, or when deciding how a new driver must be named
and layered. This skill defines **structure only**. All general C++
coding rules (formatting, assertion error handling via `RETURN_IF_ERROR`, RAII, logging, concurrency, Doxygen)
are owned by `embedded_system/.agents/skills/embedded-cpp-code-conventions/SKILL.md`
and are not repeated here.

---

## 1. Directory Layout

```text
embedded_system/Source/HAL/
├── IHAL/                  # Platform-independent abstract interfaces (public API)
│   ├── IHal.h             # Master base interfaces (IHAL_IO/COM/MEM/CPX/SYS)
│   └── IHal_<Area>_<Device>.h # Specialized domain interfaces (IHal_Sys_Rtc, IHal_Cpx_Ble, etc.)
├── Common/                # Cross-platform concrete drivers (no SDK includes)
│   ├── io_gpio_expander   # Example: MCP23X17-backed expander wrapper
│   └── cpx_mcp23x17
└── Platform/<Name>/       # Per-SDK implementations (ESP32)
    ├── <driver>.cpp/.hpp
    └── Library/           # SDK glue used by drivers (logImpl, errorTranslateImpl)
```

### Layer Responsibilities:
- **`IHAL/`** — Pure abstract interfaces. No SDK includes, no platform types,
  no implementation state. This is the only HAL layer that applications, processes,
  and PAL protocols may depend on.
  - **Foundational Base Interfaces (`IHal.h`):** `IHAL_IO`, `IHAL_COM`, `IHAL_MEM`, `IHAL_CPX`, `IHAL_SYS`.
  - **Specialized Interfaces:** `IHal_Io_Adc.h`, `IHal_Sys_Rtc.h`, `IHal_Sys_ResetReason.h`, `IHal_Sys_Rng.h`, `IHal_Sys_Wdt.h`, `IHal_Cpx_Ble.h`, `IHal_Mem_Ota.h`.
  - *Note on `IHal_Mem_Ota`:* Manages dual-bank bootloader slot lifecycles, hash verification, and provisional trial states. It is a specialized standalone interface that does not inherit from generic byte-addressable `IHAL_MEM`.
- **`Common/`** — Concrete drivers portable across platforms. May depend on
  `IHAL/` and `System/` but must not include platform SDK headers.
- **`Platform/<Name>/`** — SDK-backed implementations of `IHAL/` interfaces
  (e.g., `sys_rtc` implements `IHal_Sys_Rtc.h`, `cpx_wifi` implements `IHal_Cpx_Wifi.h`). May include SDK
  headers. `Library/` holds shared SDK glue.

**Dependency direction is strictly one-way:**
$$\text{Application/Process} \longrightarrow \text{IHAL} \longrightarrow \text{Common} \longrightarrow \text{Platform/SDK}$$
Nothing in `IHAL/` may include from `Common/`, `Platform/`, or any platform SDK.

### Generic Peripheral Interfaces and Consumers
- Model reusable hardware capabilities in `IHAL/`, not the use case of the first consumer. For example, `IHal_Io_Adc` represents ADC unit/channel configuration, raw samples, and calibrated voltage conversion; it must not be a temperature-sensor-specific ADC contract.
- Keep ADC configuration platform-neutral. Platform implementations may reject unsupported units, resolutions, attenuation, or calibration with the appropriate error rather than leaking SDK types into the interface.
- Keep consumer policy such as sample averaging, filtering, thresholds, and physical-unit conversion in the consuming sensor or process.
- Inject hardware interfaces into consumers by reference or another explicit non-owning contract. The composition root owns initialization, deinitialization, and lifetime ordering of the concrete hardware driver; a consumer must not tear down an injected peripheral it does not own.
- Single-owner peripheral drivers need not add internal mutexes by default. Document the single-owner/external-serialization contract; add internal synchronization only when the interface explicitly promises concurrent access or there are multiple independent callers that require it.

---

## 2. Driver & Interface Naming Taxonomy

File, class, and interface names encode the HAL area via a lowercase type prefix. This convention is **unique to HAL drivers** and intentionally differs from the PascalCase rule used for application-layer concrete classes.

| Prefix | Area                                  | Specialized Interface Header | Interface Class Name                                             | ESP32 Platform Implementation      |
| :----- | :------------------------------------ | :--------------------------- | :--------------------------------------------------------------- | :--------------------------------- |
| `io_`  | Digital/analogue I/O                  | `IHal_Io_Gpio.h`<br>`IHal_Io_Adc.h` | `IHAL_IO_GPIO` *(legacy)*<br>`IHal_Io_Adc` *(modern)* | `io_gpio`<br>`io_adc` |
| `com_` | Communication buses                   | `IHal_Com_i2c.h`             | `IHAL_COM_I2C` *(legacy)*                                        | `com_i2c`                          |
| `cpx_` | Complex peripherals / wireless stacks | `IHal_Cpx_Ble.h`             | `IHal_Cpx_Ble` *(modern)*<br>`IHAL_CPX_WIFI` *(legacy)*           | `cpx_ble`<br>`cpx_wifi`            |
| `mem_` | Memory and persistence                | `IHal_Mem_Ota.h`             | `IHal_Mem_Ota` *(standalone)*                                    | `mem_nvs`, `mem_ota`               |
| `sys_` | Silicon/system supervision & RTC      | `IHal_Sys_Rtc.h`<br>`IHal_Sys_Rng.h`<br>`IHal_Sys_ResetReason.h`<br>`IHal_Sys_Wdt.h` | `IHal_Sys_Rtc`<br>`IHal_Sys_Rng`<br>`IHal_Sys_ResetReason`<br>`IHal_Sys_Wdt` | `sys_rtc`<br>`sys_rng`<br>`sys_resetReason`<br>`sys_wdt` |

### Taxonomy Rules:
- **Interface Files:** `IHal_<Area>_<Device>.h` (e.g., `IHal_Sys_Rtc.h`, `IHal_Cpx_Ble.h`).
- **Interface Classes:**
  - **Modern Standard:** PascalCase with `IHal_` prefix matching the filename (`class IHal_Sys_Rtc : public IHAL_SYS`).
  - **Legacy In-Tree Variant:** Screaming snake with `IHAL_` prefix (`class IHAL_IO_GPIO : public IHAL_IO`). Do not use for new interfaces.
- **HAL Driver Classes and Source Files:** Lowercase area prefix + camelCase:
  `sys_rtc`, `sys_rng`, `sys_resetReason`, `io_gpio`, `com_i2c`, `cpx_wifi`, `mem_nvs`. No suffix.
- **Struct/Enum Typedefs:** `_t` suffix, camelCase starting lowercase:
  `hal_rtc_config_t`, `hal_wifi_mode_t`, `hal_reset_reason_t`. Scoped enums use `enum class name_t : uint8_t`.
- **Header Guards:**
  - Modern interfaces and platform headers: Use `#pragma once`.
  - Legacy headers: ALL_CAPS with `_H` suffix, e.g. `FILE_IHAL_COM_I2C_H`.

---

## 3. Interface vs. Implementation Contract

- An `IHAL/` interface declares the complete public contract: lifecycle (`init`/`deInit`), operational methods, and asynchronous event callbacks — fully documented with Doxygen.
- Platform implementations document only deviations or hardware-specific nuances; methods marked `override` do not duplicate base-interface Doxygen.
- New capabilities are added to the interface **first**, then implemented per platform. Never expose platform-only types through an `IHAL/` header.
- Platform drivers that acquire resources in multiple initialization stages use a shared cleanup routine for partial-init failures and `deInit()`. Cleanup returns and logs failures explicitly and retains handles whose release failed so teardown can be retried.
- Platform-specific extensions that are not part of the portable contract (e.g., `cpx_credentialsManager`) live beside the platform drivers under `Platform/<Name>/` and are not promoted to `IHAL/` until a second consumer or platform requires them.

---

## 4. Legacy-API and Deviation Policy

The tree currently contains known deviations. They are **documented technical debt, not licence to copy the pattern**: do not reproduce them in new code, and do not "fix" them opportunistically without a dedicated cleanup task.

### Known Deviations:
1. **Header File Extensions:** `cpx_wifi.h` and `cpx_ble.h` contain C++ headers but retain legacy `.h` extensions rather than `.hpp`.
2. **Header Guard Split:**
   - Legacy `#ifndef` guards: `IHal.h`, `IHal_Io_Gpio.h`, `IHal_Com_i2c.h`, `IHal_Cpx_Wifi.h`, `IHal_Mem_Ota.h`, `io_gpio`, `com_i2c`, `mem_nvs`, `cpx_wifi`.
   - Modern `#pragma once`: `IHal_Cpx_Ble.h`, `IHal_Sys_ResetReason.h`, `IHal_Sys_Rng.h`, `IHal_Sys_Rtc.h`, `IHal_Sys_Wdt.h`, `sys_rtc.hpp`, `sys_rng.hpp`, `sys_resetReason.hpp`, `sys_wdt.hpp`.
3. **Class Naming Split in Interfaces:** Older interfaces use screaming snake (`IHAL_IO_GPIO`, `IHAL_COM_I2C`, `IHAL_CPX_WIFI`), whereas newer interfaces use PascalCase area naming (`IHal_Cpx_Ble`, `IHal_Sys_ResetReason`, `IHal_Sys_Rng`, `IHal_Sys_Rtc`, `IHal_Sys_Wdt`).
4. **Standalone Interface:** `IHal_Mem_Ota` does not inherit from `IHAL_MEM` in `IHal.h`; it operates independently for dual-partition firmware flashing and boot-slot verification.
5. **Dead Helper Declarations:** Five helper declarations at the bottom of `IHal_Cpx_Wifi.h` (`hal_wifi_*_to_string`, `hal_wifi_validate_*`) have no corresponding definition.
6. **Legacy `cpx_wifi` APIs:** Six legacy `cpx_wifi` APIs (`setWifiMode`, `getWifiMode`, `scan(void*)`, `getWifiEventGroup`, queue-based `getScanResults`, `wifiApRecord_t`) have zero active consumers in the application layer.
7. **Empty Stubs:** `Platform/ESP32/hal_watchdog.hpp` is an empty (0-byte) file kept for backwards include compatibility.
8. **Macro Artifacts:** `#undef INTERFACE` artifacts exist at the end of `IHal.h` and `IHal_Io_Gpio.h`.
9. **Enum Style Split:** Legacy C `typedef enum { ... } name_t` (in `IHal_Cpx_Wifi.h`, `IHal_Com_i2c.h`) vs modern C++ scoped `enum class name_t : uint8_t` (in `IHal_Cpx_Ble.h`, `IHal_Io_Gpio.h`, `IHal_Sys_ResetReason.h`, `IHal_Sys_Rtc.h`).
10. **System Header Inclusions:** `System/system.h` uses legacy C headers `<stdio.h>` / `<string.h>` instead of `<cstdio>` / `<cstring>`.
