/** @file       mem_partition.cpp
 *  @brief      ESP32 platform implementation of IHal_Mem_Partition (flash partition access).
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
#include "mem_partition.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_partition.h>
#include <nvs_flash.h>

#include <cstring>

/** FUNCTIONS *****************************************************************/
mem_partition::mem_partition()
    : _initialized(HAL_UNINITIALIZED)
{
}

mem_partition::~mem_partition()
{
    (void)deInit();
}

sys_error_t mem_partition::init(void* params)
{
    (void)params;

    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("mem_partition: already initialized");
        return ERROR_SUCCESS;
    }

    _initialized = HAL_INITIALIZED;
    SYS_LOG_D("mem_partition: initialized");
    return ERROR_SUCCESS;
}

sys_error_t mem_partition::feed()
{
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    return ERROR_SUCCESS;
}

sys_error_t mem_partition::deInit()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("mem_partition: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t mem_partition::getPartitionInfo(const char* label, hal_partition_info_t* info)
{
    RETURN_IF_ERROR(label == nullptr || info == nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, label);
    RETURN_IF_ERROR(part == nullptr, ERROR_NOT_FOUND);

    info->address   = part->address;
    info->size      = part->size;
    info->encrypted = part->encrypted;
    return ERROR_SUCCESS;
}

sys_error_t mem_partition::readPartition(const char* label, uint32_t offset, uint8_t* buffer, size_t length)
{
    RETURN_IF_ERROR(label == nullptr || buffer == nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, label);
    RETURN_IF_ERROR(part == nullptr, ERROR_NOT_FOUND);

    const esp_err_t err = esp_partition_read(part, offset, buffer, length);
    if (err != ESP_OK)
    {
        SYS_LOG_E("mem_partition: read of '%s' failed (0x%x)", label, err);
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t mem_partition::getNvsSecurityKeys(const char* label, uint8_t* eky, uint8_t* tky)
{
    RETURN_IF_ERROR(label == nullptr || eky == nullptr || tky == nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(_initialized != HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, label);
    RETURN_IF_ERROR(part == nullptr, ERROR_NOT_FOUND);

    nvs_sec_cfg_t   secCfg{};
    const esp_err_t err = nvs_flash_read_security_cfg(part, &secCfg);
    if (err != ESP_OK)
    {
        SYS_LOG_E("mem_partition: nvs_flash_read_security_cfg failed (0x%x)", err);
        return ERROR_FAIL;
    }

    std::memcpy(eky, secCfg.eky, HAL_NVS_KEY_SIZE);
    std::memcpy(tky, secCfg.tky, HAL_NVS_KEY_SIZE);
    return ERROR_SUCCESS;
}
