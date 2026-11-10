/** @file       mem_partition.hpp
 *  @brief      ESP32 platform implementation of IHal_Mem_Partition (flash partition access).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Mem_Partition.h"
#include "System/system.h"

/**
 * @class mem_partition
 * @brief ESP32 implementation of IHal_Mem_Partition.
 *
 * Stateless: each call resolves the partition by label through the partition table.
 */
class mem_partition : public IHal_Mem_Partition
{
public:
    mem_partition();
    ~mem_partition() override;

    mem_partition(const mem_partition&)            = delete;
    mem_partition& operator=(const mem_partition&) = delete;
    mem_partition(mem_partition&&)                 = delete;
    mem_partition& operator=(mem_partition&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t getPartitionInfo(const char* label, hal_partition_info_t* info) override;
    sys_error_t readPartition(const char* label, uint32_t offset, uint8_t* buffer, size_t length) override;
    sys_error_t getNvsSecurityKeys(const char* label, uint8_t* eky, uint8_t* tky) override;

private:
    uint8_t _initialized;
};
