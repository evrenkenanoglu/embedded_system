/** @file       DsEngine.hpp
 *  @brief      Hardware Digital Signature (DS) peripheral driver wrapping esp_ds.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       19/09/2026
 */

#pragma once

// 1. Local Project / Protocol / HAL Headers
#include "System/system.h"

// 2. Third-Party / ESP-IDF SDK Headers
#include <esp_ds.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 3. C++ Standard Library Headers
#include <cstddef>
#include <cstdint>

/**
 * @class DsEngine
 * @brief Thread-safe hardware driver executing digital signatures via silicon eFuse HMAC keys.
 *
 * @note Resource Safety: Enforces RAII, Rule of Five (deleted copy/move), and FreeRTOS mutex locking.
 */
class DsEngine
{
public:
    struct DsConfig_t
    {
        const esp_ds_data_t* dsContext;    ///< Pointer to valid ESP-DS encrypted key parameter context
        hmac_key_id_t        hmacKeyId;    ///< Silicon eFuse HMAC key ID (HMAC_KEY0 through HMAC_KEY5)
        size_t               rsaBitLength; ///< RSA key bit length (e.g. 2048, 3072, 4096)
    };

private:
    DsConfig_t        _config;
    SemaphoreHandle_t _mutex;
    bool              _isInitialized;

public:
    DsEngine();
    ~DsEngine();

    // Rule of Five: Delete copy and move semantics for hardware driver wrappers
    DsEngine(const DsEngine&)            = delete;
    DsEngine& operator=(const DsEngine&) = delete;
    DsEngine(DsEngine&&)                 = delete;
    DsEngine& operator=(DsEngine&&)      = delete;

public:
    /**
     * @brief Initializes the Digital Signature hardware peripheral configuration context.
     *
     * @param[in] config Configuration parameters containing DS context and eFuse HMAC key slot.
     * @return sys_error_t ERROR_SUCCESS on successful initialization.
     */
    sys_error_t init(const DsConfig_t& config);

    /**
     * @brief Deinitializes the DS engine and releases internal synchronization primitives.
     *
     * @return sys_error_t ERROR_SUCCESS on successful deinitialization.
     */
    sys_error_t deInit();

    /**
     * @brief Computes a hardware-accelerated signature over a pre-calculated message digest.
     *
     * @param[in]  messageDigest Buffer containing the hash/digest to sign (e.g. 32 bytes for SHA-256).
     * @param[in]  digestLen     Length of the digest in bytes.
     * @param[out] outSignature  Target buffer allocated to receive the raw signature (min rsaBitLength / 8 bytes).
     * @param[out] outSigLen     Populated with the exact written signature length in bytes.
     *
     * @return sys_error_t       ERROR_SUCCESS on success, otherwise an error status code.
     */
    sys_error_t sign(
        const uint8_t* messageDigest, //
        size_t         digestLen,     //
        uint8_t*       outSignature,  //
        size_t&        outSigLen      //
    );

    /**
     * @brief Checks if the DS hardware engine is currently initialized and configured.
     *
     * @return true if initialized, false otherwise.
     */
    bool isInitialized() const;
};