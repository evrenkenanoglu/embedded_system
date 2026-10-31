/** @file       sensor_ntc.cpp
 *  @brief      Platform-independent NTC thermistor implementation of IHal_Sensor_Temperature.
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
#include "System/error_macros.h"

#include <cstdint>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr uint32_t kDefaultSampleCnt = 16U;   ///< Default multi-sample count per reading
    constexpr uint32_t kFaultThresholdMv = 50U;   ///< Open/short detection margin in millivolts
    constexpr uint32_t kReferenceMv      = 3300U; ///< Nominal full-scale voltage in millivolts

} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/
/** FUNCTIONS *****************************************************************/
sensor_ntc::sensor_ntc(IHal_Io_Adc& adc)
    : _initialized(HAL_UNINITIALIZED)
    , _adc(adc)
    , _config()
    , _state(hal_temp_sensor_state_t::FAULT_UNINITIALIZED)
{
    _config.sample_count = kDefaultSampleCnt;
}

sensor_ntc::~sensor_ntc()
{
    ON_ERROR_WITH_LOG(
        deInit(),                                                // Expression
        "sensor_ntc de-initialization failed during destruction" // Log Message
    );
}

sys_error_t sensor_ntc::init(void* params)
{
    /// Preserve the generic IHAL_IO entry point while routing configuration through the typed API.
    return (params == nullptr) ? init(_config) : init(*static_cast<const hal_temp_sensor_config_t*>(params));
}

sys_error_t sensor_ntc::init(const hal_temp_sensor_config_t& config)
{
    /// Validate NTC-specific sampling configuration before touching the ADC.
    RETURN_IF_ERROR(config.sample_count == 0U, ERROR_INVALID_ARG, SYS_LOG_E("sensor_ntc: sample_count must be non-zero"));

    /// Avoid changing NTC sampling policy while the sensor is active.
    if (_initialized == HAL_INITIALIZED)
    {
        const bool sameConfig = (_config.sample_count == config.sample_count);
        return sameConfig ? ERROR_SUCCESS : ERROR_INVALID_STATE;
    }

    _config = config;

    /// Record the sensor lifecycle; the composition root owns ADC initialization.
    _initialized = HAL_INITIALIZED;
    _state       = hal_temp_sensor_state_t::OK;
    SYS_LOG_I("sensor_ntc: initialized (%u samples per reading)", (unsigned)_config.sample_count);
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
    /// Reset sensor state; the caller must stop concurrent reads before teardown.
    _initialized = HAL_UNINITIALIZED;
    _state       = hal_temp_sensor_state_t::FAULT_UNINITIALIZED;
    SYS_LOG_D("sensor_ntc: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sensor_ntc::readRawVoltageMv(uint32_t& outMv)
{
    /// Validate initialization before reading from the injected ADC.
    if (_initialized != HAL_INITIALIZED)
    {
        SYS_LOG_E("sensor_ntc: read requested before initialization");
        return ERROR_NOT_INITIALIZED;
    }

    /// Multi-sample the configured ADC input and average successful raw readings.
    uint64_t rawSum   = 0U;
    uint32_t failures = 0U;
    for (uint32_t i = 0U; i < _config.sample_count; ++i)
    {
        uint32_t raw = 0U;
        if (_adc.readRaw(raw) != ERROR_SUCCESS)
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
        SYS_LOG_E("sensor_ntc: all %u ADC samples failed", (unsigned)_config.sample_count);
        return ERROR_READ_FAILED;
    }

    /// Convert the average raw sample to calibrated millivolts.
    const uint32_t rawMean    = static_cast<uint32_t>(rawSum / (_config.sample_count - failures));
    uint32_t       measuredMv = 0U;
    if (_adc.rawToVoltageMv(rawMean, measuredMv) != ERROR_SUCCESS)
    {
        _state = hal_temp_sensor_state_t::FAULT_UNINITIALIZED;
        SYS_LOG_E("sensor_ntc: ADC raw-to-voltage conversion failed");
        return ERROR_READ_FAILED;
    }

    /// Classify the electrical health of the sensor path.
    if (measuredMv < kFaultThresholdMv)
    {
        _state = hal_temp_sensor_state_t::FAULT_SHORT_CIRCUIT;
        SYS_LOG_E("sensor_ntc: short circuit detected (%u mV)", (unsigned)measuredMv);
        return ERROR_IO;
    }
    if (measuredMv > (kReferenceMv - kFaultThresholdMv))
    {
        _state = hal_temp_sensor_state_t::FAULT_OPEN_CIRCUIT;
        SYS_LOG_E("sensor_ntc: open circuit detected (%u mV)", (unsigned)measuredMv);
        return ERROR_IO;
    }

    _state = hal_temp_sensor_state_t::OK;
    outMv  = measuredMv;
    return ERROR_SUCCESS;
}

hal_temp_sensor_state_t sensor_ntc::getSensorState()
{
    return _state;
}
