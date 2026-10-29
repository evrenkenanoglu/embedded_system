/** @file       IThermalMonitor.hpp
 *  @brief      Platform Abstraction Layer interface for thermistor temperature computation.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "System/system.h"

#include <cstdint>

/**
 * @brief Voltage divider topology of the NTC measurement circuit.
 */
enum class ntc_divider_type_t : uint8_t
{
    PULL_UP   = 0, ///< NTC between ADC pin and GND, fixed resistor to VDD
    PULL_DOWN = 1  ///< NTC between VDD and ADC pin, fixed resistor to GND
};

/**
 * @brief Calibration parameters of the NTC thermistor and its divider network.
 */
typedef struct
{
    float              r_nominal_ohms; ///< Resistance at the nominal temperature (R25)
    float              t_nominal_c;    ///< Nominal temperature in degrees Celsius
    float              b_coefficient;  ///< Beta coefficient (B25/85)
    float              r_fixed_ohms;   ///< Fixed divider resistor value in ohms
    float              v_ref_mv;       ///< Divider supply voltage in millivolts
    ntc_divider_type_t divider_type;   ///< Divider topology of the measurement circuit
} thermal_calibration_t;

/**
 * @class IThermalMonitor
 * @brief Pure abstract contract for calibrated temperature calculations.
 *
 * Converts a calibrated ADC voltage into a physical temperature using the
 * thermistor model of the attached NTC network. Implementations must be free
 * of dynamic memory allocation so the conversion can run from a deterministic
 * real-time task.
 */
class IThermalMonitor
{
public:
    /**
     * @brief Loads the thermistor calibration parameters.
     *
     * @param[in] config Thermistor and divider network calibration data.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NULL_POINTER when
     *         config is invalid, ERROR_INVALID_ARG when parameters are
     *         non-physical (zero resistance, zero reference voltage).
     */
    virtual sys_error_t init(const thermal_calibration_t& config) = 0;

    /**
     * @brief Converts a calibrated voltage into a temperature in degrees Celsius.
     *
     * @param[in]  voltageMv Calibrated sensor voltage in millivolts.
     * @param[out] outTempC  Computed temperature in degrees Celsius.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_INITIALIZED when
     *         the monitor has no calibration loaded, ERROR_INVALID_ARG when
     *         the voltage is outside the divider operating range.
     */
    virtual sys_error_t voltageToTemperature(uint32_t voltageMv, float& outTempC) = 0;

    virtual ~IThermalMonitor() = default;
};
