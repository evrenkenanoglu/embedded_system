/** @file       ThermalMonitor.cpp
 *  @brief      NTC thermistor temperature computation engine (Beta model).
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
#include "ThermalMonitor.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <cmath>

/** CONSTANTS *****************************************************************/
namespace
{
    /// Kelvin to Celsius offset: T[K] = T[C] + 273.15
    constexpr float kKelvinOffset = 273.15F;
} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/
ThermalMonitor::ThermalMonitor()
    : _initialized(false)
    , _calibration()
{
}

sys_error_t ThermalMonitor::init(const thermal_calibration_t& config)
{
    /// Validate the calibration parameters for physical plausibility.
    RETURN_IF_ERROR(config.r_nominal_ohms <= 0.0F, ERROR_INVALID_ARG, SYS_LOG_E("ThermalMonitor: nominal resistance must be positive"));
    RETURN_IF_ERROR(config.b_coefficient <= 0.0F, ERROR_INVALID_ARG, SYS_LOG_E("ThermalMonitor: beta coefficient must be positive"));
    RETURN_IF_ERROR(config.r_fixed_ohms <= 0.0F, ERROR_INVALID_ARG, SYS_LOG_E("ThermalMonitor: fixed resistance must be positive"));
    RETURN_IF_ERROR(config.v_ref_mv <= 0.0F, ERROR_INVALID_ARG, SYS_LOG_E("ThermalMonitor: reference voltage must be positive"));

    _calibration = config;
    _initialized = true;
    SYS_LOG_I(
        "ThermalMonitor: calibration loaded (R25=%.0f, B=%.0f, Rfixed=%.0f, divider=%s)",
        (double)config.r_nominal_ohms,
        (double)config.b_coefficient,
        (double)config.r_fixed_ohms,
        (config.divider_type == ntc_divider_type_t::PULL_UP) ? "pull_up" : "pull_down");
    return ERROR_SUCCESS;
}

sys_error_t ThermalMonitor::voltageToTemperature(uint32_t voltageMv, float& outTempC)
{
    /// Validate monitor initialization state.
    RETURN_IF_ERROR(_initialized == false, ERROR_NOT_INITIALIZED);

    /// Guard against voltages outside the divider operating range.
    RETURN_IF_ERROR(
        (voltageMv <= 0U) || (static_cast<float>(voltageMv) >= _calibration.v_ref_mv),        // Expression
        ERROR_INVALID_ARG,                                                                    // Error code
        SYS_LOG_E("ThermalMonitor: voltage %u mV outside divider range", (unsigned)voltageMv) // Log message
    );

    /// Compute the NTC resistance from the divider topology.
    const auto  vOut = static_cast<float>(voltageMv);
    const float vRef = _calibration.v_ref_mv;
    float       rNtc = 0.0F;
    if (_calibration.divider_type == ntc_divider_type_t::PULL_UP)
    {
        // NTC between ADC pin and GND, fixed resistor to VDD:
        // R_ntc = R_fixed * V_out / (V_ref - V_out)
        rNtc = _calibration.r_fixed_ohms * vOut / (vRef - vOut);
    }
    else
    {
        // NTC between VDD and ADC pin, fixed resistor to GND:
        // R_ntc = R_fixed * (V_ref - V_out) / V_out
        rNtc = _calibration.r_fixed_ohms * (vRef - vOut) / vOut;
    }

    /// Guard against a degenerate resistance before taking the logarithm.
    RETURN_IF_ERROR(rNtc <= 0.0F, ERROR_INVALID_ARG, SYS_LOG_E("ThermalMonitor: computed NTC resistance is non-physical"));

    /// Apply the Beta parameter equation: 1/T = 1/T0 + (1/B) * ln(R/R0).
    const float t0Kelvin   = _calibration.t_nominal_c + kKelvinOffset;
    const float ratio      = rNtc / _calibration.r_nominal_ohms;
    const float invTKelvin = (1.0F / t0Kelvin) + (std::log(ratio) / _calibration.b_coefficient);

    /// Guard against a non-physical reciprocal temperature.
    RETURN_IF_ERROR(invTKelvin <= 0.0F, ERROR_INVALID_ARG, SYS_LOG_E("ThermalMonitor: computed inverse temperature is non-physical"));

    /// Convert back to degrees Celsius.
    outTempC = (1.0F / invTKelvin) - kKelvinOffset;
    return ERROR_SUCCESS;
}
