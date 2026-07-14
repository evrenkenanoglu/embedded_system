/** @file       IHal_Mem_Ota.h
 *  @brief      Hardware Abstraction Layer OTA Memory Interface
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       05/07/2026
 */

#ifndef FILE_IHAL_MEM_OTA_H
#define FILE_IHAL_MEM_OTA_H

#include "System/system.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @class IHal_Mem_Ota
 * @brief Interface for Hardware Abstraction Layer (HAL) memory operations specific to OTA updates.
 *
 * @note Thread-Safety: Access to this driver partition must be externally synchronized if accessed
 *       by multiple writer tasks.
 */
class IHal_Mem_Ota
{
public:
    virtual ~IHal_Mem_Ota() = default;

    /**
     * @brief Initialize the OTA Memory interface.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t init(void* params = nullptr) = 0;

    /**
     * @brief Deinitialize the OTA Memory interface.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Start the OTA process. This prepares the partition (e.g., erases it) for writing.
     *
     * @param[in] imageSize The total expected size of the firmware binary.
     * @return sys_error_t  ERROR_SUCCESS on success.
     */
    virtual sys_error_t begin(size_t imageSize) = 0;

    /**
     * @brief Write a chunk of the firmware image to the OTA partition.
     *
     * @param[in] data   Pointer to the buffer containing the chunk of firmware.
     * @param[in] length The size of the chunk in bytes.
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t write(const uint8_t* data, size_t length) = 0;

    /**
     * @brief End the OTA process and validate the written partition.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t end() = 0;

    /**
     * @brief Abort the OTA process and clean up any allocated resources.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t abort() = 0;

    /**
     * @brief Set the newly written OTA partition as the active boot partition.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t setBootPartition() = 0;

    /**
     * @brief Mark the currently running application as valid and cancel rollback.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t markAppValid() = 0;

    /**
     * @brief Mark the currently running application as invalid and rollback/reboot.
     *
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t markAppInvalid() = 0;

    /**
     * @brief Set the OTA mode to either full or delta update.
     *
     * @param[in] isDelta True for delta update mode, false for full update mode.
     */
    virtual void setDeltaMode(bool isDelta) = 0;

    /**
     * @brief Read a chunk of the written partition back for verification.
     *
     * @param[in]  offset   Offset within the partition to start reading.
     * @param[out] buffer   Destination buffer to store the read bytes.
     * @param[in]  length   Number of bytes to read.
     * @return sys_error_t  ERROR_SUCCESS on success.
     */
    virtual sys_error_t read(size_t offset, uint8_t* buffer, size_t length) = 0;

    /**
     * @brief Retrieves the active update partition target size.
     *
     * @return size_t Partition size in bytes.
     */
    virtual size_t getPartitionSize() const = 0;
};

#endif // FILE_IHAL_MEM_OTA_H