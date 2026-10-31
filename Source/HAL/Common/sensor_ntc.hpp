/** @file       sensor_ntc.hpp
 *  @brief      Platform-independent NTC thermistor implementation of IHal_Sensor_Temperature.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Io_Adc.h"
#include "HAL/IHAL/IHal_Sensor_Temperature.h"

/**
 * @class sensor_ntc
 * @brief NTC thermistor implementation of IHal_Sensor_Temperature.
 *
 * Uses an externally configured IHal_Io_Adc for raw samples and calibration.
 * This class averages NTC readings and classifies the electrical health of the
 * sensor path (open/short detection), but does not own the ADC lifecycle.
 *
 * @note Thread-Safety: This driver is not internally synchronized. Its owner
 *       must serialize initialization, reads, state access, and deinitialization.
 */
class sensor_ntc : public IHal_Sensor_Temperature
{
public:
    explicit sensor_ntc(IHal_Io_Adc& adc);
    ~sensor_ntc() override;

    sensor_ntc(const sensor_ntc&)            = delete;
    sensor_ntc& operator=(const sensor_ntc&) = delete;
    sensor_ntc(sensor_ntc&&)                 = delete;
    sensor_ntc& operator=(sensor_ntc&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t init(const hal_temp_sensor_config_t& config) override;
    void        get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t deInit() override;

    sys_error_t             readRawVoltageMv(uint32_t& outMv) override;
    hal_temp_sensor_state_t getSensorState() override;

private:
    uint8_t                  _initialized;
    IHal_Io_Adc&             _adc;
    hal_temp_sensor_config_t _config;
    hal_temp_sensor_state_t  _state;
};
