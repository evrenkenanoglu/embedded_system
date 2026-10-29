/** @file       TemperatureMeasurement.cpp
 *  @brief      Matter Temperature Measurement cluster (0x0402) device plugin.
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
#include "TemperatureMeasurement.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

/** CONSTANTS *****************************************************************/
namespace
{
    /// Matter spec: MeasuredValue is in 0.01 degrees Celsius steps.
    constexpr float kCentiScale = 100.0F;
    /// Matter spec: minimum supported range (-20.00 degrees Celsius).
    constexpr int16_t kMinMeasuredValue = -2000;
    /// Matter spec: maximum supported range (100.00 degrees Celsius).
    constexpr int16_t kMaxMeasuredValue = 10000;
} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/
TemperatureMeasurement::TemperatureMeasurement(Proc_ThermalMonitor& monitor)
    : _monitor(monitor) // Sample source
{
    /// Override the device type inherited from the OnOff plugin chain.
    _device_type = MatterTypes::Device::TEMPERATURE_SENSOR_DEVICE;
}

TemperatureMeasurement::~TemperatureMeasurement() = default;

sys_error_t TemperatureMeasurement::start()
{
    /// Subscribe to the acquisition process samples.
    RETURN_ON_ERROR(_monitor.addListener([this](const thermal_sample_t& sample) { onSample(sample); }), SYS_LOG_E("Failed to subscribe temperature measurement listener"));

    return ERROR_SUCCESS;
}

sys_error_t TemperatureMeasurement::publishTemperature(float temperatureC)
{
    /// Scale to 0.01 degrees Celsius steps and clamp to the supported range.
    float scaled = temperatureC * kCentiScale;
    if (scaled < static_cast<float>(kMinMeasuredValue))
    {
        scaled = static_cast<float>(kMinMeasuredValue);
    }
    if (scaled > static_cast<float>(kMaxMeasuredValue))
    {
        scaled = static_cast<float>(kMaxMeasuredValue);
    }

    /// Round to the nearest centi-degree and publish as INT16.
    constexpr float               kRoundHalf = 0.5F;
    MatterTypes::AttributeValue_t value      = {};
    value.type                               = MatterTypes::Value::INT16;
    value.value.i16                          = static_cast<int16_t>(scaled + (scaled >= 0.0F ? kRoundHalf : -kRoundHalf));

    return updateLocalState(&value);
}

void TemperatureMeasurement::onSample(const thermal_sample_t& sample)
{
    /// Skip publication while the sensor path is faulted.
    if (sample.sensorState != hal_temp_sensor_state_t::OK)
    {
        SYS_LOG_W("TemperatureMeasurement: skipping publish for faulted sensor (state: %d)", (int)sample.sensorState);
        return;
    }

    RETURN_IF_ERROR(publishTemperature(sample.temperatureC) != ERROR_SUCCESS, , SYS_LOG_E("TemperatureMeasurement: failed to publish %.2fC", (double)sample.temperatureC));
}
