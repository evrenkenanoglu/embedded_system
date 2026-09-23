/**
 * @file mem_nvs.cpp
 * @brief Source file for mem_nvs
 *
 * This file contains definitions for the mem_nvs class and related data types and functions.
 */

#include "mem_nvs.hpp"
#include <cstring>
#include <string>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <esp_partition.h>

mem_nvs::mem_nvs(const std::string& nvsNamespace, const std::string& partitionName)
    : _initialized(HAL_UNINITIALIZED)
    , _partitionName(partitionName)
    , _namespace(nvsNamespace)
    , _nvshandle(0)
{
    // constructor implementation
}

mem_nvs::~mem_nvs()
{
    // destructor implementation
}

sys_error_t mem_nvs::init(void* params)
{
    UNUSED(params);

    if (_initialized == HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    esp_err_t err = ESP_OK;

    if (_partitionName == NVS_DEFAULT_PART_NAME)
    {
        err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            err = nvs_flash_erase();
            if (err == ESP_OK)
            {
                err = nvs_flash_init();
            }
        }
    }
    else
    {
#if CONFIG_NVS_SEC_KEY_PROTECT_USING_FLASH_ENC
        // Find the nvs_keys partition for Flash Encryption protection scheme
        const esp_partition_t* keyPart = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, "nvs_keys");

        if (keyPart != nullptr)
        {
            nvs_sec_cfg_t secCfg;
            esp_err_t     secErr = nvs_flash_read_security_cfg(keyPart, &secCfg);
            if (secErr == ESP_OK)
            {
                // Secure initialization of encrypted custom partition
                err = nvs_flash_secure_init_partition(_partitionName.c_str(), &secCfg);
            }
            else
            {
                err = nvs_flash_init_partition(_partitionName.c_str());
            }
        }
        else
        {
            err = nvs_flash_init_partition(_partitionName.c_str());
        }
#else
        err = nvs_flash_init_partition(_partitionName.c_str());
#endif
    }

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                                        // Expression
        TRANSLATE_ERROR(err),                                                                   // Error code
        SYS_LOG_E("Failed to initialize NVS partition '%s': 0x%x", _partitionName.c_str(), err) // Error message
    );

    _initialized = HAL_INITIALIZED;
    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::readData(const void* addressOrKey, uint8_t* data, size_t length)
{
    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (addressOrKey == nullptr || data == nullptr || length == 0)
    {
        return ERROR_INVALID_ARG;
    }

    nvs_handle_t handle = 0;
    esp_err_t    err;

    // Open NVS
    err = nvs_open_from_partition(_partitionName.c_str(), _namespace.c_str(), NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        SYS_LOG_E("Failed to open NVS partition '%s' namespace '%s': 0x%x", _partitionName.c_str(), _namespace.c_str(), err);
        return TRANSLATE_ERROR(err);
    }

    // Create the key safely without buffer over-read
    char key[NVS_KEY_NAME_MAX_SIZE] = {0};
    strncpy(key, static_cast<const char*>(addressOrKey), NVS_KEY_NAME_MAX_SIZE - 1);

    size_t reqLen = length;

    // Read data from NVS as blob
    err = nvs_get_blob(handle, key, (void*)data, &reqLen);

    // If type mismatch, fallback to string readout (e.g. PEM certificates)
    if (err == ESP_ERR_NVS_TYPE_MISMATCH)
    {
        reqLen = length;
        err    = nvs_get_str(handle, key, reinterpret_cast<char*>(data), &reqLen);
    }

    // If operation failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return TRANSLATE_ERROR(err);
    }

    // Read operation successful, close NVS
    nvs_close(handle);

    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::writeData(const void* addressOrKey, const uint8_t* data, size_t length)
{
    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (addressOrKey == nullptr || data == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    nvs_handle_t handle = 0;
    esp_err_t    err;

    // Open NVS
    err = nvs_open_from_partition(_partitionName.c_str(), _namespace.c_str(), NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        SYS_LOG_E("Failed to open NVS partition '%s' namespace '%s': 0x%x", _partitionName.c_str(), _namespace.c_str(), err);
        return TRANSLATE_ERROR(err);
    }

    // Create key safely without buffer over-read
    char key[NVS_KEY_NAME_MAX_SIZE] = {0};
    strncpy(key, static_cast<const char*>(addressOrKey), NVS_KEY_NAME_MAX_SIZE - 1);

    // Write data to NVS
    err = nvs_set_blob(handle, key, data, length);

    // If operation failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return TRANSLATE_ERROR(err);
    }

    // Write operation successful, commit changes
    err = nvs_commit(handle);

    // If commit failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return TRANSLATE_ERROR(err);
    }

    // Close NVS
    nvs_close(handle);
    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::erase(const void* addressOrKey)
{
    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (addressOrKey == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    nvs_handle_t handle = 0;
    esp_err_t    err;

    // Open NVS
    err = nvs_open_from_partition(_partitionName.c_str(), _namespace.c_str(), NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        SYS_LOG_E("Failed to open NVS partition '%s' namespace '%s': 0x%x", _partitionName.c_str(), _namespace.c_str(), err);
        return TRANSLATE_ERROR(err);
    }

    // Create key safely without buffer over-read
    char key[NVS_KEY_NAME_MAX_SIZE] = {0};
    strncpy(key, static_cast<const char*>(addressOrKey), NVS_KEY_NAME_MAX_SIZE - 1);

    // Erase key from NVS
    err = nvs_erase_key(handle, key);

    // If operation failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return TRANSLATE_ERROR(err);
    }

    // Erase operation successful, commit changes
    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return TRANSLATE_ERROR(err);
    }

    // Close NVS
    nvs_close(handle);

    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::getSize(uint32_t* size)
{
    UNUSED(size);
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t mem_nvs::deInit()
{
    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    _initialized = HAL_UNINITIALIZED;
    return ERROR_SUCCESS;
}