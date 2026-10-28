/** @file       IHal_Sys_Rtc.h
 *  @brief      Specialized interface for Real-Time Clock and system time operations.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal.h"
#include <cstddef>
#include <cstdint>
#include <ctime>

/**
 * @brief Clock source feeding the RTC / system time base.
 */
enum class hal_rtc_clock_source_t : uint8_t
{
    INTERNAL_RC = 0,  /*!< Internal RC oscillator (least accurate) */
    EXTERNAL_CRYSTAL, /*!< External 32.768 kHz crystal (typical) */
    EXTERNAL_OSC      /*!< External oscillator / TCXO input */
};

/**
 * @brief RTC configuration parameters.
 */
typedef struct
{
    hal_rtc_clock_source_t clk_src;                 /*!< Selected clock source */
    uint32_t               calibration_interval_ms; /*!< Periodic calibration interval, 0 = disabled */
} hal_rtc_config_t;

/**
 * @class IHal_Sys_Rtc
 * @brief Specialization of IHAL_SYS providing wall-clock (POSIX epoch) time,
 *        monotonic time and timezone management.
 *
 * Implementations must be safe for concurrent use from multiple tasks.
 */
class IHal_Sys_Rtc : public IHAL_SYS
{
public:
    /**
     * @brief Set the wall-clock time (seconds since 1970-01-01T00:00:00Z).
     *
     * @param[in] epochSeconds  POSIX epoch timestamp in seconds.
     * @param[in] fractionalUs  Sub-second fraction in microseconds (0..999999), default 0.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t setTime(time_t epochSeconds, uint32_t fractionalUs = 0) = 0;

    /**
     * @brief Get the current wall-clock time.
     *
     * @param[out] epochSeconds  POSIX epoch timestamp in seconds.
     * @param[out] fractionalUs  Optional sub-second fraction in microseconds (may be nullptr).
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t getTime(time_t& epochSeconds, uint32_t* fractionalUs = nullptr) = 0;

    /**
     * @brief Get the monotonic time since boot (never jumps, never wraps in practice).
     *
     * @return uint64_t Monotonic microseconds since boot.
     */
    virtual uint64_t getMonotonicTimeUs() = 0;

    /**
     * @brief Set the POSIX TZ string used for local-time conversion.
     *
     * @param[in] tzString  POSIX TZ string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3"), must not be nullptr.
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_ARG on nullptr/overflow.
     */
    virtual sys_error_t setTimezone(const char* tzString) = 0;

    /**
     * @brief Get the currently configured POSIX TZ string.
     *
     * @param[out] tzBuffer  Destination buffer, must not be nullptr.
     * @param[in]  maxLen    Size of the destination buffer in bytes (including NUL).
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_ARG or ERROR_BUFFER_OVERFLOW otherwise.
     */
    virtual sys_error_t getTimezone(char* tzBuffer, size_t maxLen) = 0;

    /**
     * @brief Check whether a plausible wall-clock time has been established.
     *
     * @return true if the wall-clock time is considered valid/real-time-synced.
     * @return false otherwise.
     */
    virtual bool isTimeSet() = 0;

    virtual ~IHal_Sys_Rtc() = default;
};
