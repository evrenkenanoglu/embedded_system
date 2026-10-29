/** @file       TemperatureMeasurement.hpp
 *  @brief      Matter Temperature Measurement cluster (0x0402) device plugin.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "PAL/Protocols/MATTER/Device/OnOff/OnOff_Plugin.hpp"
#include "Process/Peripheral/Proc_ThermalMonitor.hpp"

/**
 * @class TemperatureMeasurement
 * @brief Application-side Matter device publishing local temperature readings.
 *
 * Subscribes to the thermal acquisition process and republishes the filtered
 * temperature to the Matter stack through the Temperature Measurement cluster
 * (0x0402). Attribute values follow the Matter specification: MeasuredValue is
 * expressed in 0.01 degrees Celsius steps as a signed 16-bit integer.
 *
 * @note Thread-Safety: Sample delivery and publication must be serialized by
 *       the thermal monitor's task context.
 * @note Lifetime: This plugin must remain alive until its thermal monitor has
 *       stopped delivering callbacks.
 */
class TemperatureMeasurement : public OnOff_Plugin
{
public:
    /**
     * @brief Construct a new TemperatureMeasurement object.
     *
     * @param monitor Acquisition process providing filtered thermal samples.
     */
    explicit TemperatureMeasurement(Proc_ThermalMonitor& monitor);
    ~TemperatureMeasurement() override;

    TemperatureMeasurement(const TemperatureMeasurement&)            = delete;
    TemperatureMeasurement& operator=(const TemperatureMeasurement&) = delete;
    TemperatureMeasurement(TemperatureMeasurement&&)                 = delete;
    TemperatureMeasurement& operator=(TemperatureMeasurement&&)      = delete;

    /**
     * @brief Subscribes the plugin to the acquisition process samples.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_FAIL when
     *         registration with the monitor fails.
     */
    sys_error_t start();

    /**
     * @brief Publishes a temperature to the Matter stack.
     *
     * Formats the temperature into 0.01 degrees Celsius steps and pushes it to
     * the MeasuredValue (0x0000) attribute of the Temperature Measurement
     * cluster through the local state update callback.
     *
     * @param temperatureC Temperature in degrees Celsius.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_STATE when
     *         the endpoint has not been registered by the stack yet.
     */
    sys_error_t publishTemperature(float temperatureC);

private:
    void onSample(const thermal_sample_t& sample);

    Proc_ThermalMonitor& _monitor;
};
