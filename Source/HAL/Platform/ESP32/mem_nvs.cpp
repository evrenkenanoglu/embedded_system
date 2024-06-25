/**
 * @file mem_nvs.cpp
 * @brief Source file for mem_nvs
 *
 * This file contains definitions for the mem_nvs class and related data types and functions.
 */

#include "mem_nvs.hpp"
#include <string>

mem_nvs::mem_nvs(const std::string& nvsNamespace) : _initialized(HAL_UNINITIALIZED), _namespace(nvsNamespace)
{
    // constructor implementation
}

mem_nvs::~mem_nvs()
{
    // destructor implementation
}

sys_error_t mem_nvs::init()
{
    if (_initialized == HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
        if (err != ESP_OK)
            return ERROR_FAIL;
    }

    _initialized = HAL_INITIALIZED;
    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::readData(void* addressOrKey, uint8_t* data, size_t length)
{

    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (addressOrKey == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    esp_err_t err;

    // Open NVS
    ESP_ERROR_CHECK(nvs_open(_namespace.c_str(), NVS_READONLY, &_nvshandle));

    // Create the key
    char key[NVS_KEY_NAME_MAX_SIZE - 1] = {0};

    // Copy key from addressOrKey to key
    memcpy(key, addressOrKey, NVS_KEY_NAME_MAX_SIZE - 1);

    // Read data from NVS
    err = nvs_get_blob(_nvshandle, key, (void*)data, &length);

    // If operation failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(_nvshandle);
        return ERROR_FAIL;
    }

    // Read operation successful, close NVS
    nvs_close(_nvshandle);

    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::writeData(void* addressOrKey, const uint8_t* data, size_t length)
{

    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (addressOrKey == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    esp_err_t err;

    // Open NVS
    ESP_ERROR_CHECK(nvs_open(_namespace.c_str(), NVS_READWRITE, &_nvshandle));

    // Create key
    char key[NVS_KEY_NAME_MAX_SIZE - 1] = {0};

    // Copy key from addressOrKey to key
    memcpy(key, addressOrKey, NVS_KEY_NAME_MAX_SIZE - 1);

    // Write data to NVS
    err = nvs_set_blob(_nvshandle, key, data, length);

    // If operation failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(_nvshandle);
        return ERROR_FAIL;
    }

    // Write operation successful, commit changes
    err = nvs_commit(_nvshandle);

    // If commit failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(_nvshandle);
        return ERROR_FAIL;
    }

    // Close NVS
    nvs_close(_nvshandle);
    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::erase(void* addressOrKey)
{

    if (_initialized == HAL_UNINITIALIZED)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (addressOrKey == nullptr)
    {
        return ERROR_INVALID_ARG;
    }
    esp_err_t err;

    // Open NVS
    ESP_ERROR_CHECK(nvs_open(_namespace.c_str(), NVS_READWRITE, &_nvshandle));

    // Create key
    char key[NVS_KEY_NAME_MAX_SIZE - 1] = {0};

    // Copy key from addressOrKey to key
    memcpy(key, addressOrKey, NVS_KEY_NAME_MAX_SIZE - 1);

    // Erase key from NVS
    err = nvs_erase_key(_nvshandle, key);

    // If operation failed, close NVS and return error
    if (err != ESP_OK)
    {
        nvs_close(_nvshandle);
        return ERROR_FAIL;
    }

    // Close NVS
    nvs_close(_nvshandle);

    return ERROR_SUCCESS;
}

sys_error_t mem_nvs::getSize(uint32_t* size)
{
    return ERROR_NOT_IMPLEMENTED;
}
