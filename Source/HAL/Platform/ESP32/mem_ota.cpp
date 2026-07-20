/** @file       mem_ota.cpp
 *  @brief      Memory implementation for OTA updates on ESP32.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       17/06/2026
 */

/** INCLUDES ******************************************************************/
#include "mem_ota.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <esp_image_format.h>
#include <esp_log.h>
#include <esp_partition.h>

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include <esp_efuse.h>
#endif

#include <cstring>

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

esp_err_t mem_ota::read_running_partition_cb(uint8_t* buf_p, size_t size, int src_offset, void* user_data)
{
    auto* self = static_cast<mem_ota*>(user_data);
    return self->_handleReadRunning(buf_p, size, src_offset);
}

esp_err_t mem_ota::write_target_partition_cb(const uint8_t* buf_p, size_t size, void* user_data)
{
    auto* self = static_cast<mem_ota*>(user_data);
    return self->_handleWriteTarget(buf_p, size);
}

/** FUNCTIONS *****************************************************************/

mem_ota::mem_ota()
    : _updateHandle(0)
    , _deltaOtaHandle(nullptr)
    , _updatePartition(nullptr)
    , _isInitialized(false)
    , _isOngoing(false)
    , _headerValidated(false)
    , _isDelta(false)
    , _accumulatorCount(0)
{
}

mem_ota::~mem_ota()
{
    deInit();
}

sys_error_t mem_ota::init(void* /*params*/)
{
    /// Check if already initialized
    RETURN_IF_ERROR(
        (_isInitialized),                                     // Expression
        ERROR_SUCCESS,                                        // Error code
        SYS_LOG_I("OTA Memory interface already initialized") // Error message
    );

    /// Initialize the OTA memory interface
    _isInitialized = true;

    SYS_LOG_I("OTA Memory interface initialized");
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::deInit()
{
    /// Check if already deinitialized
    if (!_isInitialized)
    {
        return ERROR_SUCCESS;
    }

    /// If an OTA session is ongoing, abort it
    abort();

    /// Reset internal state
    resetInternalState();
    _isInitialized = false;

    SYS_LOG_I("OTA Memory interface deinitialized");
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::begin(size_t imageSize)
{
    /// Check if the OTA memory interface is initialized
    RETURN_IF_ERROR(
        (_isInitialized != true),                             // Expression
        ERROR_NOT_INITIALIZED,                                // Error code
        SYS_LOG_E("Cannot begin OTA: HAL is not initialized") // Error message
    );

    /// Check if an OTA session is already ongoing
    RETURN_IF_ERROR(
        (_isOngoing),                                // Expression
        ERROR_INVALID_STATE,                         // Error code
        SYS_LOG_E("OTA session already in progress") // Error message
    );

    /// Get the next update partition and ensure it is valid
    _updatePartition = esp_ota_get_next_update_partition(nullptr);
    RETURN_IF_ERROR(
        (_updatePartition == nullptr),                                 // Expression
        ERROR_FAIL,                                                    // Error code
        SYS_LOG_E("Failed to find suitable next OTA update partition") // Error message
    );

    SYS_LOG_I("Writing to partition subtype %d at offset 0x%" PRIx32, _updatePartition->subtype, _updatePartition->address);

    sys_error_t error;
    if (_isDelta)
    {
        error = _beginDelta();
    }
    else
    {
        error = _beginFull(imageSize);
    }

    RETURN_ON_ERROR(
        (error),                   // Expression
        _updatePartition = nullptr // Cleanup
    );

    _isOngoing       = true;
    _headerValidated = false;
    SYS_LOG_I("OTA write session started successfully (Mode: %s)", _isDelta ? "Delta" : "Full");
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::write(const uint8_t* data, size_t length)
{
    /// Check if the OTA memory interface is initialized
    RETURN_IF_ERROR(
        (_isInitialized != true),                         // Expression
        ERROR_NOT_INITIALIZED,                            // Error code
        SYS_LOG_E("Cannot write: HAL is not initialized") // Error message
    );

    /// Check if an OTA session is ongoing
    RETURN_IF_ERROR(
        (_isOngoing != true),                                        // Expression
        ERROR_INVALID_STATE,                                         // Error code
        SYS_LOG_E("Cannot write: no active OTA session in progress") // Error message
    );

    /// Validate input parameters
    RETURN_IF_ERROR(
        (data == nullptr || length == 0),                    // Expression
        ERROR_INVALID_ARG,                                   // Error code
        SYS_LOG_E("Cannot write: data or length is invalid") // Error message
    );

    if (_isDelta)
    {
        return _writeDelta(data, length);
    }
    else
    {
        return _writeFull(data, length);
    }
}

sys_error_t mem_ota::end()
{
    const bool initialCheck = (_isInitialized != false) && (_isOngoing != false);
    RETURN_IF_ERROR(
        (!initialCheck),                                                                     // Expression
        ERROR_INVALID_STATE,                                                                 // Error code
        SYS_LOG_E("Cannot end OTA: HAL is not initialized or no active session in progress") // Error message
    );

    sys_error_t err;
    if (_isDelta)
    {
        err = _endDelta();
    }
    else
    {
        err = _endFull();
    }

    /// Reset internal state skipping partition reset to retain the update partition for boot setting
    resetInternalState(true);

    return err;
}

sys_error_t mem_ota::abort()
{
    if (!_isOngoing)
    {
        return ERROR_SUCCESS;
    }

    sys_error_t err;
    if (_isDelta)
    {
        err = _abortDelta();
    }
    else
    {
        err = _abortFull();
    }

    /// Always reset internal state to ensure no dangling handles or active states remain
    resetInternalState();

    return err;
}

void mem_ota::resetInternalState(bool skipPartitionReset)
{
    if (!skipPartitionReset)
    {
        _updatePartition = nullptr;
    }

    _updateHandle     = 0;
    _deltaOtaHandle   = nullptr;
    _isOngoing        = false;
    _headerValidated  = false;
    _isDelta          = false;
    _accumulatorCount = 0;
}

sys_error_t mem_ota::setBootPartition()
{
    RETURN_IF_ERROR(
        (_updatePartition == nullptr),                                      // Expression
        ERROR_INVALID_STATE,                                                // Error code
        SYS_LOG_E("Cannot set boot partition: no update partition defined") // Error message
    );

    /// Set boot partition
    const esp_err_t err = esp_ota_set_boot_partition(_updatePartition);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                                   // Expression
        TRANSLATE_ERROR(err),                                                              // Error code
        SYS_LOG_E("esp_ota_set_boot_partition failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    SYS_LOG_I("Boot partition configured to new target. Ready for reset.");

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_validateIncomingImageHeader(const uint8_t* data, size_t length)
{
    /// Check if write chunk is large enough to contain the image header, segment header, and app description
    constexpr size_t min_header_size = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
    RETURN_IF_ERROR(
        (length < min_header_size),                                                                                                 // Expression
        ERROR_INVALID_ARG,                                                                                                          // Error code
        SYS_LOG_E("First write chunk size too small for header validation: %zu bytes (min required: %zu)", length, min_header_size) // Error message
    );

    /// Copy the image header from the incoming data [2].
    // Safely copy to stack to avoid alignment and strict-aliasing issues [2]
    esp_app_desc_t       newAppInfo;
    const uint8_t* const newAppInfoPtr = data + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
    std::memcpy(&newAppInfo, newAppInfoPtr, sizeof(esp_app_desc_t));

    /// Validate the magic word in the new app description [2].
    RETURN_IF_ERROR(
        (newAppInfo.magic_word != ESP_APP_DESC_MAGIC_WORD),                                                                    // Expression
        ERROR_INVALID_STATE,                                                                                                   // Error code
        SYS_LOG_E("Invalid firmware magic word (expected 0x%08X, saw 0x%08X)", ESP_APP_DESC_MAGIC_WORD, newAppInfo.magic_word) // Error message
    );

    /// Compare version of new app against running partition description
    const esp_partition_t* running = esp_ota_get_running_partition();
    RETURN_IF_ERROR(
        (running == nullptr),                                 // Expression
        ERROR_INVALID_STATE,                                  // Error code
        SYS_LOG_E("Cannot get running partition description") // Error message
    );

    /// Retrieve the app description of the currently running partition [2].
    esp_app_desc_t runningAppInfo;
    RETURN_IF_ERROR(
        (esp_ota_get_partition_description(running, &runningAppInfo) != ESP_OK), // Expression
        ERROR_INVALID_STATE,                                                     // Error code
        SYS_LOG_E("Cannot get running partition description")                    // Error message
    );

    SYS_LOG_I("Running app version: %s", runningAppInfo.version);
    SYS_LOG_I("New app version: %s", newAppInfo.version);

    /// Check if the new firmware version is identical to the running version
    RETURN_IF_ERROR(
        (std::memcmp(newAppInfo.version, runningAppInfo.version, sizeof(newAppInfo.version)) == 0), // Expression
        ERROR_INVALID_STATE,                                                                        // Error code
        SYS_LOG_W("New firmware version is identical to the running version. Aborting update.")     // Error message
    );

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /// Check if the new firmware's secure version against the hardware eFuse secure version to prevent rollback attacks [2].
    const uint32_t hwSecVersion = esp_efuse_read_secure_version();
    RETURN_IF_ERROR(
        (newAppInfo.secure_version < hwSecVersion), // Expression
        ERROR_INVALID_STATE,                        // Error code
        SYS_LOG_E(
            "New firmware security version is lower than secure eFuse version: %" PRIu32 " < %" PRIu32, // Error message
            newAppInfo.secure_version,
            hwSecVersion));
#endif

    SYS_LOG_I("Firmware header validated successfully. Version: %s, Secure Version: %" PRIu32, newAppInfo.version, newAppInfo.secure_version);
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::markAppValid()
{
    const esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                                              // Expression
        TRANSLATE_ERROR(err),                                                                         // Error code
        SYS_LOG_E("Failed to mark app valid and cancel rollback: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    SYS_LOG_I("App marked as valid, rollback cancelled successfully");
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::markAppInvalid()
{
    SYS_LOG_W("Marking current running app as invalid and triggering rollback...");

    const esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                                     // Expression
        TRANSLATE_ERROR(err),                                                                // Error code
        SYS_LOG_E("Failed to mark app invalid/rollback: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::read(size_t offset, uint8_t* buffer, size_t length)
{
    RETURN_IF_ERROR(
        (!_isInitialized),                                      // Expression
        ERROR_NOT_INITIALIZED,                                  // Error code
        SYS_LOG_E("Cannot read partition: HAL not initialized") // Error message
    );

    RETURN_IF_ERROR(
        (_updatePartition == nullptr),                                 // Expression
        ERROR_INVALID_STATE,                                           // Error code
        SYS_LOG_E("Cannot read partition: no active partition target") // Error message
    );

    const esp_err_t err = esp_partition_read(_updatePartition, offset, buffer, length);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                           // Expression
        TRANSLATE_ERROR(err),                                                      // Error code
        SYS_LOG_E("esp_partition_read failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

size_t mem_ota::getPartitionSize() const
{
    return (_updatePartition != nullptr) ? _updatePartition->size : 0;
}

void mem_ota::setDeltaMode(bool isDelta)
{
    _isDelta = isDelta;
}

/** PRIVATE HELPER APIS (SEPARATED FULL AND DELTA OPERATIONS) *****************/

sys_error_t mem_ota::_beginFull(size_t imageSize)
{
    /// Start the standard OTA process using the ESP-IDF OTA API
    esp_err_t err = esp_ota_begin(_updatePartition, imageSize, &_updateHandle);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                      // Expression
        TRANSLATE_ERROR(err),                                                 // Error code
        SYS_LOG_E("esp_ota_begin failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_beginDelta()
{
    /// Begin standard underlying OTA sequence first (OTA_SIZE_UNKNOWN works for delta mode)
    esp_err_t err = esp_ota_begin(_updatePartition, OTA_SIZE_UNKNOWN, &_updateHandle);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                      // Expression
        TRANSLATE_ERROR(err),                                                 // Error code
        SYS_LOG_E("esp_ota_begin failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    esp_delta_ota_cfg_t cfg     = {};
    cfg.user_data               = this;
    cfg.read_cb_with_user_data  = read_running_partition_cb;
    cfg.write_cb_with_user_data = write_target_partition_cb;

    _deltaOtaHandle = esp_delta_ota_init(&cfg);

    RETURN_IF_ERROR((_deltaOtaHandle == nullptr),                                 // Expression
                    ERROR_FAIL,                                                   // Error code
                    SYS_LOG_E("Failed to initialize esp_delta_ota decompressor"), // Error message
                    esp_ota_abort(_updateHandle);                                 // Cleanup
                    _updateHandle = 0);

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_writeFull(const uint8_t* data, size_t length)
{
    if (!_headerValidated)
    {
        RETURN_ON_ERROR(_validateIncomingImageHeader(data, length), // Expression
                        abort();                                    // Cleanup
                        SYS_LOG_E("Incoming image header validation failed, aborting update"));
        _headerValidated = true;
    }

    const esp_err_t err = esp_ota_write(_updateHandle, data, length);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                      // Expression
        TRANSLATE_ERROR(err),                                                 // Error code
        SYS_LOG_E("esp_ota_write failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_writeDelta(const uint8_t* data, size_t length)
{
    /// Feed the incoming delta patch data to the Delta OTA decompressor engine
    esp_err_t err = esp_delta_ota_feed_patch(_deltaOtaHandle, data, length);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                        // Expression
        TRANSLATE_ERROR(err),                                   // Error code
        SYS_LOG_E("esp_delta_ota_feed_patch failed: 0x%x", err) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_endFull()
{
    /// Finalize the standard OTA process using the ESP-IDF OTA API
    const esp_err_t err = esp_ota_end(_updateHandle);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                    // Expression
        TRANSLATE_ERROR(err),                                               // Error code
        SYS_LOG_E("esp_ota_end failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_endDelta()
{
    esp_err_t err = esp_delta_ota_finalize(_deltaOtaHandle);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                      // Expression
        ERROR_FAIL,                                           // Error code
        SYS_LOG_E("esp_delta_ota_finalize failed: 0x%x", err) // Error message
    );

    esp_delta_ota_deinit(_deltaOtaHandle);
    _deltaOtaHandle = nullptr;

    const esp_err_t otaErr = esp_ota_end(_updateHandle);
    _updateHandle          = 0;

    RETURN_IF_ERROR(
        (otaErr != ESP_OK),                                                       // Expression
        TRANSLATE_ERROR(otaErr),                                                  // Error code
        SYS_LOG_E("esp_ota_end failed: %s (0x%x)", ERROR_MESSAGE(otaErr), otaErr) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_abortFull()
{
    const esp_err_t err = esp_ota_abort(_updateHandle);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                      // Expression
        TRANSLATE_ERROR(err),                                                 // Error code
        SYS_LOG_E("esp_ota_abort failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_abortDelta()
{
    /// If a delta OTA session is ongoing, deinitialize the Delta OTA decompressor engine
    if (_deltaOtaHandle != nullptr)
    {
        esp_delta_ota_deinit(_deltaOtaHandle);
        _deltaOtaHandle = nullptr;
    }

    const esp_err_t err = esp_ota_abort(_updateHandle);

    RETURN_IF_ERROR(
        (err != ESP_OK),                                                      // Expression
        TRANSLATE_ERROR(err),                                                 // Error code
        SYS_LOG_E("esp_ota_abort failed: %s (0x%x)", ERROR_MESSAGE(err), err) // Error message
    );

    return ERROR_SUCCESS;
}

esp_err_t mem_ota::_handleReadRunning(uint8_t* buf_p, size_t size, int src_offset)
{
    /// Read data from the currently running partition at the specified offset into the provided buffer
    const esp_partition_t* running = esp_ota_get_running_partition();

    RETURN_IF_ERROR(
        (running == nullptr),                         // Expression
        ESP_FAIL,                                     // Error code
        SYS_LOG_E("Running partition target is null") // Error message
    );

    return esp_partition_read(running, src_offset, buf_p, size);
}

esp_err_t mem_ota::_handleWriteTarget(const uint8_t* buf_p, size_t size)
{
    if (!_headerValidated)
    {
        constexpr size_t min_header_size = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);

        /// 1. Fast Path: If the single chunk is already large enough and no bytes are accumulated, bypass entirely
        if (_accumulatorCount == 0 && size >= min_header_size)
        {
            const sys_error_t err = _validateIncomingImageHeader(buf_p, size);

            RETURN_IF_ERROR(
                (err != ERROR_SUCCESS),                                     // Expression
                ESP_FAIL,                                                   // Error code
                SYS_LOG_E("Reconstructed binary header validation failed.") // Error message
            );

            _headerValidated = true;
            return esp_ota_write(_updateHandle, buf_p, size);
        }

        /// 2. Accumulate incoming micro-chunks in the temporary buffer
        size_t bytesToCopy = min_header_size - _accumulatorCount;
        if (bytesToCopy > size)
        {
            bytesToCopy = size;
        }

        std::memcpy(_headerAccumulator + _accumulatorCount, buf_p, bytesToCopy);
        _accumulatorCount += bytesToCopy;

        /// 3. Once we accumulate the minimal size, run validation and write the block
        if (_accumulatorCount >= min_header_size)
        {
            const sys_error_t err = _validateIncomingImageHeader(_headerAccumulator, _accumulatorCount);

            RETURN_IF_ERROR(
                (err != ERROR_SUCCESS),                                   // Expression
                ESP_FAIL,                                                 // Error code
                SYS_LOG_E("Accumulated binary header validation failed.") // Error message
            );

            _headerValidated = true;

            /// Write the validated accumulated block directly to standard OTA ops
            const esp_err_t otaErr = esp_ota_write(_updateHandle, _headerAccumulator, _accumulatorCount);

            RETURN_IF_ERROR(
                (otaErr != ESP_OK), // Expression
                otaErr              // Error code
            );

            /// Write any trailing data from the remaining slice of the current chunk
            if (size > bytesToCopy)
            {
                return esp_ota_write(_updateHandle, buf_p + bytesToCopy, size - bytesToCopy);
            }
        }

        return ESP_OK;
    }

    /// Standard fast path write for any blocks after validation succeeds
    return esp_ota_write(_updateHandle, buf_p, size);
}