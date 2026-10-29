/** @file       ThermalMonitor.hpp
 *  @brief      NTC thermistor temperature computation engine (Beta model).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "PAL/Protocols/Thermal/IThermalMonitor.hpp"

/**
 * @class ThermalMonitor
 * @brief Converts calibrated NTC divider voltages into temperatures.
 *
 * Implements the voltage divider resistance computation and the Beta
 * parameter equation (1/T = 1/T0 + (1/B) * ln(R/R0)). All arithmetic is
 * performed with fixed-size floats and no dynamic memory allocation so the
 * conversion is safe to call from a deterministic real-time task.
 */
class ThermalMonitor : public IThermalMonitor
{
public:
    ThermalMonitor();
    ~ThermalMonitor() override = default;

    ThermalMonitor(const ThermalMonitor&)            = delete;
    ThermalMonitor& operator=(const ThermalMonitor&) = delete;
    ThermalMonitor(ThermalMonitor&&)                 = delete;
    ThermalMonitor& operator=(ThermalMonitor&&)      = delete;

    sys_error_t init(const thermal_calibration_t& config) override;
    sys_error_t voltageToTemperature(uint32_t voltageMv, float& outTempC) override;

private:
    bool                  _initialized;
    thermal_calibration_t _calibration;
};
