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

#include "PAL/Protocols/OTA/IOtaService.hpp"
#include "System/errorTranslateHandler.h"

#include <string>

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
 * @class IOtaManager
 * @brief High-level application orchestrator governing update triggers and self-test verification.
 * 
 * @note Thread-Safety: Interfaces derived from this class must be designed to withstand
 *       asynchronous state requests from the system application loop.
 */
class IOtaManager
{
public:
    virtual ~IOtaManager() = default;

    /**
     * @brief High-level initialization of manager modules and certificate signatures.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t init() = 0;

    /**
     * @brief Cleans up manager configurations and temporary update allocations.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Connects to the update API to scan for newly published manifests.
     *
     * @param[in]  manifestUrl Complete URL pointing to the remote JSON target payload.
     * @param[out] outOptions  Options block containing parsed remote parameters.
     * 
     * @return OtaCheckResult Status showing download and evaluation feasibility.
     */
    virtual OtaCheckResult checkForUpdates(const std::string& manifestUrl, OtaOptions_t& outOptions) = 0;

    /**
     * @brief Executes download, partition flash, and signature authentication.
     *
     * @param[in]  options Session parameter structure matching remote update options.
     * 
     * @return sys_error_t ERROR_SUCCESS if execution succeeds, otherwise an error status code.
     */
    virtual sys_error_t executeUpdate(const OtaOptions_t& options) = 0;

    /**
     * @brief Performs self-checks and commits the newly booted partition layout to memory.
     *
     * Must be triggered shortly after initialization on a fresh boot to confirm stability.
     * Failure to invoke this on boot triggers safe fallbacks to the older partition.
     *
     * @return sys_error_t ERROR_SUCCESS if validation is accepted, otherwise an error status code.
     */
    virtual sys_error_t validateCurrentFirmware() = 0;

    /**
     * @brief Restarts the local micro-controller unit into the updated partition.
     */
    virtual void rebootSystem() = 0;
};