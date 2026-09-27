/** @file       sys_wdt.cpp
 *  @brief      ESP32 platform implementation for IHal_Sys_Wdt.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *  @author     Evren Kenanoglu
 *  @date       27/09/2026
 */

#include "sys_wdt.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

sys_wdt::sys_wdt()
    : _initialized(HAL_UNINITIALIZED)
{
}

sys_wdt::~sys_wdt()
{
    deInit();
}

sys_error_t sys_wdt::init(void* params)
{
    UNUSED(params);
    _initialized = HAL_INITIALIZED;
    return ERROR_SUCCESS;
}

sys_error_t sys_wdt::deInit()
{
    _initialized = HAL_UNINITIALIZED;
    return ERROR_SUCCESS;
}

sys_error_t sys_wdt::registerCurrentTask()
{
    const esp_err_t err = esp_task_wdt_add(NULL);
    if (err == ESP_ERR_INVALID_STATE)
    {
        return ERROR_SUCCESS;
    }

    RETURN_IF_ERROR(
        (err != ESP_OK),
        TRANSLATE_ERROR(err),
        SYS_LOG_W("esp_task_wdt_add failed: %s (0x%x)", ERROR_MESSAGE(err), err)
    );

    return ERROR_SUCCESS;
}

sys_error_t sys_wdt::feed()
{
    const esp_err_t err = esp_task_wdt_reset();
    if (err == ESP_ERR_NOT_FOUND)
    {
        return ERROR_SUCCESS;
    }

    RETURN_IF_ERROR(
        (err != ESP_OK),
        TRANSLATE_ERROR(err),
        SYS_LOG_W("esp_task_wdt_reset failed: %s (0x%x)", ERROR_MESSAGE(err), err)
    );

    return ERROR_SUCCESS;
}

sys_error_t sys_wdt::unregisterCurrentTask()
{
    const esp_err_t err = esp_task_wdt_delete(NULL);
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_INVALID_STATE)
    {
        return ERROR_SUCCESS;
    }

    RETURN_IF_ERROR(
        (err != ESP_OK),
        TRANSLATE_ERROR(err),
        SYS_LOG_W("esp_task_wdt_delete failed: %s (0x%x)", ERROR_MESSAGE(err), err)
    );

    return ERROR_SUCCESS;
}

void sys_wdt::yield(uint32_t delayTicks)
{
    vTaskDelay(pdMS_TO_TICKS(delayTicks));
}
