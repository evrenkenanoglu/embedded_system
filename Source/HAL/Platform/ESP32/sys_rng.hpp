/** @file       sys_rng.hpp
 *  @brief      ESP32 platform implementation of IHal_Sys_Rng (hardware TRNG).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Rng.h"
#include "System/system.h"

/**
 * @class sys_rng
 * @brief ESP32 implementation of IHal_Sys_Rng.
 *
 * Backed by the ESP32-C6 hardware true random number generator peripheral
 * (esp_random / esp_fill_random). Thread-safe: all public methods serialize
 * through an internal FreeRTOS mutex, since the TRNG buffer can be accessed
 * simultaneously by TLS worker threads and protocol tasks.
 */
class sys_rng : public IHal_Sys_Rng
{
public:
    sys_rng();
    ~sys_rng() override;

    sys_rng(const sys_rng&)            = delete;
    sys_rng& operator=(const sys_rng&) = delete;
    sys_rng(sys_rng&&)                 = delete;
    sys_rng& operator=(sys_rng&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t getRandomBytes(uint8_t* outputBuffer, size_t length) override;
    uint32_t    getRandomU32() override;
    uint32_t    getRandomRange(uint32_t min, uint32_t max) override;

private:
    bool _lock(TickType_t timeoutMs);
    void _unlock();

    uint8_t           _initialized;
    SemaphoreHandle_t _mutex;
};
