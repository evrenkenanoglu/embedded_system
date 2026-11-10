/** @file       sys_reset.cpp
 *  @brief      ESP32 platform implementation of IHal_Sys_Reset (software system restart).
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
#include "sys_reset.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_system.h>

/** FUNCTIONS *****************************************************************/
sys_reset::sys_reset()
    : _initialized(HAL_UNINITIALIZED)
{
}

sys_reset::~sys_reset()
{
    (void)deInit();
}

sys_error_t sys_reset::init(void* params)
{
    (void)params;

    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_reset: already initialized");
        return ERROR_SUCCESS;
    }

    _initialized = HAL_INITIALIZED;
    SYS_LOG_D("sys_reset: initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_reset::feed()
{
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    return ERROR_SUCCESS;
}

sys_error_t sys_reset::deInit()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_reset: de-initialized");
    return ERROR_SUCCESS;
}

void sys_reset::restartSystem()
{
    SYS_LOG_W("sys_reset: restarting system");
    esp_restart();
}
