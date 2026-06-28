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
#include "esp_ota_ops.h"
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

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/

mem_ota::mem_ota()
    : _updateHandle(0)
    , _updatePartition(nullptr)
    , _isInitialized(false)
    , _isOngoing(false)
    , _headerValidated(false)
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

    /// Begin the OTA update
    esp_err_t err = esp_ota_begin(_updatePartition, imageSize, &_updateHandle);
    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_begin failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    _isOngoing       = true;
    _headerValidated = false;
    SYS_LOG_I("OTA write session started successfully");
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

    // Validate the image header if we haven't done so yet
    if (!_headerValidated)
    {
        /// Validate the incoming image header
        RETURN_ON_ERROR(                                                           //
            (validateIncomingImageHeader(data, length)),                           // Expression
            abort();                                                               // Cleanup
            SYS_LOG_E("Incoming image header validation failed, aborting update"); // Error message
        );

        _headerValidated = true;
    }

    /// OTA write firmware data to partition
    const esp_err_t err = esp_ota_write(_updateHandle, data, length);

    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_write failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    return ERROR_SUCCESS;
}

sys_error_t mem_ota::end()
{
    const bool initial_check = (_isInitialized != false) && (_isOngoing != false);
    RETURN_IF_ERROR(!initial_check, ERROR_INVALID_STATE, SYS_LOG_E("Cannot end OTA: HAL is not initialized or no active session in progress"));

    /// End the OTA update and validate the written partition
    const esp_err_t err = esp_ota_end(_updateHandle);

    /// Reset internal state
    resetInternalState();

    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_end failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    SYS_LOG_I("OTA write session completed and verified");
    return ERROR_SUCCESS;
}

sys_error_t mem_ota::abort()
{
    if (!_isOngoing)
    {
        return ERROR_SUCCESS;
    }
    /// Abort the OTA process
    const esp_err_t err = esp_ota_abort(_updateHandle);

    /// Always reset internal state to ensure no dangling handles or active states remain
    resetInternalState();

    RETURN_IF_ERROR(err != ESP_OK, TRANSLATE_ERROR(err), SYS_LOG_E("esp_ota_abort failed: %s (0x%x)", ERROR_MESSAGE(err), err));

    SYS_LOG_I("OTA write session aborted");

    return ERROR_SUCCESS;
}

void mem_ota::resetInternalState()
{
    _updateHandle    = 0;
    _updatePartition = nullptr;
    _isOngoing       = false;
    _headerValidated = false;
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
