/** @file       IHal_Mem_Ota.h
 *  @brief      Hardware Abstraction Layer OTA Memory Interface
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       17/06/2026
 */
#ifndef FILE_IHAL_MEM_OTA_H
#define FILE_IHAL_MEM_OTA_H

#include "System/system.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @class IHal_Mem_Ota
 * @brief Interface for Hardware Abstraction Layer (HAL) memory operations specific to OTA updates.
 */
class IHal_Mem_Ota
{
public:
    /**
     * @brief Virtual destructor for IHal_Mem_Ota.
     */
    virtual ~IHal_Mem_Ota() = default;

    /**
     * @brief Initialize the OTA Memory interface.
     *
     * @param params Pointer to initialization parameters if any.
     * @return sys_error_t The error code indicating the success or failure of the initialization.
     */
    virtual sys_error_t init(void* params = nullptr) = 0;

    /**
     * @brief Deinitialize the OTA Memory interface.
     *
     * @return sys_error_t The error code indicating the success or failure of the deinitialization.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Start the OTA process. This prepares the partition (e.g., erases it) for writing.
     *
     * @param imageSize The total expected size of the firmware binary.
     *                  Use a size or 0 / OTA_SIZE_UNKNOWN if the total size is not yet known.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t begin(size_t imageSize) = 0;

    /**
     * @brief Write a chunk of the firmware image to the OTA partition.
     *
     * @param data Pointer to the buffer containing the chunk of firmware.
     * @param length The size of the chunk in bytes.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t write(const uint8_t* data, size_t length) = 0;

    /**
     * @brief End the OTA process and validate the written partition.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t end() = 0;

    /**
     * @brief Abort the OTA process and clean up any allocated resources.
     */
    virtual sys_error_t abort() = 0;

    /**
     * @brief Set the newly written OTA partition as the active boot partition.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t setBootPartition() = 0;

    /**
     * @brief Mark the currently running application as valid and cancel rollback.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t markAppValid() = 0;

    /**
     * @brief Mark the currently running application as invalid and rollback/reboot.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t markAppInvalid() = 0;
};

#endif // FILE_IHAL_MEM_OTA_H
