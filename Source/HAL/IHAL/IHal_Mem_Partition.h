/** @file       IHal_Mem_Partition.h
 *  @brief      Specialized interface for raw flash partition inspection and NVS security key access.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "IHal.h"

#include <cstddef>
#include <cstdint>

/** Size in bytes of each NVS encryption key (AES-256). */
inline constexpr size_t HAL_NVS_KEY_SIZE = 32;

/**
 * @struct hal_partition_info_t
 * @brief Platform-independent description of a flash partition.
 */
struct hal_partition_info_t
{
    uint32_t address;   ///< Flash offset of the partition.
    uint32_t size;      ///< Partition size in bytes.
    bool     encrypted; ///< True when the partition is protected by hardware flash encryption.
};

/**
 * @class IHal_Mem_Partition
 * @brief Specialization of IHAL_SYS providing read-only access to named flash partitions.
 */
class IHal_Mem_Partition : public IHAL_SYS
{
public:
    /**
     * @brief Looks up a partition by its label.
     *
     * @param[in]  label Partition label from the partition table.
     * @param[out] info  Receives the partition description on success.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_FOUND when no partition matches.
     */
    virtual sys_error_t getPartitionInfo(const char* label, hal_partition_info_t* info) = 0;

    /**
     * @brief Reads raw bytes from a partition.
     *
     * @param[in]  label  Partition label.
     * @param[in]  offset Offset from the start of the partition.
     * @param[out] buffer Destination buffer.
     * @param[in]  length Number of bytes to read.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_FOUND when no partition matches.
     */
    virtual sys_error_t readPartition(const char* label, uint32_t offset, uint8_t* buffer, size_t length) = 0;

    /**
     * @brief Retrieves the NVS encryption keys stored in an NVS keys partition.
     *
     * @param[in]  label Label of the NVS keys partition.
     * @param[out] eky   Receives the XTS encryption key (HAL_NVS_KEY_SIZE bytes).
     * @param[out] tky   Receives the XTS tweak key (HAL_NVS_KEY_SIZE bytes).
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_FOUND when the partition is missing,
     *         ERROR_NULL_POINTER when an output buffer is nullptr.
     */
    virtual sys_error_t getNvsSecurityKeys(const char* label, uint8_t* eky, uint8_t* tky) = 0;
};
