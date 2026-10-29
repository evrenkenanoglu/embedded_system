/** @file       IHal_Sensor_Temperature.h
 *  @brief      Hardware Abstraction Layer interface for temperature sensing devices.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "IHal.h"

#include <cstdint>

/**
 * @brief Health state reported by a temperature sensing device.
 */
enum class hal_temp_sensor_state_t : uint8_t
{
    OK                  = 0, ///< Reading is valid and within the electrical operating range
    FAULT_OPEN_CIRCUIT  = 1, ///< Sensor path is open (voltage collapsed to the reference rail)
    FAULT_SHORT_CIRCUIT = 2, ///< Sensor path is shorted (voltage collapsed to ground)
    FAULT_UNINITIALIZED = 3  ///< Driver has not been initialized yet
};

/**
 * @brief Supported attenuation selections for an ADC temperature sensor.
 */
enum class hal_temp_sensor_attenuation_t : uint8_t
{
    DB_0   = 0, ///< 0 dB attenuation
    DB_2_5 = 1, ///< 2.5 dB attenuation
    DB_6   = 2, ///< 6 dB attenuation
    DB_12  = 3  ///< 12 dB attenuation
};

/**
 * @brief Platform-independent configuration for a temperature sensing device.
 */
typedef struct
{
    uint8_t                       adc_channel;  ///< ADC channel index the sensor is wired to
    uint32_t                      sample_count; ///< Number of samples averaged per reading
    hal_temp_sensor_attenuation_t attenuation;  ///< ADC attenuation selection
} hal_temp_sensor_config_t;

/**
 * @class IHal_Sensor_Temperature
 * @brief Specialization of IHAL_IO providing raw voltage acquisition from a temperature sensor.
 *
 * The interface deliberately exposes only calibrated millivolt readings and a
 * sensor health state; conversion from voltage to physical temperature belongs
 * to the PAL layer (see IThermalMonitor), keeping the HAL free of sensor-model
 * mathematics.
 *
 * @note Thread-Safety: Implementations must serialize access to sensor state
 *       and hardware as required by their platform.
 */
class IHal_Sensor_Temperature : public IHAL_IO
{
public:
    /**
     * @brief Reads the sensor output as a calibrated voltage in millivolts.
     *
     * Performs the platform-specific acquisition (multi-sample averaging and
     * ADC calibration) and reports the electrical health of the sensor path.
     *
     * @param[out] outMv Calibrated voltage in millivolts.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_INITIALIZED when
     *         the driver is not initialized, ERROR_READ_FAILED when acquisition
     *         fails, ERROR_IO when the sensor path is detected as open or shorted.
     */
    virtual sys_error_t readRawVoltageMv(uint32_t& outMv) = 0;

    /**
     * @brief Returns the electrical health state of the sensor path.
     *
     * @return hal_temp_sensor_state_t Last detected sensor state.
     */
    virtual hal_temp_sensor_state_t getSensorState() = 0;

    virtual ~IHal_Sensor_Temperature() = default;
};
