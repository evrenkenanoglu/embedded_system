/**
 * @file mem_nvs.hpp
 * @brief Header file for mem_nvs
 *
 * This file contains declarations for the mem_nvs class and related data types and functions.
 */

#ifndef MEM_NVS_HPP
#define MEM_NVS_HPP

#include "HAL/IHal.h"

#include "nvs.h"
#include "nvs_flash.h"
#include <string>

class mem_nvs : public IHAL_MEM
{
private:
    uint8_t _initialized;
    const std::string _namespace;
    nvs_handle_t _nvshandle;

public:
    mem_nvs(const std::string &nvsNamespace);
    ~mem_nvs();

    sys_error_t init() override;

    sys_error_t readData(void *addressOrKey, uint8_t *data, size_t length) override;

    sys_error_t writeData(void *addressOrKey, const uint8_t *data, size_t length) override;

    sys_error_t erase(void* addressOrKey) override;

    sys_error_t getSize(uint32_t *size) override;
};

#endif /* MEM_NVS_HPP */
