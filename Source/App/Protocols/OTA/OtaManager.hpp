/** @file       OtaManager.hpp
 *  @brief      Concrete implementation of the application-level firmware update orchestrator.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#pragma once

#include "App/Protocols/OTA/IOtaManager.hpp"
#include "PAL/Protocols/IHttpClient.hpp"
#include "PAL/Protocols/OTA/IOtaService.hpp"
#include <mutex>
#include <string>

/**
 * @class OtaManager
 * @brief Platform-independent manager implementing the two-stage conditional firmware update.
 */
class OtaManager : public IOtaManager
{
public:
    OtaManager(IOtaService& otaService, IHttpClient& httpClient);
    ~OtaManager() override;

    // Explicitly block copy mechanics to enforce unique ownership
    OtaManager(const OtaManager&)            = delete;
    OtaManager& operator=(const OtaManager&) = delete;

    // Explicitly block move mechanics
    OtaManager(OtaManager&&)            = delete;
    OtaManager& operator=(OtaManager&&) = delete;

public:
    sys_error_t init(const OtaManagerOptions_t& options, const OtaPlatformHooks_t& hooks) override;
    sys_error_t deInit() override;

    sys_error_t checkForUpdates(OtaCheckResult& outResult) override;
    sys_error_t executeUpdate() override;

    sys_error_t validateCurrentFirmware() override;
    sys_error_t rebootSystem() override;

    /**
     * @brief Computes next periodic trigger interval applying standard randomization.
     *
     * @return uint32_t Target timeout in seconds.
     */
    uint32_t calculateNextCheckInterval() const;

private:
    bool        _isTwoStageConditionMet();
    sys_error_t _parseCheckResponse(const std::string& jsonStr, OtaCheckResult& outResult);
    sys_error_t _parseUrl(const std::string& url, std::string& outHost, std::string& outPath, int& outPort, bool& outIsHttps);
    sys_error_t _sendTelemetryReport(const std::string& statusStr, sys_error_t errorCode);

private:
    mutable std::mutex  _mutex;
    IOtaService&        _otaService;
    IHttpClient&        _httpClient;
    OtaManagerOptions_t _options;
    OtaPlatformHooks_t  _platformHooks;
    bool                _isInitialized;

    // Transient target context
    bool        _updatePending;
    std::string _pendingUrl;
    std::string _pendingVersion;
    std::string _pendingType;
    size_t      _pendingSize;
    std::string _pendingSignature;
    std::string _pendingTargetSignature;
    std::string _pendingSigningCert;
};