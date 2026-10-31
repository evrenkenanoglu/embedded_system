/** @file       io_adc.cpp
 *  @brief      ESP32 ADC One-Shot and calibration implementation.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

#define ENABLE_SYS_LOG_D

/** INCLUDES ******************************************************************/
#include "io_adc.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_adc/adc_cali_scheme.h>

#include <cstdint>
#include <limits>

/** CONSTANTS *****************************************************************/
namespace
{
    bool toAdcUnit(uint8_t unitIndex, adc_unit_t& outUnit)
    {
        switch (unitIndex)
        {
            case 0U:
                outUnit = ADC_UNIT_1;
                return true;
            case 1U:
                outUnit = ADC_UNIT_2;
                return true;
            default:
                return false;
        }
    }

    bool toAdcAttenuation(hal_adc_attenuation_t attenuation, adc_atten_t& outAttenuation)
    {
        switch (attenuation)
        {
            case hal_adc_attenuation_t::DEFAULT:
            case hal_adc_attenuation_t::DB_12:
                outAttenuation = ADC_ATTEN_DB_12;
                return true;
            case hal_adc_attenuation_t::DB_0:
                outAttenuation = ADC_ATTEN_DB_0;
                return true;
            case hal_adc_attenuation_t::DB_2_5:
                outAttenuation = ADC_ATTEN_DB_2_5;
                return true;
            case hal_adc_attenuation_t::DB_6:
                outAttenuation = ADC_ATTEN_DB_6;
                return true;
            default:
                return false;
        }
    }

    bool toAdcBitWidth(uint8_t resolutionBits, adc_bitwidth_t& outBitWidth)
    {
        switch (resolutionBits)
        {
            case 0U:
                outBitWidth = ADC_BITWIDTH_DEFAULT;
                return true;
            case 9U:
                outBitWidth = ADC_BITWIDTH_9;
                return true;
            case 10U:
                outBitWidth = ADC_BITWIDTH_10;
                return true;
            case 11U:
                outBitWidth = ADC_BITWIDTH_11;
                return true;
            case 12U:
                outBitWidth = ADC_BITWIDTH_12;
                return true;
            default:
                return false;
        }
    }
} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/
sys_error_t io_adc::_cleanup()
{
    /// Mark the driver inactive before releasing any partially initialized resources.
    _initialized = HAL_UNINITIALIZED;

    /// Delete calibration state before releasing the ADC unit it references.
    if (_calibrationHandle != nullptr)
    {
        const esp_err_t err = adc_cali_delete_scheme_curve_fitting(_calibrationHandle);
        RETURN_IF_ERROR(
            (err != ESP_OK),                                                                               // Expression
            TRANSLATE_ERROR(err),                                                                          // Error Code
            SYS_LOG_E("io_adc: calibration cleanup failed (esp_err=0x%x)", static_cast<unsigned int>(err)) // Log Message
        );
        _calibrationHandle = nullptr;
    }

    /// Keep the ADC unit alive if its calibration handle could not be released.
    if (_unitHandle != nullptr)
    {
        const esp_err_t err = adc_oneshot_del_unit(_unitHandle);
        RETURN_IF_ERROR(
            (err != ESP_OK),                                                                        // Expression
            TRANSLATE_ERROR(err),                                                                   // Error Code
            SYS_LOG_E("io_adc: unit cleanup failed (esp_err=0x%x)", static_cast<unsigned int>(err)) // Log Message
        );
        _unitHandle = nullptr;
    }

    return ERROR_SUCCESS;
}

/** FUNCTIONS *****************************************************************/
io_adc::io_adc()
    : _initialized(HAL_UNINITIALIZED)
    , _unitHandle(nullptr)
    , _calibrationHandle(nullptr)
    , _config{}
{
}

io_adc::~io_adc()
{
    ON_ERROR_WITH_LOG(
        deInit(),                                  // Expression
        "io_adc cleanup failed during destruction" // Log Message
    );
}

sys_error_t io_adc::init(void* params)
{
    RETURN_IF_ERROR(params == nullptr, ERROR_NULL_POINTER);
    return init(*static_cast<const hal_io_adc_config_t*>(params));
}

sys_error_t io_adc::init(const hal_io_adc_config_t& config)
{
    /// Validate the requested unit, attenuation, and resolution before allocating resources.
    adc_unit_t unit = ADC_UNIT_1;
    RETURN_IF_ERROR(
        (!toAdcUnit(config.unit_index, unit)),                                                       // Expression
        ERROR_NOT_SUPPORTED,                                                                         // Error Code
        SYS_LOG_E("io_adc: unsupported ADC unit index %u", static_cast<unsigned>(config.unit_index)) // Log Message
    );

    adc_atten_t attenuation = ADC_ATTEN_DB_12;
    RETURN_IF_ERROR(
        (!toAdcAttenuation(config.attenuation, attenuation)), // Expression
        ERROR_INVALID_ARG,                                    // Error Code
        SYS_LOG_E("io_adc: invalid attenuation selection")    // Log Message
    );

    adc_bitwidth_t bitWidth = ADC_BITWIDTH_DEFAULT;
    RETURN_IF_ERROR(
        (!toAdcBitWidth(config.resolution_bits, bitWidth)),                                               // Expression
        ERROR_NOT_SUPPORTED,                                                                              // Error Code
        SYS_LOG_E("io_adc: unsupported ADC resolution %u", static_cast<unsigned>(config.resolution_bits)) // Log Message
    );

    /// Reject reconfiguration of an active ADC.
    if (_initialized == HAL_INITIALIZED)
    {
        const bool sameConfig = (_config.unit_index == config.unit_index) && (_config.channel == config.channel) && (_config.resolution_bits == config.resolution_bits) &&
                                (_config.attenuation == config.attenuation);
        RETURN_IF_ERROR(
            (!sameConfig),                                                         // Expression
            ERROR_INVALID_STATE,                                                   // Error Code
            SYS_LOG_E("io_adc: cannot change ADC configuration while initialized") // Log Message
        );
        return ERROR_SUCCESS;
    }
    RETURN_IF_ERROR(
        (_unitHandle != nullptr || _calibrationHandle != nullptr),                      // Expression
        ERROR_INVALID_STATE,                                                            // Error Code
        SYS_LOG_E("io_adc: retained resources remain after an earlier cleanup failure") // Log Message
    );

    /// Create the configured ADC one-shot unit.
    adc_oneshot_unit_init_cfg_t unitConfig = {};
    unitConfig.unit_id                     = unit;
    esp_err_t         err                  = adc_oneshot_new_unit(&unitConfig, &_unitHandle);
    const sys_error_t unitInitError        = (err == ESP_OK) ? ERROR_SUCCESS : TRANSLATE_ERROR(err);
    RETURN_IF_ERROR(
        (err != ESP_OK), // Expression
        unitInitError,   // Error Code
        SYS_LOG_E(
            "io_adc: unit %u creation failed (sys_err=%d, esp_err=0x%x)",
            static_cast<unsigned>(config.unit_index) + 1U,
            static_cast<int>(unitInitError),
            static_cast<unsigned int>(err)), // Log Message
        (void)_cleanup()                     // Cleanup
    );

    /// Configure the requested channel and attenuation.
    adc_oneshot_chan_cfg_t channelConfig = {};
    channelConfig.atten                  = attenuation;
    channelConfig.bitwidth               = bitWidth;
    const auto channel                   = static_cast<adc_channel_t>(config.channel);
    err                                  = adc_oneshot_config_channel(_unitHandle, channel, &channelConfig);
    const sys_error_t channelConfigError = (err == ESP_OK) ? ERROR_SUCCESS : TRANSLATE_ERROR(err);
    RETURN_IF_ERROR(
        (err != ESP_OK),    // Expression
        channelConfigError, // Error Code
        SYS_LOG_E(
            "io_adc: unit %u channel %u configuration failed (sys_err=%d, esp_err=0x%x)",
            static_cast<unsigned>(config.unit_index) + 1U,
            static_cast<unsigned>(config.channel),
            static_cast<int>(channelConfigError),
            static_cast<unsigned int>(err)), // Log Message
        (void)_cleanup()                     // Cleanup
    );

    /// Create calibration when supported; raw acquisition remains usable without it.
    adc_cali_curve_fitting_config_t calibrationConfig = {};
    calibrationConfig.unit_id                         = unit;
    calibrationConfig.chan                            = channel;
    calibrationConfig.atten                           = attenuation;
    calibrationConfig.bitwidth                        = bitWidth;
    err                                               = adc_cali_create_scheme_curve_fitting(&calibrationConfig, &_calibrationHandle);
    const sys_error_t calibrationError                = (err == ESP_OK) ? ERROR_SUCCESS : TRANSLATE_ERROR(err);
    RETURN_IF_ERROR(
        (err != ESP_OK && _calibrationHandle != nullptr), // Expression
        calibrationError,                                 // Error Code
        SYS_LOG_E(
            "io_adc: calibration setup failed with a partial handle (sys_err=%d, esp_err=0x%x)",
            static_cast<int>(calibrationError),
            static_cast<unsigned int>(err)), // Log Message
        (void)_cleanup()                     // Cleanup
    );
    if (err != ESP_OK)
    {
        SYS_LOG_W(
            "io_adc: calibration unavailable for unit %u channel %u (esp_err=0x%x); raw samples remain available",
            static_cast<unsigned>(config.unit_index) + 1U,
            static_cast<unsigned>(config.channel),
            static_cast<unsigned int>(err));
    }

    _config      = config;
    _initialized = HAL_INITIALIZED;
    SYS_LOG_I("io_adc: initialized (unit %u, channel %u)", static_cast<unsigned>(config.unit_index) + 1U, static_cast<unsigned>(config.channel));
    return ERROR_SUCCESS;
}

void io_adc::get(void* data)
{
    /// Validate the caller's output pointer before writing a raw ADC sample.
    if (data == nullptr)
    {
        SYS_LOG_E("io_adc: get() called with null buffer");
        return;
    }
    (void)readRaw(*static_cast<uint32_t*>(data));
}

sys_error_t io_adc::set(void* data)
{
    (void)data;
    SYS_LOG_W("io_adc: set() is not supported");
    return ERROR_NOT_SUPPORTED;
}

sys_error_t io_adc::deInit()
{
    /// Return early only when the driver has no initialized or partially retained resources.
    if (_initialized != HAL_INITIALIZED && _unitHandle == nullptr && _calibrationHandle == nullptr)
    {
        return ERROR_SUCCESS;
    }

    /// Release resources; callers must stop concurrent reads before teardown.
    const sys_error_t cleanupError = _cleanup();
    RETURN_ON_ERROR(cleanupError);
    SYS_LOG_D("io_adc: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t io_adc::readRaw(uint32_t& outRaw)
{
    /// Validate initialization before accessing the ADC unit.
    RETURN_IF_ERROR(
        (_initialized != HAL_INITIALIZED),                        // Expression
        ERROR_NOT_INITIALIZED,                                    // Error Code
        SYS_LOG_E("io_adc: read requested before initialization") // Log Message
    );

    /// Read one raw ADC sample.
    int               raw       = 0;
    const esp_err_t   err       = adc_oneshot_read(_unitHandle, static_cast<adc_channel_t>(_config.channel), &raw);
    const sys_error_t readError = (err == ESP_OK) ? ERROR_SUCCESS : TRANSLATE_ERROR(err);
    RETURN_IF_ERROR(
        (err != ESP_OK), // Expression
        readError,       // Error Code
        SYS_LOG_E(
            "io_adc: read failed (sys_err=%d, esp_err=0x%x)",
            static_cast<int>(readError),
            static_cast<unsigned int>(err)) // Log Message
    );
    RETURN_IF_ERROR(
        (raw < 0),                                                        // Expression
        ERROR_READ_FAILED,                                                // Error Code
        SYS_LOG_E("io_adc: ADC returned a negative raw sample (%d)", raw) // Log Message
    );

    outRaw = static_cast<uint32_t>(raw);
    return ERROR_SUCCESS;
}

sys_error_t io_adc::rawToVoltageMv(uint32_t raw, uint32_t& outMv)
{
    /// Validate initialization before accessing calibration data.
    RETURN_IF_ERROR(
        (_initialized != HAL_INITIALIZED),                              // Expression
        ERROR_NOT_INITIALIZED,                                          // Error Code
        SYS_LOG_E("io_adc: conversion requested before initialization") // Log Message
    );
    RETURN_IF_ERROR(
        (_calibrationHandle == nullptr),                                  // Expression
        ERROR_NOT_SUPPORTED,                                              // Error Code
        SYS_LOG_W("io_adc: calibrated voltage conversion is unavailable") // Log Message
    );
    RETURN_IF_ERROR(
        (raw > static_cast<uint32_t>(std::numeric_limits<int>::max())), // Expression
        ERROR_INVALID_ARG,                                              // Error Code
        SYS_LOG_E("io_adc: raw sample exceeds calibration API range")   // Log Message
    );

    /// Convert a raw sample using the configured channel's calibration data.
    int               voltageMv       = 0;
    const esp_err_t   err             = adc_cali_raw_to_voltage(_calibrationHandle, static_cast<int>(raw), &voltageMv);
    const sys_error_t conversionError = (err == ESP_OK) ? ERROR_SUCCESS : TRANSLATE_ERROR(err);
    RETURN_IF_ERROR(
        (err != ESP_OK), // Expression
        conversionError, // Error Code
        SYS_LOG_E(
            "io_adc: raw-to-voltage conversion failed (sys_err=%d, esp_err=0x%x)",
            static_cast<int>(conversionError),
            static_cast<unsigned int>(err)) // Log Message
    );
    RETURN_IF_ERROR(
        (voltageMv < 0),                                             // Expression
        ERROR_READ_FAILED,                                           // Error Code
        SYS_LOG_E("io_adc: calibration returned a negative voltage") // Log Message
    );
    outMv = static_cast<uint32_t>(voltageMv);
    return ERROR_SUCCESS;
}
