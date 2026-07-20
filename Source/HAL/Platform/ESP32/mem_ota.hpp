/** @file       mem_ota.hpp
 *  @brief      Platform-specific flash partition driver for ESP32 OTA memory operations.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       17/06/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Mem_Ota.h"

#include <esp_delta_ota.h>
#include <esp_ota_ops.h>

/**
 * @class mem_ota
 * @brief Platform-specific flash driver implementing physical sector OTA writing on ESP32.
 *
 * @note Thread-Safety: This class is not thread-safe and must be accessed exclusively from
 *       a single task context.
 */
class mem_ota : public IHal_Mem_Ota
{
private:
    esp_ota_handle_t       _updateHandle;           ///< Handle for the OTA update process
    esp_delta_ota_handle_t _deltaOtaHandle;         ///< Handle for the Delta OTA update process
    const esp_partition_t* _updatePartition;        ///< Pointer to the partition where the new firmware will be written
    bool                   _isInitialized;          ///< Flag indicating whether the OTA memory interface has been initialized
    bool                   _isOngoing;              ///< Flag indicating whether an OTA update session is currently in progress
    bool                   _headerValidated;        ///< Flag indicating whether the incoming firmware image header has been validated
    bool                   _isDelta;                ///< Flag indicating whether the OTA update is a delta update
    uint8_t                _headerAccumulator[320]; ///< Temp buffer to hold headers during micro-sized delta writes (min required: 288)
    size_t                 _accumulatorCount;       ///< Number of bytes currently held inside the accumulator

private:
    /**
     * @brief Cryptographically and structurally validates the incoming firmware image header.
     *
     * @param[in] data   Pointer to the start of the firmware image data.
     * @param[in] length Length of the available header data buffer.
     * 
     * @return sys_error_t ERROR_SUCCESS on successful validation.
     */
    sys_error_t _validateIncomingImageHeader(const uint8_t* data, size_t length);

    // Static callbacks to feed the dynamic decompressor engine
    static esp_err_t read_running_partition_cb(uint8_t* buf_p, size_t size, int src_offset, void* user_data);
    static esp_err_t write_target_partition_cb(const uint8_t* buf_p, size_t size, void* user_data);

    // Separated Standard (Full) private execution operations
    sys_error_t _beginFull(size_t imageSize);
    sys_error_t _writeFull(const uint8_t* data, size_t length);
    sys_error_t _endFull();
    sys_error_t _abortFull();

    // Separated Delta update private execution operations
    sys_error_t _beginDelta();
    sys_error_t _writeDelta(const uint8_t* data, size_t length);
    sys_error_t _endDelta();
    sys_error_t _abortDelta();

    // Decompressor read/write implementation handlers
    esp_err_t _handleReadRunning(uint8_t* buf_p, size_t size, int src_offset);
    esp_err_t _handleWriteTarget(const uint8_t* buf_p, size_t size);

public:
    mem_ota();
    ~mem_ota();

    // Disable copy mechanics to prevent duplicate handles to the same OTA session
    mem_ota(const mem_ota&)            = delete;
    mem_ota& operator=(const mem_ota&) = delete;

    // Disable move mechanics unless specifically needed
    mem_ota(mem_ota&&)            = delete;
    mem_ota& operator=(mem_ota&&) = delete;

public:
    /** INTERFACE METHODS *****************************************************/
    sys_error_t init(void* params = nullptr) override;
    sys_error_t deInit() override;
    sys_error_t begin(size_t imageSize) override;
    sys_error_t write(const uint8_t* data, size_t length) override;
    sys_error_t end() override;
    sys_error_t abort() override;
    sys_error_t setBootPartition() override;
    sys_error_t markAppValid() override;
    sys_error_t markAppInvalid() override;
    void        setDeltaMode(bool isDelta) override;
    sys_error_t read(size_t offset, uint8_t* buffer, size_t length) override;
    size_t      getPartitionSize() const override;

public:
    /**
     * @brief Resets the internal state properties of the driver.
     *
     * @param[in] skipPartitionReset Set to true to preserve the active update partition target.
     */
    void resetInternalState(bool skipPartitionReset = false);
};