/** @file       sensor_ntc.cpp
 *  @brief      ESP32 platform implementation of IHal_Sensor_Temperature (NTC thermistor via ADC).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#define ENABLE_SYS_LOG_D

/** INCLUDES ******************************************************************/
#include "sensor_ntc.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>
#include <freertos/semphr.h>

#include <cstdint>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr uint32_t kMutexTimeoutMs   = 100U;  ///< Mutex acquisition timeout in milliseconds
    constexpr uint32_t kDefaultSampleCnt = 16U;   ///< Default multi-sample count per reading
    constexpr uint32_t kFaultThresholdMv = 50U;   ///< Open/short detection margin in millivolts
    constexpr uint32_t kReferenceMv      = 3300U; ///< Nominal full-scale voltage in millivolts

    bool toAdcAttenuation(hal_temp_sensor_attenuation_t attenuation, adc_atten_t& outAttenuation)
    {
        switch (attenuation)
        {
            case hal_temp_sensor_attenuation_t::DB_0:
                outAttenuation = ADC_ATTEN_DB_0;
                return true;
            case hal_temp_sensor_attenuation_t::DB_2_5:
                outAttenuation = ADC_ATTEN_DB_2_5;
                return true;
            case hal_temp_sensor_attenuation_t::DB_6:
                outAttenuation = ADC_ATTEN_DB_6;
                return true;
            case hal_temp_sensor_attenuation_t::DB_12:
                outAttenuation = ADC_ATTEN_DB_12;
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
bool sensor_ntc::_lock(TickType_t timeoutMs)
{
    if (_mutex == nullptr)
    {
        return false;
    }
    return xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void sensor_ntc::_unlock()
{
    if (_mutex != nullptr)
    {
        (void)xSemaphoreGive(_mutex);
    }
}

/** FUNCTIONS *****************************************************************/
sensor_ntc::sensor_ntc()
    : _initialized(HAL_UNINITIALIZED)
    , _mutex(nullptr)
    , _adcUnit(nullptr)
    , _adcCali(nullptr)
    , _config()
    , _state(hal_temp_sensor_state_t::FAULT_UNINITIALIZED)
{
    _config.adc_channel  = 0U;
    _config.sample_count = kDefaultSampleCnt;
    _config.attenuation  = hal_temp_sensor_attenuation_t::DB_12;
}

sensor_ntc::~sensor_ntc()
{
    const sys_error_t err = deInit();
    if (err != ERROR_SUCCESS)
    {
        SYS_LOG_E("sensor_ntc: de-initialization failed during destruction (err: %d)", (int)err);
        return;
    }

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

sys_error_t sensor_ntc::init(void* params)
{
    /// Apply optional caller configuration before touching the hardware.
    if (params != nullptr)
    {
        const auto* cfg = static_cast<const hal_temp_sensor_config_t*>(params);
        RETURN_IF_ERROR(cfg->sample_count == 0U, ERROR_INVALID_ARG, SYS_LOG_E("sensor_ntc: sample_count must be non-zero"));
        _config = *cfg;
    }

    /// Check whether the driver is already initialized.
    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sensor_ntc: already initialized");
        return ERROR_SUCCESS;
    }

    adc_atten_t attenuation = ADC_ATTEN_DB_12;
    RETURN_IF_ERROR(!toAdcAttenuation(_config.attenuation, attenuation), ERROR_INVALID_ARG, SYS_LOG_E("sensor_ntc: unsupported ADC attenuation"));

    /// Allocate FreeRTOS synchronization mutex if not already created.
    if (_mutex == nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
        RETURN_IF_ERROR(_mutex == nullptr, ERROR_OUT_OF_MEMORY, SYS_LOG_E("sensor_ntc: mutex creation failed"));
    }

    /// Configure the ADC One-Shot unit on ADC1.
    adc_oneshot_unit_init_cfg_t unitCfg  = {};
    unitCfg.unit_id                      = ADC_UNIT_1;
    adc_oneshot_unit_handle_t unitHandle = nullptr;
    RETURN_IF_ERROR(adc_oneshot_new_unit(&unitCfg, &unitHandle) != ESP_OK, ERROR_INIT_FAILED, SYS_LOG_E("sensor_ntc: ADC unit creation failed"));

    /// Configure the measurement channel with the requested attenuation.
    adc_oneshot_chan_cfg_t chanCfg = {};
    chanCfg.atten                  = attenuation;
    chanCfg.bitwidth               = ADC_BITWIDTH_DEFAULT;
    if (adc_oneshot_config_channel(unitHandle, static_cast<adc_channel_t>(_config.adc_channel), &chanCfg) != ESP_OK)
    {
        /// Release the freshly created unit before failing initialization.
        (void)adc_oneshot_del_unit(unitHandle);
        SYS_LOG_E("sensor_ntc: ADC channel %u configuration failed", (unsigned)_config.adc_channel);
        return ERROR_INIT_FAILED;
    }

    /// Create the eFuse curve-fitting calibration scheme (ESP32-C6).
    adc_cali_curve_fitting_config_t caliCfg = {};
    caliCfg.unit_id                         = ADC_UNIT_1;
    caliCfg.chan                            = static_cast<adc_channel_t>(_config.adc_channel);
    caliCfg.atten                           = attenuation;
    caliCfg.bitwidth                        = ADC_BITWIDTH_DEFAULT;
    adc_cali_handle_t caliHandle            = nullptr;
    if (adc_cali_create_scheme_curve_fitting(&caliCfg, &caliHandle) != ESP_OK)
    {
        /// Release the freshly created unit before failing initialization.
        (void)adc_oneshot_del_unit(unitHandle);
        SYS_LOG_E("sensor_ntc: ADC calibration scheme creation failed");
        return ERROR_INIT_FAILED;
    }
    _adcUnit = static_cast<void*>(unitHandle);
    _adcCali = static_cast<void*>(caliHandle);

    /// Mark driver state as initialized and log status.
    _initialized = HAL_INITIALIZED;
    _state       = hal_temp_sensor_state_t::OK;
    SYS_LOG_I("sensor_ntc: initialized (ADC1 channel %u, %u samples)", (unsigned)_config.adc_channel, (unsigned)_config.sample_count);
    return ERROR_SUCCESS;
}

void sensor_ntc::get(void* data)
{
    /// Route the generic IHAL_IO accessor to the calibrated voltage read.
    auto* const outMv = static_cast<uint32_t*>(data);
    if (outMv == nullptr)
    {
        SYS_LOG_E("sensor_ntc: get() called with null buffer");
        return;
    }
    (void)readRawVoltageMv(*outMv);
}

sys_error_t sensor_ntc::set(void* data)
{
    /// The sensor is a read-only device; configuration happens through init().
    (void)data;
    SYS_LOG_W("sensor_ntc: set() not supported for read-only sensor");
    return ERROR_NOT_SUPPORTED;
}

sys_error_t sensor_ntc::deInit()
{
    /// Serialize handle teardown with active ADC reads.
    const bool lockAcquired = (_mutex == nullptr) || _lock(kMutexTimeoutMs);
    RETURN_IF_ERROR(!lockAcquired, ERROR_DEVICE_BUSY, SYS_LOG_E("sensor_ntc: de-initialization timed out waiting for ADC access"));

    if (_initialized == HAL_INITIALIZED)
    {
        /// Delete the calibration handle and the oneshot unit.
        if (_adcCali != nullptr)
        {
            (void)adc_cali_delete_scheme_curve_fitting(static_cast<adc_cali_handle_t>(_adcCali));
            _adcCali = nullptr;
        }
        if (_adcUnit != nullptr)
        {
            (void)adc_oneshot_del_unit(static_cast<adc_oneshot_unit_handle_t>(_adcUnit));
            _adcUnit = nullptr;
        }
    }

    /// Reset lifecycle state while still holding the acquisition lock.
    _initialized = HAL_UNINITIALIZED;
    _state       = hal_temp_sensor_state_t::FAULT_UNINITIALIZED;
    if (_mutex != nullptr)
    {
        _unlock();
    }
    SYS_LOG_D("sensor_ntc: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sensor_ntc::readRawVoltageMv(uint32_t& outMv)
{
    if (_mutex == nullptr)
    {
        return ERROR_NOT_INITIALIZED;
    }

    /// Acquire serialization mutex before ADC hardware access.
    RETURN_IF_ERROR(!_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    /// Validate initialization while holding the same lock as deinitialization.
    if (_initialized != HAL_INITIALIZED)
    {
        _unlock();
        return ERROR_NOT_INITIALIZED;
    }

    /// Multi-sample the channel and accumulate the raw conversion results.
    int      rawSum   = 0;
    uint32_t failures = 0U;
    for (uint32_t i = 0U; i < _config.sample_count; ++i)
    {
        int raw = 0;
        if (adc_oneshot_read(static_cast<adc_oneshot_unit_handle_t>(_adcUnit), static_cast<adc_channel_t>(_config.adc_channel), &raw) != ESP_OK)
        {
            ++failures;
            continue;
        }
        rawSum += raw;
    }

    /// Report a hard acquisition failure when every sample failed.
    if (failures == _config.sample_count)
    {
        _state = hal_temp_sensor_state_t::FAULT_UNINITIALIZED;
        _unlock();
        SYS_LOG_E("sensor_ntc: all %u ADC samples failed", (unsigned)_config.sample_count);
        return ERROR_READ_FAILED;
    }

    /// Average the successful samples and convert to calibrated millivolts.
    const int rawMean   = rawSum / static_cast<int>(_config.sample_count - failures);
    int       voltageMv = 0;
    if (adc_cali_raw_to_voltage(static_cast<adc_cali_handle_t>(_adcCali), rawMean, &voltageMv) != ESP_OK)
    {
        _state = hal_temp_sensor_state_t::FAULT_UNINITIALIZED;
        _unlock();
        SYS_LOG_E("sensor_ntc: raw-to-voltage calibration failed");
        return ERROR_READ_FAILED;
    }

    /// Classify the electrical health of the sensor path.
    const auto measuredMv = static_cast<uint32_t>(voltageMv);
    if (measuredMv < kFaultThresholdMv)
    {
        _state = hal_temp_sensor_state_t::FAULT_SHORT_CIRCUIT;
        _unlock();
        SYS_LOG_E("sensor_ntc: short circuit detected (%u mV)", (unsigned)measuredMv);
        return ERROR_IO;
    }
    if (measuredMv > (kReferenceMv - kFaultThresholdMv))
    {
        _state = hal_temp_sensor_state_t::FAULT_OPEN_CIRCUIT;
        _unlock();
        SYS_LOG_E("sensor_ntc: open circuit detected (%u mV)", (unsigned)measuredMv);
        return ERROR_IO;
    }

    _state = hal_temp_sensor_state_t::OK;
    outMv  = measuredMv;
    _unlock();
    return ERROR_SUCCESS;
}

hal_temp_sensor_state_t sensor_ntc::getSensorState()
{
    if (!_lock(kMutexTimeoutMs))
    {
        return hal_temp_sensor_state_t::FAULT_UNINITIALIZED;
    }

    const hal_temp_sensor_state_t state = _state;
    _unlock();
    return state;
}
