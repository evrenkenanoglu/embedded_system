# Skill: C++ Embedded Code & Architecture Conventions

Use this skill when authoring, refactoring, reviewing, or analyzing C++ firmware and HAL/PAL components across the `embedded_system` codebase.

---

## 1. File Structure and Include Hierarchy

### File Naming & Header Guards
- **File Names:** Match the class name exactly in PascalCase (e.g., `HttpsServer.hpp`, `HttpsServer.cpp`).
  *Exception — HAL drivers:* Use lowercase area prefix + camelCase (e.g., `sys_rtc.hpp`, `sys_rtc.cpp`, `cpx_wifi.cpp`).
- **Extensions:** `.hpp` for C++ headers, `.cpp` for C++ implementations.
- **Header Guards:**
  - Modern C++ / PAL / Application / Modern HAL: `#pragma once` at the very top (after the file Doxygen block).
  - Legacy or pure C-compatible HAL interfaces (e.g., `IHal.h`, `IHal_Com_i2c.h`): Traditional `#ifndef FILE_NAME_H` guards.
  Legacy guard format:
  ```cpp
  #ifndef FILE_IHAL_H
  #define FILE_IHAL_H

  // ... interface content ...

  #endif // FILE_IHAL_H
  ```

### Include Ordering ("The Sandwich Rule")
Group `#include` statements in `.cpp` files into 4 distinct blocks separated by a single blank line. Ordering from most specific to most general implicitly validates that each header is self-contained (it compiles cleanly regardless of include order):
```cpp
// 1. Matching Header File
#include "HttpsServer.hpp"

// 2. Local Application / Protocol / PAL / HAL / System Headers
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUri.hpp"
#include "System/system.h"

// 3. Third-Party / ESP-IDF SDK Headers
#include <esp_https_server.h>
#include <mbedtls/ssl.h>

// 4. C++ Standard Library Headers (Use <c...> wrappers, never C <... .h> variants)
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
```

### Header Hygiene & Compile-Time Optimization
- **No Namespace Pollution:** Never use `using namespace <name>;` in any header file.
- **Forward Declarations:** If a header references a class only by pointer (`*`) or reference (`&`), forward-declare it (`class IHttpUri;`) instead of `#include`ing the full definition, and include the actual header only in the `.cpp` where members are accessed. Recommended, not mandatory — apply where it measurably cuts include cascades.
- **Standard Library Headers:** Always use `<cstdint>`, `<cstring>`, `<cstdio>` instead of `<stdint.h>`, `<string.h>`, `<stdio.h>`.

---

## 2. Identifier Naming Conventions

| Entity                   | Casing Style                 | Prefix / Suffix    | Rules & Examples                                                                                                                                                                                                               |
| :----------------------- | :--------------------------- | :----------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Interfaces (General)** | PascalCase                   | `I`-Prefix         | `IHttpServer`, `IHttpUri`, `ISntpClient`.                                                                                                                                                                                      |
| **Interfaces (HAL)**     | PascalCase                   | `IHal_` or `IHAL_` | Modern HAL interfaces: `IHal_<Area>_<Device>` (e.g., `IHal_Sys_Rtc`, `IHal_Cpx_Ble`, `IHal_Mem_Ota`). Base and legacy HAL interfaces: Screaming snake after prefix: `IHAL_IO`, `IHAL_MEM`, `IHAL_CPX_WIFI`.                    |
| **Concrete Classes**     | PascalCase                   | None               | `HttpsServer`, `HttpUri`, `OtaService`, `Serv_sntp`. **Exception — HAL drivers:** follow the lowercase area-prefix taxonomy (`io_gpio`, `com_i2c`, `cpx_wifi`, `mem_nvs`, `sys_wdt`, `sys_rtc`, `sys_resetReason`, `sys_rng`). |
| **Public Methods**       | camelCase                    | None               | Must start lowercase: `init()`, `registerUri()`, `sendWsMessage()`.                                                                                                                                                            |
| **Private Methods**      | camelCase                    | Single `_` prefix  | `_populateConfig()`, `_handleEvent()`, `_lock()`.                                                                                                                                                                              |
| **Private Members**      | camelCase                    | Single `_` prefix  | `_server`, `_sslConfig`, `_started`, `_mutex`.                                                                                                                                                                                 |
| **Local Variables**      | camelCase                    | None               | `maxClients`, `platformUri`. *Exception:* Match SDK naming for raw descriptors (`clientId_sockfd`).                                                                                                                            |
| **Parameters**           | camelCase                    | None               | `options`, `payload`, `timeoutMs`. No Hungarian notation.                                                                                                                                                                      |
| **Macros & Constants**   | SCREAMING_SNAKE / kCamelCase | None / `k` prefix  | Macros: `HAL_INITIALIZED`, `ENABLE_SYS_LOG_D`, `ERROR_SUCCESS`. File-local constants: `constexpr uint32_t kMutexTimeoutMs = 100U;`.                                                                                            |

---

## 3. Formatting, Braces & Section Layout

- **Indentation:** Exactly 4 spaces per level. Never use raw tab characters.
- **Brace Placement:** Allman style (opening brace on a new line) for namespaces, classes, functions, structs, and `switch` statements:
  ```cpp
  class HttpsServer
  {
  public:
      sys_error_t start()
      {
          // Implementation
      }
  };
  ```
- **File-Scope Helpers:** Encapsulate non-member helpers and converters in an anonymous namespace within the `.cpp` file:
  ```cpp
  namespace
  {
  httpd_ws_type_t toPlatformWsType(WsFrameType type)
  {
      // ...
  }
  } // namespace
  ```

### Source File Section Separator Blocks
In `.cpp` files (and legacy `.h` files), partition content using uppercase section comment blocks:
```cpp
/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/
```

---

## 4. Error Handling and Recovery (`sys_error_t`)

- **Standard Status Type:** All fallible functions must return `sys_error_t`. Success is always `ERROR_SUCCESS`.
- **SDK Translation:** Translate third-party or ESP-IDF return codes (`esp_err_t`) using `TRANSLATE_ERROR(err)` before bubbling errors up.

### Error Macro Reference

| Macro                                               | Evaluated Expression | Action on Error                                                                                              | Recommended Use Case                                                                                                       |
| :-------------------------------------------------- | :------------------- | :----------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------- |
| `RETURN_ON_ERROR(expr, ...)`                        | `sys_error_t`        | If `expr != ERROR_SUCCESS`, runs cleanup (`...`), returns error.                                             | Calling functions that return `sys_error_t`.                                                                               |
| `RETURN_ON_ERROR_WITH_LOG(expr, msg, ...)`          | `sys_error_t`        | If `expr != ERROR_SUCCESS`, logs `msg`, cleans up, returns error.                                            | Bubbling up internal operational failures with log context.                                                                |
| `ON_ERROR_WITH_LOG(expr, msg)`                      | `sys_error_t`        | Logs file/line, code, and `msg`. **Does not return**.                                                        | Non-fatal steps where execution must continue.                                                                             |
| `RETURN_IF_ERROR(cond, errCode, ...)`               | Boolean condition    | **Assertion/Precondition:** If `cond` is **`false`** (check fails), runs cleanup (`...`), returns `errCode`. | Pointer validity (`ptr != nullptr`), state assertions (`_initialized == HAL_INITIALIZED`), mutex locks (`_lock(timeout)`). |
| `RETURN_IF_ERROR_WITH_LOG(cond, errCode, msg, ...)` | Boolean condition    | If `cond` is **`false`**, logs `msg`, runs cleanup, returns `errCode`.                                       | Invariant and precondition checks requiring explicit warning/error logs.                                                   |
| `BREAK_ON_ERROR(expr)`                              | Boolean condition    | If `expr` is `false`, breaks out of containing loop.                                                         | Batch processing loops and byte stream iteration.                                                                          |

### Precondition Assertion Formatting (`RETURN_IF_ERROR`)
`RETURN_IF_ERROR(condition, errCode)` checks that the condition is **satisfied/true**. It triggers and returns `errCode` when the condition is **violated/false**:

```cpp
// Pointer null-check (asserts outputBuffer is NOT null)
RETURN_IF_ERROR(outputBuffer != nullptr, ERROR_NULL_POINTER);

// Numeric range check (asserts length is positive)
RETURN_IF_ERROR(length > 0U, ERROR_INVALID_ARG);

// Lifecycle check (asserts driver is initialized)
RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

// Mutex acquisition (asserts lock succeeded)
RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);
```

---

## 5. Resource Management, RAII & Memory Safety

### Rule of Five & Ownership
Classes wrapping physical peripherals, comms channels, or hardware driver handles (`sys_rtc`, `sys_rng`, `sys_resetReason`, `Serv_sntp`, `mem_ota`, `HttpsServer`, etc.) must enforce **unique ownership**; copying and moving these classes is prohibited to prevent duplicate deletions and dangling hardware handles. Destructors must call the corresponding deinitialization or stop routine (`deInit()`, `stop()`, `disconnect()`) to ensure graceful resource release when going out of scope:
```cpp
class sys_rtc : public IHal_Sys_Rtc
{
public:
    sys_rtc();
    ~sys_rtc() override;

    // Block copy semantics
    sys_rtc(const sys_rtc&) = delete;
    sys_rtc& operator=(const sys_rtc&) = delete;

    // Block move semantics
    sys_rtc(sys_rtc&&) = delete;
    sys_rtc& operator=(sys_rtc&&) = delete;
};
```

### Deterministic Heap Management (No-Throw)
- Embedded dynamic allocations must be deterministic: never allow exceptions to fire.
- Use placement new with `std::nothrow` and wrap allocations in `std::unique_ptr`:
  ```cpp
  const size_t buf_size = 1024;
  std::unique_ptr<char[]> buf(new (std::nothrow) char[buf_size]());
  if (!buf)
  {
      SYS_LOG_E("Out of memory - Buffer allocation failed!");
      return ERROR_NO_MEM;
  }
  ```

### Memory Alignment & Strict Aliasing (No `reinterpret_cast`)
- **Prohibition:** Never cast raw byte buffers (`uint8_t*`) directly to struct pointers via `reinterpret_cast`.
- **Rule:** Copy data into an aligned, stack-allocated struct via `std::memcpy`:
  ```cpp
  esp_app_desc_t app_info;
  std::memcpy(&app_info, raw_buffer + offset, sizeof(esp_app_desc_t));
  ```

---

## 6. Logging and Diagnostics

### Log Level Mapping

| Macro            | Level     | Audience       | Purpose                                                                                  |
| :--------------- | :-------- | :------------- | :--------------------------------------------------------------------------------------- |
| `SYS_LOG_E(...)` | **ERROR** | Operator / Dev | Critical execution failure (allocation failure, driver init failure, transaction abort). |
| `SYS_LOG_W(...)` | **WARN**  | Operator / Dev | Recoverable anomalies, redundant calls, identical version re-flash attempts.             |
| `SYS_LOG_I(...)` | **INFO**  | End User / Op  | Lifecycle transitions (interface ready, server bound, URI registered).                   |
| `SYS_LOG_D(...)` | **DEBUG** | Developer      | High-frequency diagnostics, payload dumps, byte counts. Stripped in production.          |

### Module Debug Filtering
To enable debug logging for a specific `.cpp` file, define `ENABLE_SYS_LOG_D` **before** including `LogHandler.h`:
```cpp
#define ENABLE_SYS_LOG_D

/*** INCLUDES ***/
#include "sys_rtc.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
```

---

## 7. Concurrency & ESP-IDF Integration

### FreeRTOS Synchronization
- **Bounded Lock Timeouts:** Mutexes must always take a finite timeout (e.g., `pdMS_TO_TICKS(kMutexTimeoutMs)`), never `portMAX_DELAY`, to prevent deadlocks:
  ```cpp
  constexpr uint32_t kMutexTimeoutMs = 100U;

  bool sys_rtc::_lock(TickType_t timeoutMs)
  {
      if (_mutex == nullptr)
      {
          return false;
      }
      return xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
  }

  void sys_rtc::_unlock()
  {
      if (_mutex != nullptr)
      {
          (void)xSemaphoreGive(_mutex);
      }
  }
  ```

### Interrupt Service Routine (ISR) Hard Constraints
- ❌ **No blocking calls:** Never call `vTaskDelay`, or block on mutexes/semaphores/queues.
- ❌ **No dynamic allocation:** Never call `new`, `malloc`, `delete`, `free`.
- ❌ **No standard logging:** Never call `SYS_LOG_*` or `printf`.
- ✔️ **`FromISR` APIs Only:** Use only FreeRTOS functions suffixed with `FromISR` (e.g., `xQueueSendFromISR`, `xSemaphoreGiveFromISR`).
- ✔️ **Context Switching:** Always pass `BaseType_t *pxHigherPriorityTaskWoken` to `FromISR` calls and yield if unblocked.

---

## 8. Doxygen Documentation & In-Body Procedural Flow

### 1. File-Level Header (Top of `.hpp`, `.cpp`, `.h`)
Must precede all `#pragma once` and `#include` directives:
```cpp
/** @file       Filename.hpp
 *  @brief      A brief one-sentence description of the file's purpose.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       DD/MM/YYYY
 */
```

### 2. Class-Level Block (Preceding class declaration)
```cpp
/**
 * @class ClassName
 * @brief High-level summary of the class's responsibility and target hardware.
 *
 * @note Thread-Safety: Specify if the class is thread-safe, reentrant, or requires
 *       external synchronization primitives.
 */
class ClassName
{
    // ...
};
```

### 3. Public Method Block (Preceding declarations in `.hpp`)
```cpp
/**
 * @brief Deinitialize the communication interface and release associated hardware.
 *
 * Detailed description of any side effects (closing sockets, disabling transceivers).
 *
 * @param[in]  timeout_ms The maximum time to wait for a graceful shutdown.
 * @param[out] status_out Pointer to save the driver's final shutdown state.
 *
 * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
 *
 * @note This method must not be called from an ISR context.
 * @warning Access to this resource must be synchronized if called from multiple tasks.
 */
virtual sys_error_t deInit(uint32_t timeout_ms, uint32_t* status_out) = 0;
```
*Note on `override`:* Concrete implementations in `.hpp` using `override` do **not** require duplicated Doxygen comments if the base interface is already documented.

---

### 4. In-Body Procedure Flow (`///`) vs. Private Developer Notes (`//`)

Functions with procedural flows, multiple milestones, state transitions, or validation sequences must differentiate between **exported procedural documentation** and **private developer comments**:

#### A. Exported Procedural Steps: `///` (Triple-Slash)
- **Purpose:** Directly describes the procedural steps, actions, and milestones of the function.
- **Export Behavior:** Doxygen extracts `///` comments directly into the generated documentation under the function's **Detailed In-Body Description**.
- **Rules:**
  - Must align with current code indentation.
  - Do **not** clutter comments with `###`, `Step 1:`, or `STEP X` prefixes. Write concise, direct descriptions of the procedure.
  - Standard Doxygen commands (`@note`, `@warning`) can be used if additional context is needed.

#### B. Private Developer Notes: `//` (Double-Slash)
- **Purpose:** Reserved strictly for private notes, low-level register workarounds, debug hints, and inline scratchpad notes.
- **Export Behavior:** Intentionally **ignored** by Doxygen; will never appear in exported documentation.

#### In-Body Flow Example:
```cpp
sys_error_t sys_rtc::setTime(time_t epochSeconds, uint32_t fractionalUs)
{
    /// Validate initialization state and fractional microsecond bounds.
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(fractionalUs <= kFractionalUsMax, ERROR_INVALID_ARG);

    /// Acquire the serialization mutex to serialize access across concurrent tasks.
    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    // Hardware note: settimeofday expects struct timeval with microseconds (Ignored by Doxygen)
    struct timeval tv;
    tv.tv_sec  = epochSeconds;
    tv.tv_usec = static_cast<suseconds_t>(fractionalUs);

    /// Commit the wall-clock timestamp to the POSIX system time base.
    if (settimeofday(&tv, nullptr) != 0)
    {
        _unlock();
        SYS_LOG_E("sys_rtc: settimeofday failed (errno=%d)", errno);
        return ERROR_WRITE_FAILED;
    }

    /// Release the synchronization mutex.
    _unlock();

    SYS_LOG_I("sys_rtc: time set to %lld.%06lu s", static_cast<long long>(epochSeconds), static_cast<unsigned long>(fractionalUs));
    return ERROR_SUCCESS;
}
```
