/** @file       io_adc.hpp
 *  @brief      ESP32 implementation of the platform-independent ADC IHAL.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Io_Adc.h"
#include "System/system.h"

#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_oneshot.h>

/**
 * @class io_adc
 * @brief ESP32 ADC One-Shot implementation with optional curve-fitting calibration.
 *
 * @note Thread-Safety: This driver is not internally synchronized. Its owner
 *       must serialize initialization, reads, conversion, and deinitialization.
 */
class io_adc : public IHal_Io_Adc
{
public:
    io_adc();
    ~io_adc() override;

    io_adc(const io_adc&)            = delete;
    io_adc& operator=(const io_adc&) = delete;
    io_adc(io_adc&&)                 = delete;
    io_adc& operator=(io_adc&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t init(const hal_io_adc_config_t& config) override;
    void        get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t deInit() override;

    sys_error_t readRaw(uint32_t& outRaw) override;
    sys_error_t rawToVoltageMv(uint32_t raw, uint32_t& outMv) override;

private:
    sys_error_t _cleanup();

    uint8_t                   _initialized;
    adc_oneshot_unit_handle_t _unitHandle;
    adc_cali_handle_t         _calibrationHandle;
    hal_io_adc_config_t       _config;
};
