/** @file       IHal_Sys_Rng.h
 *  @brief      Specialized interface for hardware true random number generator operations.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "IHal.h"

#include <cstddef>
#include <cstdint>

/**
 * @class IHal_Sys_Rng
 * @brief Specialization of IHAL_SYS providing hardware entropy generation.
 *
 * Consumed by the PAL layer (crypto engines) and the application layer
 * (OTA jitter) to obtain cryptographically secure random data from the
 * silicon true random number generator peripheral.
 */
class IHal_Sys_Rng : public IHAL_SYS
{
public:
    /**
     * @brief Fills a caller-provided buffer with random bytes.
     *
     * @param[out] outputBuffer Pointer to the buffer receiving random data.
     * @param[in]  length       Number of random bytes to generate.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NULL_POINTER when
     *         outputBuffer is nullptr, ERROR_INVALID_ARG when length is zero.
     */
    virtual sys_error_t getRandomBytes(uint8_t* outputBuffer, size_t length) = 0;

    /**
     * @brief Generates a single random 32-bit word.
     *
     * @return uint32_t Random 32-bit value.
     */
    virtual uint32_t getRandomU32() = 0;

    /**
     * @brief Generates a random value in the inclusive range [min, max].
     *
     * Uses rejection sampling to avoid modulo bias. When min >= max, min is
     * returned.
     *
     * @param[in] min Lower bound of the range (inclusive).
     * @param[in] max Upper bound of the range (inclusive).
     *
     * @return uint32_t Random value within the requested range.
     */
    virtual uint32_t getRandomRange(uint32_t min, uint32_t max) = 0;

    virtual ~IHal_Sys_Rng() = default;
};
