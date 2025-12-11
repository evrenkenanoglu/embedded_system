/**
 * @file mem_nvs.hpp
 * @brief Header file for mem_nvs
 *
 * This file contains declarations for the mem_nvs class and related data types and functions.
 */

#ifndef MEM_NVS_HPP
#define MEM_NVS_HPP

#include "HAL/IHAL/IHal.h"

#include "nvs.h"
#include "nvs_flash.h"
#include <string>

class mem_nvs : public IHAL_MEM
{
private:
    uint8_t           _initialized;
    const std::string _namespace;
    nvs_handle_t      _nvshandle;

public:
    mem_nvs(const std::string& nvsNamespace);
    ~mem_nvs();

    sys_error_t init(void* params = nullptr) override;

    sys_error_t readData(const void *addressOrKey, uint8_t* data, size_t length) override;

    sys_error_t writeData(const void* addressOrKey, const uint8_t* data, size_t length) override;

    sys_error_t erase(const void* addressOrKey) override;

    sys_error_t getSize(uint32_t* size) override;

    sys_error_t deInit() override;
};

#endif /* MEM_NVS_HPP */
