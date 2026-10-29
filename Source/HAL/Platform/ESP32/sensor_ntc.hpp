/** @file       sensor_ntc.hpp
 *  @brief      ESP32 platform implementation of IHal_Sensor_Temperature (NTC thermistor via ADC).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sensor_Temperature.h"
#include "System/system.h"

/**
 * @class sensor_ntc
 * @brief ESP32 implementation of IHal_Sensor_Temperature for an NTC thermistor.
 *
 * Backed by the ESP-IDF ADC One-Shot driver with eFuse curve-fitting
 * calibration. Each acquisition multi-samples the configured channel, averages
 * the raw ticks, converts them to calibrated millivolts and classifies the
 * electrical health of the sensor path (open/short detection).
 *
 * @note Thread-Safety: Reads, state access, and deinitialization are serialized
 *       through an internal FreeRTOS mutex. Initialization and deinitialization
 *       must not race with each other; destruction requires that callers have
 *       stopped invoking methods on this object.
 */
class sensor_ntc : public IHal_Sensor_Temperature
{
public:
    sensor_ntc();
    ~sensor_ntc() override;

    sensor_ntc(const sensor_ntc&)            = delete;
    sensor_ntc& operator=(const sensor_ntc&) = delete;
    sensor_ntc(sensor_ntc&&)                 = delete;
    sensor_ntc& operator=(sensor_ntc&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    void        get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t deInit() override;

    sys_error_t             readRawVoltageMv(uint32_t& outMv) override;
    hal_temp_sensor_state_t getSensorState() override;

private:
    bool _lock(TickType_t timeoutMs);
    void _unlock();

    uint8_t                  _initialized;
    SemaphoreHandle_t        _mutex;
    void*                    _adcUnit; ///< ADC oneshot unit handle (adc_oneshot_unit_handle_t)
    void*                    _adcCali; ///< ADC calibration handle (adc_cali_handle_t)
    hal_temp_sensor_config_t _config;
    hal_temp_sensor_state_t  _state;
};
