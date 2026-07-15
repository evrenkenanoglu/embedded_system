/** @file       IOtaService.hpp
 *  @brief      Interface defining the core OTA update orchestrator and state machine.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#pragma once

#include "System/errorTranslateHandler.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

/**
 * @brief Representation of the OTA life cycle states.
 */
enum class OtaState : uint8_t
{
    Idle = 0,    ///< Service is inactive and waiting for triggers
    Downloading, ///< Currently pulling payload data from the transport stream
    Verifying,   ///< Executing partition integrity and signature validation
    Applying,    ///< Writing boot properties to set the target execution partition
    Success,     ///< Flash update cycle succeeded
    Failed       ///< An error occurred during execution
};

/**
 * @brief Configuration properties required for the update write cycle.
 */
struct OtaOptions_t
{
    std::string url;              ///< Target download URL for the firmware binary
    std::string serverCert;       ///< Root certificate string for TLS validation
    size_t      chunkSize{4096};  ///< Size of the local write buffer
    uint32_t    timeoutMs{30000}; ///< Socket transfer and response timeouts

    // Cryptographic validation parameters matching the advanced server schema
    std::string signature;       ///< Hex signature of the downloaded binary payload
    std::string targetSignature; ///< Hex signature of the final reassembled application binary
    std::string signingCert;     ///< PEM-encoded certificate used to sign the firmware payload

    bool   isDelta{false}; ///< Flag indicating if the update payload is a delta patch
    size_t targetSize{0};  ///< The exact size of the final reassembled application binary in bytes
};

/**
 * @brief Callback signature to broadcast progress updates to listening applications.
 * @param[in] state         The current operational status of the state machine.
 * @param[in] bytesReceived Total amount of firmware bytes written to flash.
 * @param[in] totalBytes    The expected total binary length of the incoming payload.
 */
using OtaProgressCb_t = std::function<void(OtaState state, size_t bytesReceived, size_t totalBytes)>;

/**
 * @class IOtaService
 * @brief Core PAL orchestrator abstract class managing binary partition flash updates.
 *
 * @note Thread-Safety: Implementations of this interface must provide external synchronization
 *       mechanisms or internal lockouts if accessed by concurrent tasks.
 */
class IOtaService
{
public:
    virtual ~IOtaService() = default;

    /**
     * @brief Initialize the OTA service dependencies and internal states.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t init() = 0;

    /**
     * @brief Deinitialize the OTA service and release active partition maps.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Initiates the binary update transaction.
     *
     * This method blocks until the download, verification, and partition activation
     * procedures are either fully complete, aborted, or terminated by a timeout.
     *
     * @param[in]  options    Configuration options containing buffer sizes and constraints.
     * @param[in]  progressCb Callback structure to propagate system status modifications.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t startUpdate(const OtaOptions_t& options, OtaProgressCb_t progressCb) = 0;

    /**
     * @brief Forcefully terminates an active download and invalidates the partition write.
     *
     * @return sys_error_t ERROR_SUCCESS if successful, otherwise an error status code.
     */
    virtual sys_error_t abortUpdate() = 0;

    /**
     * @brief Retrieves the active state machine status.
     *
     * @return OtaState The current status of the update lifecycle.
     */
    virtual OtaState getState() const = 0;

    /**
     * @brief Retrieves the last error generated during the OTA execution cycle.
     *
     * @return sys_error_t Status error code mapping back to system errors.
     */
    virtual sys_error_t getLastError() const = 0;
};