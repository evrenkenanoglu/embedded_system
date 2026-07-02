/** @file       mem_ota.cpp
 *  @brief      Memory implementation for OTA updates on ESP32
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
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_partition.h"
#include <cstring>

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

// #define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** STATIC MEMBER FUNCTIONS ***************************************************/

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
    RETURN_IF_ERROR((_isInitialized), ERROR_SUCCESS, SYS_LOG_I("OTA Memory interface already initialized"));

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
    RETURN_IF_ERROR((_isInitialized != true), ERROR_NOT_INITIALIZED, SYS_LOG_E("Cannot begin OTA: HAL is not initialized"));

    /// Check if an OTA session is already ongoing
    RETURN_IF_ERROR((_isOngoing), ERROR_INVALID_STATE, SYS_LOG_E("OTA session already in progress"));

    // Get the next update partition and ensure it is valid
    _updatePartition = esp_ota_get_next_update_partition(nullptr);
    RETURN_IF_ERROR((_updatePartition == nullptr), ERROR_FAIL, SYS_LOG_E("Failed to find suitable next OTA update partition"));

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
    RETURN_IF_ERROR((_isInitialized != true), ERROR_NOT_INITIALIZED, SYS_LOG_E("Cannot write: HAL is not initialized"));

    /// Check if an OTA session is ongoing
    RETURN_IF_ERROR((_isOngoing != true), ERROR_INVALID_STATE, SYS_LOG_E("Cannot write: no active OTA session in progress"));

    /// Validate input parameters
    RETURN_IF_ERROR((data == nullptr || length == 0), ERROR_INVALID_ARG, SYS_LOG_E("Cannot write: data or length is invalid"));

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
    const bool initial_check = (_isInitialized != false) && (_isOngoing != false);
    RETURN_IF_ERROR(!initial_check, ERROR_INVALID_STATE, SYS_LOG_E("Cannot end OTA: HAL is not initialized or no active session in progress"));

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
        _updatePartition = nullptr;

    _updateHandle     = 0;
    _deltaOtaHandle   = nullptr;
    _isOngoing        = false;
    _headerValidated  = false;
    _isDelta          = false;
    _accumulatorCount = 0;
}

sys_error_t mem_ota::setBootPartition()
{
    RETURN_IF_ERROR((_updatePartition == nullptr), ERROR_INVALID_STATE, SYS_LOG_E("Cannot set boot partition: no update partition defined"));

    /// Set boot partition
    const esp_err_t err = esp_ota_set_boot_partition(_updatePartition);

    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_set_boot_partition failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    SYS_LOG_I("Boot partition configured to new target. Ready for reset.");

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::validateIncomingImageHeader(const uint8_t* data, size_t length)
{
    /// Check if write chunk is large enough to contain the image header, segment header, and app description
    constexpr size_t min_header_size = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
    RETURN_IF_ERROR(
        (length < min_header_size),                                                                                                 // Expression
        ERROR_INVALID_ARG,                                                                                                          // Error code
        SYS_LOG_E("First write chunk size too small for header validation: %zu bytes (min required: %zu)", length, min_header_size) // Error message
    );

    /// Copy the image header from the incoming data
    // Safely copy to stack to avoid alignment and strict-aliasing issues
    esp_app_desc_t       new_app_info;
    const uint8_t* const new_app_info_ptr = data + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
    std::memcpy(&new_app_info, new_app_info_ptr, sizeof(esp_app_desc_t));

    /// Validate the magic word in the new app description
    RETURN_IF_ERROR(
        (new_app_info.magic_word != ESP_APP_DESC_MAGIC_WORD),
        ERROR_INVALID_STATE,
        SYS_LOG_E("Invalid firmware magic word (expected 0x%08X, saw 0x%08X)", ESP_APP_DESC_MAGIC_WORD, new_app_info.magic_word));

    /// Compare version of new app against running partition description
    const esp_partition_t* running = esp_ota_get_running_partition();
    RETURN_IF_ERROR((running == nullptr), ERROR_INVALID_STATE, SYS_LOG_E("Cannot get running partition description"));

    /// Retrieve the app description of the currently running partition
    esp_app_desc_t running_app_info;
    RETURN_IF_ERROR((esp_ota_get_partition_description(running, &running_app_info) != ESP_OK), ERROR_INVALID_STATE, SYS_LOG_E("Cannot get running partition description"));

    SYS_LOG_I("Running app version: %s", running_app_info.version);
    SYS_LOG_I("New app version: %s", new_app_info.version);

    /// Check if the new firmware version is identical to the running version
    RETURN_IF_ERROR(
        (std::memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0),
        ERROR_INVALID_STATE,
        SYS_LOG_W("New firmware version is identical to the running version. Aborting update."));

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /// Check if the new firmware's secure version against the hardware eFuse secure version to prevent rollback attacks
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    RETURN_IF_ERROR(
        (new_app_info.secure_version < hw_sec_version),
        ERROR_INVALID_STATE,
        SYS_LOG_E("New firmware security version is lower than secure eFuse version: %" PRIu32 " < %" PRIu32, new_app_info.secure_version, hw_sec_version));
#endif

    SYS_LOG_I("Firmware header validated successfully. Version: %s, Secure Version: %" PRIu32, new_app_info.version, new_app_info.secure_version);
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::markAppValid()
{
    const esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("Failed to mark app valid and cancel rollback: %s (0x%x)", ERROR_MESSAGE(err), err));

    SYS_LOG_I("App marked as valid, rollback cancelled successfully");
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::markAppInvalid()
{
    SYS_LOG_W("Marking current running app as invalid and triggering rollback...");

    const esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();

    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("Failed to mark app invalid/rollback: %s (0x%x)", ERROR_MESSAGE(err), err));

    return ERROR_SUCCESS;
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
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_begin failed: %s (0x%x)", ERROR_MESSAGE(err), err));
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_beginDelta()
{
    // Begin standard underlying OTA sequence first (OTA_SIZE_UNKNOWN works for delta mode)
    esp_err_t err = esp_ota_begin(_updatePartition, OTA_SIZE_UNKNOWN, &_updateHandle);
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_begin failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    esp_delta_ota_cfg_t cfg     = {};
    cfg.user_data               = this;
    cfg.read_cb_with_user_data  = read_running_partition_cb;
    cfg.write_cb_with_user_data = write_target_partition_cb;

    _deltaOtaHandle = esp_delta_ota_init(&cfg);

    RETURN_IF_ERROR((_deltaOtaHandle == nullptr),                                 // Expression
                    ERROR_FAIL,                                                   // Error code
                    SYS_LOG_E("Failed to initialize esp_delta_ota decompressor"); // Error message
                    esp_ota_abort(_updateHandle);                                 // Cleanup
                    _updateHandle = 0;                                            // Cleanup
    );

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_writeFull(const uint8_t* data, size_t length)
{
    if (!_headerValidated)
    {
        RETURN_ON_ERROR((validateIncomingImageHeader(data, length)), abort(); SYS_LOG_E("Incoming image header validation failed, aborting update"););
        _headerValidated = true;
    }

    const esp_err_t err = esp_ota_write(_updateHandle, data, length);
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_write failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_writeDelta(const uint8_t* data, size_t length)
{
    /// Feed the incoming delta patch data to the Delta OTA decompressor engine
    esp_err_t err = esp_delta_ota_feed_patch(_deltaOtaHandle, data, length);
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_delta_ota_feed_patch failed: 0x%x", err));
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_endFull()
{
    /// Finalize the standard OTA process using the ESP-IDF OTA API
    const esp_err_t err = esp_ota_end(_updateHandle);
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_end failed: %s (0x%x)", ERROR_MESSAGE(err), err));
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_endDelta()
{
    esp_err_t err = esp_delta_ota_finalize(_deltaOtaHandle);
    RETURN_IF_ERROR((err != ESP_OK), ERROR_FAIL, SYS_LOG_E("esp_delta_ota_finalize failed: 0x%x", err));

    esp_delta_ota_deinit(_deltaOtaHandle);
    _deltaOtaHandle = nullptr;

    const esp_err_t ota_err = esp_ota_end(_updateHandle);
    _updateHandle           = 0;

    RETURN_IF_ERROR(ota_err != ESP_OK, TRANSLATE_ERROR(ota_err), SYS_LOG_E("esp_ota_end failed: %s (0x%x)", ERROR_MESSAGE(ota_err), ota_err));
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("Delta patch finalization failed."));

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::_abortFull()
{
    const esp_err_t err = esp_ota_abort(_updateHandle);
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_abort failed: %s (0x%x)", ERROR_MESSAGE(err), err));
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
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_abort failed: %s (0x%x)", ERROR_MESSAGE(err), err));
    return ERROR_SUCCESS;
}

esp_err_t mem_ota::_handleReadRunning(uint8_t* buf_p, size_t size, int src_offset)
{
    /// Read data from the currently running partition at the specified offset into the provided buffer
    const esp_partition_t* running = esp_ota_get_running_partition();
    RETURN_IF_ERROR((running == nullptr), ESP_FAIL);

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
            const sys_error_t err = validateIncomingImageHeader(buf_p, size);
            RETURN_IF_ERROR((err != ERROR_SUCCESS), ESP_FAIL, SYS_LOG_E("Reconstructed binary header validation failed."));

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
            const sys_error_t err = validateIncomingImageHeader(_headerAccumulator, _accumulatorCount);
            RETURN_IF_ERROR((err != ERROR_SUCCESS), ESP_FAIL, SYS_LOG_E("Accumulated binary header validation failed."));
            _headerValidated = true;

            /// Write the validated accumulated block directly to standard OTA ops
            const esp_err_t ota_err = esp_ota_write(_updateHandle, _headerAccumulator, _accumulatorCount);
            RETURN_IF_ERROR((ota_err != ESP_OK), ota_err);

            /// Write any trailing data from the remaining slice of the current chunk
            if (size > bytesToCopy)
            {
                return esp_ota_write(_updateHandle, buf_p + bytesToCopy, size - bytesToCopy);
            }
        }

        return ESP_OK;
    }

    // Standard fast path write for any blocks after validation succeeds
    return esp_ota_write(_updateHandle, buf_p, size);
}