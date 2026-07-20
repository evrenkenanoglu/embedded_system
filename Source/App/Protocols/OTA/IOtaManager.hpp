/** @file       IOtaManager.hpp
 *  @brief      Interface defining the application-level firmware update orchestrator.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#pragma once

#include "System/errorTranslateHandler.h"
#include <string>
#include <functional>

/**
 * @brief Verification results generated during updates scanning cycles.
 */
enum class OtaCheckResult : uint8_t
{
    NoUpdateAvailable = 0,  ///< The device is already running the latest firmware version
    UpdateAvailable,        ///< Compatible target version detected on the remote server
    HardwareMismatch,       ///< Manifest matches a version intended for another board layout
    ManifestFetchError,     ///< Failed to establish link or download update details
    ManifestParseError,     ///< JSON contents contain invalid keys or structures
    IncompatibleVersion     ///< Target version fails validation constraints
};

/**
 * @struct OtaManagerOptions_t
 * @brief Structure containing settings for update checks and target validation parameters.
 */
struct OtaManagerOptions_t
{
    std::string gatewayUrl;                   ///< Remote update gateway URL (e.g., https://192.168.0.172:8443)
    std::string serverCert;                   ///< Base CA trust anchor certificate
    std::string apiKey;                       ///< Authorization token key passed via custom headers
    std::string currentVersion;               ///< Current running application firmware version
    std::string hardwareType;                 ///< Hardware board layout or module identifier
    std::string deviceId;                     ///< Unique hardware address or MAC identifier
    std::string channel;                      ///< Active deployment cohort channel (stable/beta/testing)
    uint32_t    currentHsvn = 0;              ///< Hardware Security Version Number
    uint32_t    baseCheckIntervalSec = 86400; ///< Periodic check baseline (24 hours)
    uint32_t    jitterRangeSec       = 1800;  ///< Maximum timing offset (30 minutes)
    uint8_t     minBatteryPct        = 80;    ///< Threshold for stage 2 download
    uint8_t     allowedStartHour     = 2;     ///< Low traffic window start hour (2 AM)
    uint8_t     allowedEndHour       = 4;     ///< Low traffic window end hour (4 AM)
    bool        bypassConditions     = false; ///< Bypasses conditions for local testing
};

/**
 * @struct OtaPlatformHooks_t
 * @brief Platform abstraction hooks to decouple hardware peripherals.
 */
struct OtaPlatformHooks_t
{
    std::function<uint8_t()>  getBatteryPercentage = nullptr; ///< Returns battery capacity (0-100%)
    std::function<int()>      getCurrentLocalHour   = nullptr; ///< Returns local hour (0-23) or -1 if clock is unsynced
    std::function<uint32_t()> getRandomNumber       = nullptr; ///< Returns a 32-bit random integer
    std::function<void()>     rebootSystem          = nullptr; ///< Triggers system soft-reboot
};

/**
 * @class IOtaManager
 * @brief High-level application orchestrator governing update triggers and self-test verification.
 * 
 * @note Thread-Safety: Interfaces derived from this class are thread-safe and can withstand
 *       asynchronous state requests from the system application loop.
 */
class IOtaManager
{
public:
    virtual ~IOtaManager() = default;

    /**
     * @brief High-level initialization of manager modules and certificate signatures.
     *
     * @param[in] options Configuration parameters governing the remote connections and limits.
     * @param[in] hooks   Hardware-specific interface callbacks.
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t init(const OtaManagerOptions_t& options, const OtaPlatformHooks_t& hooks) = 0;

    /**
     * @brief Cleans up manager configurations and temporary update allocations.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Connects to the update API to scan for newly published manifests.
     *
     * @param[out] outResult Verification result representing update feasibility.
     * @return sys_error_t ERROR_SUCCESS if execution completes, otherwise an error status code.
     */
    virtual sys_error_t checkForUpdates(OtaCheckResult& outResult) = 0;

    /**
     * @brief Executes download, partition flash, and signature authentication.
     *
     * @return sys_error_t ERROR_SUCCESS if execution succeeds, otherwise an error status code.
     */
    virtual sys_error_t executeUpdate() = 0;

    /**
     * @brief Performs self-checks and commits the newly booted partition layout to memory.
     *
     * @return sys_error_t ERROR_SUCCESS if validation is accepted, otherwise an error status code.
     */
    virtual sys_error_t validateCurrentFirmware() = 0;

    /**
     * @brief Restarts the local micro-controller unit into the updated partition.
     */
    virtual void rebootSystem() = 0;
};