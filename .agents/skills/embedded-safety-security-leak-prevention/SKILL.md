---
name: embedded-safety-security-leak-prevention
description: >-
  Code-level safety and security leak prevention for ESP32 firmware: secret and
  credential exposure in logs and memory, unsafe buffer and format-string use,
  unbounded blocking, callback and shared-state lifetime, unchecked external
  input, fail-safe load/relay behavior, and resource cleanup on error paths.
  Load alongside embedded-cpp-code-conventions whenever generating, reviewing, or
  refactoring firmware, HTTP/BLE/Matter handlers, storage, OTA, or output control.
---

# Skill: Embedded Safety & Security Leak Prevention

Use this skill with `embedded-cpp-code-conventions` for every firmware change that handles secrets, external input, shared state, storage, or a physical output (relay, LED, load). Cryptographic protocol design, secure boot, flash encryption, and anti-rollback are owned by `hardware-security-crypto-integrity`; this skill covers how application code avoids leaking data or failing unsafely.

Each rule states the leak class, the required invariant, and the verified repository example where it appears.

---

## 1. Secrets and Credentials

- **Never log secret material.** Passwords, passphrases, PEM private keys, certificates' private parts, tokens, and raw key bytes must not reach `SYS_LOG_*`, `printf`, or serialized debug output. Log the operation and the key/partition name, never the value.
  - Example: `uri_wifiConnect.cpp` logs `"Received SSID and password!"` without values. Keep that shape. Do not add `ssid` or `password` to format arguments.
- **Do not log memory addresses of secret buffers.** Pointer values reveal layout and help exploit chains.
  - Repository example to fix: `Serv_httpsServer.cpp:105-106` logs `reinterpret_cast<uintptr_t>(_sslConfig.prvtkey_pem)`. Remove the address output and log only lengths or a presence flag.
- **Zeroize secrets after use.** Any stack or heap buffer that held a password, key, or derived secret must be cleared before release, including on error paths. Use `mbedtls_platform_zeroize(buf, len)` for mbedTLS-owned or temporary buffers.
  ```cpp
  RETURN_IF_ERROR(
      (passwordLength == 0U),                                      // Expression
      ERROR_INVALID_ARG,                                           // Error Code
      mbedtls_platform_zeroize(passwordCopy, sizeof(passwordCopy)) // Cleanup
  );
  ```
- **Compare secrets and MACs in constant time.** `std::memcmp` returns early on the first mismatch, which leaks timing. Use `mbedtls_ct_memcmp` (or an equivalent constant-time routine) for tokens, MACs, digests of secrets, and password checks. Plain `memcmp` is acceptable only for non-secret data such as version strings (`mem_ota.cpp:321`).
- **Never persist secrets in plain text outside the designated secure store.** Credentials belong in the NVS/secure-storage HAL path. Do not write them to logs, debug dumps, HTTP responses, or the OTA catalog.
- **Do not echo secrets in responses.** Error messages returned to clients must describe the failure class, not reflect the submitted credential or internal state.

---

## 2. Buffers, Strings, and Format Safety

- **Forbidden APIs:** `sprintf`, `vsprintf`, `strcpy`, `strcat`, `gets`, `strtok` (not re-entrant). Use `snprintf`, `strncpy` with explicit termination, or `std::memcpy` with a checked length.
- **Always check truncation.** A `snprintf` result that is negative or `>= sizeof(buf)` means the output was cut. Treat it as an error when the truncated text is used for a decision, a key, or a path.
- **Never pass a non-literal format string.** `SYS_LOG_E(variable)` or `SYS_LOG_E(errMsg)` makes the buffer a format string. Use `SYS_LOG_E("%s", text)` when a runtime string must be printed.
  - Repository example to fix: `error_macros.h` `ON_ERROR_WITH_LOG` formats into `char errMsg[256]` with `std::sprintf` (unbounded) and then calls `SYS_LOG_E(errMsg)`. Replace with `snprintf` plus `SYS_LOG_E("%s", errMsg)`, or pass the message as a format argument directly. The 256-byte stack buffer also makes this macro unsafe in destructors, as noted in the repository memory on error handling.
- **Use the declared length, not `strlen`, for external input.** HTTP and BLE payloads carry `req_len`. Parse with the length bound, or copy into a bounded local buffer and terminate it.
  - Repository example to fix: `uri_wifiConnect.cpp` calls `cJSON_Parse(req_ptr)` and ignores `req_len`. The body is not guaranteed to be NUL-terminated.
- **Null-check before dereferencing library output.** `cJSON_PrintUnformatted` can return `NULL`, but `uri_wifiConnect.cpp:77` calls `strlen(response_str)` without checking it. Check `resp_buf` for `NULL` too.
  ```cpp
  RETURN_IF_ERROR(
      (response_str == nullptr), // Expression
      ERROR_NO_MEM,              // Error Code
      cJSON_Delete(response)     // Cleanup
  );
  ```
- **Validate external input length and range before storage.** Reject out-of-range values before persisting or acting on them:
  - SSID: 1 to 32 bytes (802.11).
  - WPA2/WPA3 passphrase: 8 to 63 ASCII characters.
  - The current `uri_wifiConnect.cpp` only rejects empty strings. Extend the check in `WifiConfigurationManager::storeCredentials` or the URI handler.
- **Cast and copy rules:** never `reinterpret_cast` a byte buffer to a struct (conventions section 5). Copy with `std::memcpy` after checking the length.

---

## 3. Bounded Blocking and Liveness

- **Mutexes take finite timeouts.** `portMAX_DELAY` on a mutex can deadlock the system if a lock holder faults or stalls. This matches the bounded-lock rule in `embedded-cpp-code-conventions` section 7.
  - Repository examples to fix: `WifiConfigurationManager.cpp` (`getStatus`, `subscribeEvent`, and the other `_statusMutex`/`_subscribersMutex` takes at lines 243, 257, 286, 403, 441, 453, 497), and `com_i2c.cpp` (lines 105, 137, 169, 225).
  - Use `pdMS_TO_TICKS(kMutexTimeoutMs)` and handle the timeout as `ERROR_TIMEOUT` or `ERROR_DEVICE_BUSY`.
- **Infinite waits are allowed only for a dedicated worker's idle wait** on its own wake source, such as `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` in a task with a defined stop path. Any such loop must still observe a stop or shutdown request.
- **Startup must not hang on an external dependency.** Waits for Wi-Fi, certificates, key generation, or the NVS partition need a bounded timeout and a defined failure outcome. Log the failure and keep the device in a safe state.
- **ISR constraints** from `embedded-cpp-code-conventions` section 7 apply to any interrupt that touches shared state, including GPIO and button handlers.

---

## 4. Callback, Subscriber, and Shared-State Lifetime

- **A callback's `user_data` must outlive its registration.** Unregister before destroying the owner or the stack-local context. The `MatterWrapper` null-unregistration fix (ERR-003 in the project integration skill) is the reference case.
- **Callbacks run on the producer's task.** Keep them brief and non-blocking. Do not call blocking APIs, take long locks, or perform flash I/O from a callback. Hand off work to the owning task through a bounded queue or task notification.
- **Read and write multi-field state under one lock.** Partial reads can show inconsistent connection or authorization state. Expose snapshots or accessor functions that take the lock.
- **Do not hold a lock while calling out** to a callback, a logger that may block, or another module that can take the same lock. Copy the data, release the lock, then call out.
- **Single owner for each output.** A GPIO, relay pin, or bus should have one writer task or one owning HAL object. Two consumers writing the same pin is a safety defect.

---

## 5. Physical Output and Fail-Safe Behavior

- **Fail-safe default for loads and relays:** the relay or load output stays de-energized until the device has established its state (restored from storage, validated the schedule and thermal status, and completed startup checks). Do not energize outputs as a side effect of task creation, queue initialization, or a generic startup default.
  - Repository example to review: `Proc_App.cpp:623` queues `ALL_SWITCHES` ON during startup. Confirm this is intended before relying on it. It is tracked in `TODOs/07-smart-plug-production-control-and-safety-integration.md`.
- **Protection trips must cut the load.** A thermal or overcurrent trip must de-energize the output, latch the fault, and require an explicit, authorized clear. Indication alone is not a protection action.
  - Repository example: the thermal callback in `main/Proc_AppTest_Ota.cpp` (about lines 726-742) only updates indication and logs. This is a gap tracked in TODO 07.
- **Faults latch.** A protection fault must survive a transient sensor reading or reconnect and must not clear itself without hysteresis rules defined in the design.
- **Sensor failure is a fault.** A failed or out-of-range sensor read must map to a defined safe state, not to the last good value.
- **Watchdog and reset paths** must not leave an output energized after an unexpected restart unless the design explicitly requires it.

---

## 6. Error Handling Without Leaks

- **Never ignore a return value** from security, storage, crypto, network, or output calls. Use `RETURN_IF_ERROR` or `RETURN_ON_ERROR` with the correct cleanup argument, following the multi-line format in `embedded-cpp-code-conventions` section 4.
- **Release every allocation on every path.** The good pattern is `uri_wifiConnect.cpp:81-91`: `free(response_str)`, `cJSON_Delete(json)`, `cJSON_Delete(response)` before the single return. Early returns after allocation must run the same cleanup. Prefer RAII wrappers for mbedTLS and FreeRTOS handles.
- **Do not continue after a failed parse or verification.** A failed signature, certificate, or JSON parse must abort the operation and return the error. Do not fall back to a default value that bypasses the check.
- **Error messages must not leak internal detail to untrusted callers.** Log details at the device level, and return a generic message to HTTP or BLE clients.

---

## 7. Review Checklist

Before completing firmware work that touches the areas above, verify:

1. No `SYS_LOG_*` or response body contains a password, key, token, PEM text, or raw secret.
2. No pointer address of a secret or key buffer is printed.
3. Secrets are zeroized on success and error paths, and secret comparisons are constant time.
4. No `sprintf`, `strcpy`, `strcat`, or non-literal format string is used. Every `snprintf` result is checked for truncation.
5. External input is length-checked with its declared length, and every library return pointer is null-checked before use.
6. Every mutex take uses a finite timeout. Infinite waits exist only in dedicated worker loops with a stop path.
7. Callback `user_data` is unregistered before its owner is destroyed. No blocking work runs inside a callback.
8. Relay and load outputs have a fail-safe default, and protection trips cut the output and latch.
9. Every allocation and handle is released on all error paths.

---

## Error Registry

Before safety or security work, review this registry and any related failure notes in applicable skills. Record verified, reusable mistakes found during the task before considering the work complete. Each entry must capture the symptom, root cause, and a preventive rule; use `ERR-NNN` IDs scoped to this file. Do not record speculation, transient output, or secrets.

No verified incidents are recorded yet. Repository-wide findings that are latent defects (for example the `ON_ERROR_WITH_LOG` buffer and the `portMAX_DELAY` mutex takes above) are tracked as code-review items, not as registry entries, until a runtime failure is reproduced.
