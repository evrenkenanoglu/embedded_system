/** @file       sys_appInfo.cpp
 *  @brief      ESP32 platform implementation of IHal_Sys_AppInfo (running image descriptor).
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
#include "sys_appInfo.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_app_desc.h>

/** FUNCTIONS *****************************************************************/
sys_appInfo::sys_appInfo()
    : _initialized(HAL_UNINITIALIZED)
{
}

sys_appInfo::~sys_appInfo()
{
    (void)deInit();
}

sys_error_t sys_appInfo::init(void* params)
{
    (void)params;

    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_appInfo: already initialized");
        return ERROR_SUCCESS;
    }

    _initialized = HAL_INITIALIZED;
    SYS_LOG_D("sys_appInfo: initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_appInfo::feed()
{
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    return ERROR_SUCCESS;
}

sys_error_t sys_appInfo::deInit()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_appInfo: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_appInfo::getRunningAppInfo(hal_app_info_t* info)
{
    RETURN_IF_ERROR(info == nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    const esp_app_desc_t* desc = esp_app_get_description();
    RETURN_IF_ERROR(desc == nullptr, ERROR_NOT_FOUND);

    info->version       = desc->version;
    info->secureVersion = desc->secure_version;
    return ERROR_SUCCESS;
}
