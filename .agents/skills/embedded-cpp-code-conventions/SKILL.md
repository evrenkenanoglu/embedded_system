# Skill: C++ Embedded Code & Architecture Conventions

Use this skill when authoring, refactoring, reviewing, or analyzing C++ firmware and HAL/PAL components across the `embedded_system` codebase.

---

## 1. File Structure and Include Hierarchy

### File Naming & Header Guards
- **File Names:** Match the class name exactly in PascalCase (e.g., `HttpsServer.hpp`, `HttpsServer.cpp`).
- **Extensions:** `.hpp` for C++ headers, `.cpp` for C++ implementations.
- **Header Guards:**
  - Modern C++ / PAL / Application: `#pragma once` at the very top (after file Doxygen block).
  - Pure C-compatible HAL interfaces (e.g., `ihal.h`): Traditional `#ifndef FILE_NAME_H` guards.
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

| Entity                 | Casing Style    | Prefix / Suffix   | Rules & Examples                                                                                                                                                                                                                                                                                            |
| :--------------------- | :-------------- | :---------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Interfaces**         | PascalCase      | `I`-Prefix        | `IHttpServer`, `IHttpUri`. HAL abstract classes may use screaming snake after prefix: `IHAL_IO`, `IHAL_MEM`.                                                                                                                                                                                                |
| **Concrete Classes**   | PascalCase      | None              | `HttpsServer`, `HttpUri`, `OtaService`. **Exception — HAL drivers:** file, class, and interface names under `Source/HAL/` follow the lowercase area-prefix taxonomy (`io_gpio`, `com_i2c`, `cpx_wifi`, `mem_nvs`, `sys_wdt`, `IHal_<Area>_<Device>.h`); see `embedded-hal-architecture/SKILL.md` section 2. |
| **Public Methods**     | camelCase       | None              | Must start lowercase: `init()`, `registerUri()`, `sendWsMessage()`.                                                                                                                                                                                                                                         |
| **Private Methods**    | camelCase       | Single `_` prefix | `_populate_config()`, `_handle_event()`, `_cleanBuffers()`.                                                                                                                                                                                                                                                 |
| **Private Members**    | camelCase       | Single `_` prefix | `_server`, `_sslConfig`, `_started`, `_semaphore`.                                                                                                                                                                                                                                                          |
| **Local Variables**    | camelCase       | None              | `maxClients`, `platformUri`. *Exception:* Match SDK naming for raw descriptors (`clientId_sockfd`).                                                                                                                                                                                                         |
| **Parameters**         | camelCase       | None              | `options`, `payload`, `timeoutMs`. No Hungarian notation.                                                                                                                                                                                                                                                   |
| **Macros & Constants** | SCREAMING_SNAKE | None              | `HAL_INITIALIZED`, `ENABLE_SYS_LOG_D`, `ERROR_SUCCESS`.                                                                                                                                                                                                                                                     |

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

| Macro                                               | Evaluated Expression | Action on Error                                               | Recommended Use Case                                         |
| :-------------------------------------------------- | :------------------- | :------------------------------------------------------------ | :----------------------------------------------------------- |
| `RETURN_ON_ERROR(expr, ...)`                        | `sys_error_t`        | Runs cleanup (`...`), returns evaluated error code.           | Calling internal operations returning `sys_error_t`.         |
| `RETURN_ON_ERROR_WITH_LOG(expr, msg, ...)`          | `sys_error_t`        | Logs file/line, code, and `msg`. Runs cleanup, returns error. | Bubbling up internal operational failures with log context.  |
| `ON_ERROR_WITH_LOG(expr, msg)`                      | `sys_error_t`        | Logs file/line, code, and `msg`. **Does not return**.         | Non-fatal steps where execution must continue.               |
| `RETURN_IF_ERROR(expr, errCode, ...)`               | Boolean condition    | If `true`, runs cleanup (`...`), returns `errCode`.           | Pointer checks (`ptr == nullptr`) or raw SDK non-zero codes. |
| `RETURN_IF_ERROR_WITH_LOG(expr, errCode, msg, ...)` | Boolean condition    | If `true`, logs `msg`, runs cleanup, returns `errCode`.       | Validation checks requiring explicit warning/error logs.     |
| `BREAK_ON_ERROR(expr)`                              | Boolean condition    | If `true`, breaks out of containing loop.                     | Batch processing loops and byte stream iteration.            |

### Evaluating Return Codes Safely
Always cast the output of an evaluated expression inside the standard return macros if the expression type is not implicitly `sys_error_t` (e.g. raw SDK return codes passed to `RETURN_IF_ERROR`).

### Multi-Line Macro Invocation Formatting
When macro calls span multiple lines, each parameter must sit on its own line documented with right-aligned comments:
```cpp
RETURN_IF_ERROR(
    (length < min_header_size),                                                                                                 // Expression
    ERROR_INVALID_ARG,                                                                                                          // Error code
    SYS_LOG_E("First write chunk size too small for header validation: %zu bytes (min required: %zu)", length, min_header_size) // Error message
);
```

The same right-aligned comment discipline applies to log-coupled SDK-error translation:
```cpp
RETURN_IF_ERROR(
    (err != ESP_OK),                                                         // Expression
    TRANSLATE_ERROR(err),                                                    // Error code
    SYS_LOG_E("esp_ota_begin failed: %s (0x%x)", ERROR_MESSAGE(err), err)    // Error message / Log
);
```

### State Cleanup via Variadic Arguments (`__VA_ARGS__`)
```cpp
RETURN_ON_ERROR(
    _prepare_request(method, path, headers, body, body_len),    // Expression
    SYS_LOG_E("Failed to prepare request!");                    // Log Message
    _active_response_buf = nullptr;                             // Inline cleanup
);
```

---

## 5. Resource Management, RAII & Memory Safety

### Rule of Five & Ownership
Classes wrapping physical peripherals, comms channels, or hardware driver handles (`HttpsClient`, `HttpsServer`, `mem_ota`, etc.) must enforce **unique ownership**; copying and moving these classes is prohibited to prevent duplicate deletions and dangling hardware handles. Destructors must call the corresponding deinitialization or stop routine (`deInit()`, `stop()`, `disconnect()`) to ensure graceful resource release when going out of scope:
```cpp
class HttpsServer : public IHttpServer
{
public:
    explicit HttpsServer(const HttpServerStartOptions_t& options);
    ~HttpsServer() override;

    // Block copy semantics
    HttpsServer(const HttpsServer&) = delete;
    HttpsServer& operator=(const HttpsServer&) = delete;

    // Block move semantics (unless explicitly required and designed)
    HttpsServer(HttpsServer&&) = delete;
    HttpsServer& operator=(HttpsServer&&) = delete;
};
```
- **Destructors:** Must call corresponding deinitialization or stop routines (`deInit()`, `stop()`, `disconnect()`).

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
  // buf is automatically freed when it goes out of scope
  ```

### Memory Alignment & Strict Aliasing (No `reinterpret_cast`)
- **Prohibition:** Never cast raw byte buffers (`uint8_t*`) directly to struct pointers via `reinterpret_cast`. This violates strict aliasing rules and triggers unaligned memory bus errors on 32-bit architectures (Xtensa / RISC-V).
- **Rationale:** Copying into a stack-allocated struct ensures the structure is correctly aligned to word boundaries.
- **Rule:** Copy data into an aligned, stack-allocated struct via `std::memcpy`:
  ```cpp
  // AVOID: const esp_app_desc_t* desc = reinterpret_cast<const esp_app_desc_t*>(raw_buffer);

  // CORRECT:
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
Debug logging (`SYS_LOG_D`) is stripped at compile-time by default to prevent high-frequency debug logs from consuming flash space or CPU cycles in production. To enable it for a specific `.cpp` file, define `ENABLE_SYS_LOG_D` **before** including `LogHandler.h`:
```cpp
#include "mem_ota.hpp"
#include "esp_image_format.h"

// Enable debug logs for this translation unit only
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
```

### Logging Performance Rules
- **No `std::string` Concatenation:** Do not use `+` or `std::to_string()` inside `SYS_LOG_D(...)`. Use standard `printf` format specifiers so zero CPU cycles are spent when debug logging is disabled.
- **Portability:** Use standard C99 `PRI` macros for integers (e.g., `PRIu32`, `PRIx32`).
- **Zero Logging in ISR:** Never call `SYS_LOG_*` or `printf` inside an ISR. Standard logging is blocking and relies on thread-unsafe printf locks (non-reentrant).

```cpp
// BAD: CPU cycles are wasted on string allocation even if DEBUG logs are disabled
SYS_LOG_D("Socket FD: " + std::to_string(sockfd));

// GOOD: Zero allocation; optimized away at compile time if DEBUG is disabled
SYS_LOG_D("Socket FD: %d", sockfd);

// GOOD: Portably formatting hardware addresses/registers
SYS_LOG_I("Writing to partition subtype %d at offset 0x%" PRIx32, subtype, address);
```

---

## 7. Concurrency & ESP-IDF Integration

### FreeRTOS Synchronization
- **Prefer scoped RAII locks** for semaphore/mutex ownership wherever possible, rather than manual release routines on error exit blocks.
- Protect shared member variables across tasks using FreeRTOS semaphores or mutexes. When multiple tasks access or modify class member variables (arrays, state flags, socket buffers), access must be serialized with a FreeRTOS semaphore or mutex to prevent lockouts and deadlocks on critical shared resources.
- Guarantee that mutexes are released along every early-exit and error path:
  ```cpp
  if (xSemaphoreTake(_semaphore, portMAX_DELAY) != pdTRUE)
  {
      return ERROR_TIMEOUT;
  }
  _registeredUris.push_back(static_cast<HttpUri*>(&uri));
  xSemaphoreGive(_semaphore);
  return ERROR_SUCCESS;
  ```

### Interrupt Service Routine (ISR) Hard Constraints
- ❌ **No blocking calls:** Never call `vTaskDelay`, or block on mutexes/semaphores/queues.
- ❌ **No dynamic allocation:** Never call `new`, `malloc`, `delete`, `free`.
- ❌ **No standard logging:** Never call `SYS_LOG_*` or `printf`.
- ✔️ **`FromISR` APIs Only:** Use only FreeRTOS functions suffixed with `FromISR` (e.g., `xQueueSendFromISR`, `xSemaphoreGiveFromISR`).
- ✔️ **Context Switching:** Always pass `BaseType_t *pxHigherPriorityTaskWoken` to `FromISR` calls and yield if unblocked.

### SDK Callback Context Isolation
Callbacks registered to ESP-IDF drivers or protocol stacks (HTTP events, Wi-Fi events, mTLS handshakes) execute inside the underlying SDK task thread (e.g. the `httpd` task), **not** the application task. Any class member modified inside such a callback requires a mutex or `std::atomic`; pass data to the application thread via thread-safe queues instead of direct member access.
```cpp
void HttpsServer::https_server_user_callback(esp_https_server_user_cb_arg_t* user_cb)
{
    // This callback is executed inside the 'httpd' task thread context, NOT the application thread.
    switch (user_cb->user_cb_state)
    {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            // Modifying any member variable here requires a thread-safe atomic or mutex lock.
            break;
        default:
            break;
    }
}
```

### Multi-Threaded Callbacks
Callbacks from ESP-IDF protocol stacks (HTTP events, Wi-Fi events, mTLS handshakes) run in the **SDK driver task context**, not the application task. Any state updated inside an SDK callback must be protected using a mutex or `std::atomic`.

### Task Parameterization
Never hardcode task stack sizes or priorities in `.cpp` files; hardcoding can lead to stack overflows or priority inversion. Expose them via startup configuration structures (e.g., `options.task_stack_size`, `options.task_priority`), allowing them to be tuned based on the application's overall memory map.
```cpp
sys_error_t HttpsServer::populate_config(const HttpServerStartOptions_t& options, httpd_ssl_config_t& sslConfig)
{
    // Expose stack and priority options instead of hardcoding values
    sslConfig.httpd.stack_size       = options.task_stack_size;
    sslConfig.httpd.task_priority    = options.task_priority;
    sslConfig.httpd.max_open_sockets = options.max_connections;

    return ERROR_SUCCESS;
}
```

---

## 8. Doxygen Documentation Standard

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
*Note on `override`:* Concrete implementations of an interface method in `.hpp` using the `override` specifier do **not** require duplicated Doxygen comments if the base interface is already documented.

### 4. In-Body Function Flow Documentation (`///` vs `//`)
- **`///` (Triple-Slash):** Document high-level execution steps, milestones, or state transitions inside a function body. Doxygen extracts and appends these to the public **In-Body Description** section. Align `///` comments with the code indentation. Markdown syntax is supported.
- **`//` (Double-Slash):** Reserved strictly for private developer notes, register workarounds, and internal implementation details. Ignored by Doxygen.

```cpp
sys_error_t executeTransaction(const uint8_t* payload, size_t len)
{
    // Dev note: Allocating on stack to guarantee zero heap fragmentation (Ignored by Doxygen)
    uint8_t buffer[128];

    /// Validates parameters against static buffer constraints.
    if (len > sizeof(buffer))
    {
        return ERROR_INVALID_ARG;
    }

    /// @note Cryptographic hardware must be initialized prior to execution.
    /// Encrypts raw data directly within the local stack memory block.
    sys_error_t err = crypto_encrypt(payload, buffer, len);
    if (err != ERROR_SUCCESS) return err;

    /// ### Step 3: Dispatch Payload
    return network_send(buffer, len);
}
```
