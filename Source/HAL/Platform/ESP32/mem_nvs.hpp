/**
 * @file mem_nvs.hpp
 * @brief Header file for mem_nvs with custom and encrypted partition support.
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
    const std::string _partitionName; ///< NVS partition label (e.g. "nvs", "fctry")
    const std::string _namespace;     ///< Target NVS namespace
    nvs_handle_t      _nvshandle;

public:
    /**
     * @brief Constructs an NVS memory driver instance.
     *
     * @param[in] nvsNamespace   Target NVS namespace identifier.
     * @param[in] partitionName Target flash partition label (defaults to NVS_DEFAULT_PART_NAME).
     */
    mem_nvs(const std::string& nvsNamespace, const std::string& partitionName = NVS_DEFAULT_PART_NAME);
    ~mem_nvs();

    sys_error_t init(void* params = nullptr) override;

    sys_error_t readData(const void* addressOrKey, uint8_t* data, size_t length) override;

    sys_error_t writeData(const void* addressOrKey, const uint8_t* data, size_t length) override;

    sys_error_t erase(const void* addressOrKey) override;

    sys_error_t getSize(uint32_t* size) override;

    sys_error_t deInit() override;
};

#endif /* MEM_NVS_HPP */