/** @file       mem_ota.hpp
 *  @brief      Memory implementation
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       17/06/2026
 */
#ifndef MEM_OTA_HPP
#define MEM_OTA_HPP

#include "HAL/IHAL/IHal_Mem_Ota.h"
#include "esp_delta_ota.h" 
#include "esp_ota_ops.h"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/**
 * @class mem_ota
 * @brief Memory implementation
 */
class mem_ota : public IHal_Mem_Ota
{
private:
    /** VARIABLES *************************************************************/
    esp_ota_handle_t       _updateHandle;    // Handle for the OTA update process
    esp_delta_ota_handle_t _deltaOtaHandle;  // Handle for the Delta OTA update process
    const esp_partition_t* _updatePartition; // Pointer to the partition where the new firmware will be written
    bool                   _isInitialized;   // Flag indicating whether the OTA memory interface has been initialized
    bool                   _isOngoing;       // Flag indicating whether an OTA update session is currently in progress
    bool                   _headerValidated; // Flag indicating whether the incoming firmware image header has been validated
    bool                   _isDelta;         // Flag indicating whether the OTA update is a delta update

    /** PRIVATE METHODS *******************************************************/
    sys_error_t validateIncomingImageHeader(const uint8_t* data, size_t length);

    // Static callbacks to feed the dynamic decompressor engine
    static esp_err_t read_running_partition_cb(uint8_t *buf_p, size_t size, int src_offset, void *user_data);
    static esp_err_t write_target_partition_cb(const uint8_t *buf_p, size_t size, void *user_data);

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
    esp_err_t _handleReadRunning(uint8_t *buf_p, size_t size, int src_offset);
    esp_err_t _handleWriteTarget(const uint8_t *buf_p, size_t size);

public:
    mem_ota();
    ~mem_ota();

    // Disable copy mechanics to prevent duplicate handles to the same OTA session
    mem_ota(const mem_ota&)            = delete;
    mem_ota& operator=(const mem_ota&) = delete;

    // Disable move mechanics unless specifically needed
    mem_ota(mem_ota&&)            = delete;
    mem_ota& operator=(mem_ota&&) = delete;

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
    void setDeltaMode(bool isDelta) override;

public:
    /** USER METHODS *******************************************************/
    void resetInternalState(bool skipPartitionReset = false);
};

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#endif // MEM_OTA_HPP