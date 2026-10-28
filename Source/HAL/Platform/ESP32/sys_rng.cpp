/** @file       sys_rng.cpp
 *  @brief      ESP32 platform implementation of IHal_Sys_Rng (hardware TRNG).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#define ENABLE_SYS_LOG_D

/*** INCLUDES ***/
#include "sys_rng.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_random.h>
/*** INCLUDES ***/

/*** CONSTANTS ***/
namespace
{
    constexpr uint32_t kMutexTimeoutMs = 100U;
} // namespace
/*** CONSTANTS ***/

/*** LOCAL FUNCTIONS ***/
bool sys_rng::_lock(TickType_t timeoutMs)
{
    if (_mutex == nullptr)
    {
        return false;
    }
    return xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void sys_rng::_unlock()
{
    if (_mutex != nullptr)
    {
        (void)xSemaphoreGive(_mutex);
    }
}
/*** LOCAL FUNCTIONS ***/

/*** FUNCTIONS ***/
sys_rng::sys_rng()
    : _initialized(HAL_UNINITIALIZED)
    , _mutex(nullptr)
{
}

sys_rng::~sys_rng()
{
    (void)deInit();
}

sys_error_t sys_rng::init(void* params)
{
    (void)params;

    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_rng: already initialized");
        return ERROR_SUCCESS;
    }

    if (_mutex == nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
        RETURN_IF_ERROR(_mutex != nullptr, ERROR_OUT_OF_MEMORY);
    }

    _initialized = HAL_INITIALIZED;
    SYS_LOG_I("sys_rng: initialized (hardware TRNG)");
    return ERROR_SUCCESS;
}

sys_error_t sys_rng::feed()
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    // The ESP32 TRNG requires no periodic hardware feed; entropy conditioning
    // is handled by the silicon peripheral itself.
    return ERROR_SUCCESS;
}

sys_error_t sys_rng::deInit()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_rng: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_rng::getRandomBytes(uint8_t* outputBuffer, size_t length)
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(outputBuffer != nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(length > 0U, ERROR_INVALID_ARG);

    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    esp_fill_random(outputBuffer, length);

    _unlock();
    return ERROR_SUCCESS;
}

uint32_t sys_rng::getRandomU32()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return 0U;
    }

    if (!_lock(kMutexTimeoutMs))
    {
        return 0U;
    }

    const uint32_t value = esp_random();

    _unlock();
    return value;
}

uint32_t sys_rng::getRandomRange(uint32_t min, uint32_t max)
{
    if (min >= max)
    {
        return min;
    }

    // Rejection sampling over the full 32-bit space avoids modulo bias: the
    // limit is the largest multiple of the range that fits in 32 bits.
    const uint32_t range  = max - min + 1U;
    const uint32_t limit  = UINT32_MAX - (UINT32_MAX % range);
    uint32_t       sample = 0U;

    if (!_lock(kMutexTimeoutMs))
    {
        return min;
    }

    do
    {
        sample = esp_random();
    } while (sample > limit);

    _unlock();
    return min + (sample % range);
}
/*** FUNCTIONS ***/
