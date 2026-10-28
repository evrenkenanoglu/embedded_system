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

/** INCLUDES ******************************************************************/
#include "sys_rng.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstddef>
#include <cstdint>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr uint32_t kMutexTimeoutMs = 100U;
} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/
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

/** FUNCTIONS *****************************************************************/
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

    /// Check whether the driver is already initialized.
    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_rng: already initialized");
        return ERROR_SUCCESS;
    }

    /// Allocate FreeRTOS synchronization mutex if not already created.
    if (_mutex == nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
        RETURN_IF_ERROR(_mutex != nullptr, ERROR_OUT_OF_MEMORY);
    }

    /// Mark driver state as initialized and log status.
    _initialized = HAL_INITIALIZED;
    SYS_LOG_I("sys_rng: initialized (hardware TRNG)");
    return ERROR_SUCCESS;
}

sys_error_t sys_rng::feed()
{
    /// Validate driver initialization state.
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    // The ESP32 TRNG requires no periodic hardware feed; entropy conditioning
    // is handled by the silicon peripheral itself.
    return ERROR_SUCCESS;
}

sys_error_t sys_rng::deInit()
{
    /// Return early if already deinitialized.
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    /// Delete FreeRTOS synchronization mutex.
    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    /// Reset initialized lifecycle flag.
    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_rng: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_rng::getRandomBytes(uint8_t* outputBuffer, size_t length)
{
    /// Validate driver initialization, output buffer pointer, and requested length.
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(outputBuffer != nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(length > 0U, ERROR_INVALID_ARG);

    /// Acquire serialization mutex before hardware entropy buffer access.
    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    /// Read requested random byte stream from ESP32 hardware TRNG.
    esp_fill_random(outputBuffer, length);

    /// Release serialization mutex.
    _unlock();
    return ERROR_SUCCESS;
}

uint32_t sys_rng::getRandomU32()
{
    /// Return zero immediately if driver is uninitialized.
    if (_initialized != HAL_INITIALIZED)
    {
        return 0U;
    }

    /// Acquire serialization mutex.
    if (!_lock(kMutexTimeoutMs))
    {
        return 0U;
    }

    /// Read single 32-bit random word from hardware register.
    const uint32_t value = esp_random();

    /// Release serialization mutex.
    _unlock();
    return value;
}

uint32_t sys_rng::getRandomRange(uint32_t min, uint32_t max)
{
    /// Return lower bound immediately if requested range is inverted or empty.
    if (min >= max)
    {
        return min;
    }

    /// When spanning the full 32-bit range, sample hardware TRNG directly without modulo.
    if (min == 0U && max == UINT32_MAX)
    {
        return getRandomU32();
    }

    // Rejection sampling over the full 32-bit space avoids modulo bias: the
    // limit is the largest multiple of the range that fits in 32 bits.
    const uint32_t range  = max - min + 1U;
    const uint32_t limit  = UINT32_MAX - (UINT32_MAX % range);
    uint32_t       sample = 0U;

    /// Acquire serialization mutex before reading hardware entropy.
    if (!_lock(kMutexTimeoutMs))
    {
        return min;
    }

    /// Sample hardware random register until value falls within unbiased boundary.
    do
    {
        sample = esp_random();
    } while (sample > limit);

    /// Release serialization mutex.
    _unlock();
    return min + (sample % range);
}
