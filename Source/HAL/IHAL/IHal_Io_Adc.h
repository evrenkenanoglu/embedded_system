/** @file       IHal_Io_Adc.h
 *  @brief      Platform-independent contract for analogue-to-digital conversion.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

#pragma once

#include "IHal.h"

#include <cstdint>

/**
 * @brief Optional analogue input attenuation.
 *
 * A platform may reject unsupported selections with ERROR_NOT_SUPPORTED.
 */
enum class hal_adc_attenuation_t : uint8_t
{
    DEFAULT = 0, ///< Use the platform's default attenuation
    DB_0    = 1, ///< 0 dB attenuation
    DB_2_5  = 2, ///< 2.5 dB attenuation
    DB_6    = 3, ///< 6 dB attenuation
    DB_12   = 4  ///< 12 dB attenuation
};

/**
 * @brief Platform-independent configuration for an ADC input.
 */
typedef struct
{
    uint8_t               unit_index;      ///< Zero-based ADC peripheral index
    uint8_t               channel;         ///< ADC channel index
    uint8_t               resolution_bits; ///< Zero selects the platform default resolution
    hal_adc_attenuation_t attenuation;     ///< Input attenuation selection
} hal_io_adc_config_t;

/**
 * @class IHal_Io_Adc
 * @brief Generic analogue-to-digital conversion interface.
 *
 * Consumers configure and read ADC inputs without depending on a particular
 * sensor or vendor SDK. Sampling, averaging, filtering, and interpretation
 * belong to the consumer. Raw samples are always available; calibrated voltage
 * conversion is available when supported by the platform. Implementations are
 * not required to be internally synchronized; the owner must serialize access
 * and lifecycle operations.
 */
class IHal_Io_Adc : public IHAL_IO
{
public:
    /**
     * @brief Initialize an ADC input with platform-independent settings.
     *
     * @param[in] config ADC unit index, channel, resolution, and attenuation.
     *
     * @return sys_error_t ERROR_SUCCESS on success, otherwise an initialization
     *         or argument error; ERROR_NOT_SUPPORTED for unavailable settings.
     */
    virtual sys_error_t init(const hal_io_adc_config_t& config) = 0;

    /**
     * @brief Read one raw ADC sample.
     *
     * @param[out] outRaw Raw ADC code as an unsigned integer. Its width follows
     *                    the configured resolution, or the platform default.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_INITIALIZED when
     *         the ADC is not initialized, or ERROR_READ_FAILED on acquisition
     *         failure.
     */
    virtual sys_error_t readRaw(uint32_t& outRaw) = 0;

    /**
     * @brief Convert a raw sample to calibrated millivolts.
     *
     * @param[in] raw Raw ADC code previously returned by this ADC configuration.
     * @param[out] outMv Calibrated input voltage in millivolts.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_INITIALIZED when
     *         the ADC is not initialized, ERROR_NOT_SUPPORTED when calibration
     *         is unavailable, or ERROR_READ_FAILED on conversion failure.
     */
    virtual sys_error_t rawToVoltageMv(uint32_t raw, uint32_t& outMv) = 0;

    ~IHal_Io_Adc() override = default;
};
