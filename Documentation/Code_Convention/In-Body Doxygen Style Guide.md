### `7. Doxygen and Documentation Standards.md`

Use this standard to write file headers, public APIs, and in-body logic comments to generate a structured, comprehensive Doxygen developer reference.

---

#### 1. File-Level Header Blocks
Every file (`.hpp`, `.cpp`, `.h`) must begin with a file-level block containing metadata and licensing information.

*   **Rule:** Place the file block at the very top of the file, preceding any `#pragma once` or include directives.
*   **Template:**
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

---

#### 2. Class-Level Documentation
Document the scope and thread-safety invariants of every class or interface.

*   **Rule:** Position the class documentation block directly above the class declaration.
*   **Template:**
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

---

#### 3. Method-Level Documentation
Every public method in a header or interface must be fully documented using parameters and return tag structures.

*   **Rule:** 
    *   Document the behavior, pre-conditions, and post-conditions of the method.
    *   Prefix parameters with direction tags: `[in]` (read-only), `[out]` (written by function), or `[in,out]` (read and written).
*   **Template:**
    ```cpp
    /**
     * @brief Deinitialize the communication interface and release associated hardware.
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

---

#### 4. In-Body Function Flow Documentation
Use this standard when writing or refactoring function implementation code to document internal logical steps.

*   **Rule 1: Syntax Differentiation:**
    *   **`///` (Triple-slash):** Use inside function bodies for high-level execution steps, milestones, or state transitions. Doxygen extracts and appends these to the public **In-Body Description** section of the function.
    *   **`//` (Double-slash):** Use for strictly internal developer notes (e.g., register workarounds, memory limits, TODOs). Doxygen ignores these.
*   **Rule 2: Scope Indentation:** Align `///` comments directly with the indentation of the surrounding code block.
*   **Rule 3: Markdown Support:** Use standard markdown (headers, bullet points, backticks for code) and Doxygen tags within the comments.

*   **Structured Implementation Template:**
    ```cpp
    sys_error_t executeTransaction(const uint8_t* payload, size_t len)
    {
        // Dev note: Allocating on stack to guarantee zero heap fragmentation (Ignored by Doxygen)
        uint8_t buffer[128]; 

        /// ### Step 1: Input Validation
        /// Validates parameters against static buffer constraints.
        if (len > sizeof(buffer)) 
        {
            return ERROR_INVALID_ARG;
        }

        /// ### Step 2: Payload Cryptography
        /// @note Cryptographic hardware must be initialized prior to execution.
        /// Encrypts raw data directly within the local stack memory block.
        sys_error_t err = crypto_encrypt(payload, buffer, len);
        if (err != ERROR_SUCCESS) return err;

        /// ### Step 3: Dispatch Payload
        return network_send(buffer, len);
    }
    ```

---

#### 5. Section Separator Blocks (C++ Implementation Files)
Organize the contents of source files (`.cpp`) and old-style headers (`.h`) using uppercase section separators to keep declaration blocks cleanly partitioned.

*   **Rule:** Use these separator comments to group standard file blocks:
    ```cpp
    /** INCLUDES ******************************************************************/

    /** CONSTANTS *****************************************************************/

    /** TYPEDEFS ******************************************************************/

    /** MACROS ********************************************************************/

    /** VARIABLES *****************************************************************/

    /** LOCAL FUNCTIONS ***********************************************************/

    /** FUNCTIONS *****************************************************************/
    ```