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
#include "HAL/IHAL/IHal.h"
#include "Library/Common/Crc.hpp"
#include "PAL/Protocols/HTTP/IHttpClient.hpp"
#include "PAL/Protocols/OTA/IOtaService.hpp"
#include "PAL/Protocols/OTA/OtaCheckpointManager.hpp"
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mutex>
#include <string>
#include <vector>

/**
 * @class OtaManager
 * @brief OTA orchestrator owning the FreeRTOS background lifecycle and update workflow.
 */
class OtaManager : public IOtaManager
{
public:
    OtaManager(IOtaService& otaService, IHttpClient& httpClient, uint32_t taskStackSize = 16 * 1024, UBaseType_t taskPriority = tskIDLE_PRIORITY + 1);
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
    sys_error_t startAutomatic(const OtaManagerStartupConfig_t& startupConfig) override;
    sys_error_t stopAutomatic() override;
    bool        isAutomaticRunning() const override;
    sys_error_t executeUpdate() override;

    sys_error_t validateCurrentFirmware() override;
    sys_error_t loadTrustAnchorsFromStorage(IHAL_MEM& factoryMem) override;
    void        rebootSystem() override;
    sys_error_t registerSelfTest(const std::string& name, OtaSelfTestHook_t testHook) override;
    sys_error_t processPendingRollbackTelemetry() override;

    /**
     * @brief Computes next periodic trigger interval applying standard randomization.
     *
     * @return uint32_t Target timeout in seconds.
     */
    uint32_t calculateNextCheckInterval() const;

    /**
     * @brief Injects an optional checkpoint manager to enable download resumption across power cuts.
     *
     * @param[in] checkpointMgr Pointer to an instantiated OtaCheckpointManager.
     */
    void setCheckpointManager(OtaCheckpointManager* checkpointMgr);

    /**
     * @brief Injects an NVS storage device for recording and inspecting rollback diagnostics.
     *
     * @param[in] diagMem Pointer to an IHAL_MEM instance.
     * @return sys_error_t ERROR_SUCCESS if storage initialization succeeds.
     */
    sys_error_t setDiagnosticStorage(IHAL_MEM* diagMem);

private:
    static void _automaticTaskEntry(void* param);
    void        _automaticWorker();
    bool        _isTwoStageConditionMet();
    sys_error_t _parseCheckResponse(const std::string& jsonStr, OtaCheckResult& outResult);
    sys_error_t _parseUrl(const std::string& url, std::string& outHost, std::string& outPath, int& outPort, bool& outIsHttps);
    sys_error_t _sendTelemetryReport(const std::string& statusStr, sys_error_t errorCode);

private:
    mutable std::recursive_mutex _lifecycleMutex;
    mutable std::recursive_mutex _mutex;
    IOtaService&                 _otaService;
    IHttpClient&                 _httpClient;
    const uint32_t               _taskStackSize;
    const UBaseType_t            _taskPriority;
    std::atomic<TaskHandle_t>    _taskHandle{nullptr};
    std::atomic<bool>            _stopRequested{false};
    std::atomic<bool>            _automaticRunning{false};
    OtaManagerOptions_t          _options;
    OtaPlatformHooks_t           _platformHooks;
    OtaManagerStartupConfig_t    _startupConfig;
    bool                         _isInitialized;
    OtaCheckpointManager*        _checkpointMgr{nullptr};
    IHAL_MEM*                    _diagStorage{nullptr};
    IHal_Sys_Rng*                _rngDriver{nullptr};
    IHal_Sys_ResetReason*        _resetReasonDriver{nullptr};

    // Transient target context
    bool        _updatePending;
    std::string _pendingUrl;
    std::string _pendingVersion;
    std::string _pendingType;
    size_t      _pendingSize;
    std::string _pendingSignature;
    std::string _pendingTargetSignature;
    std::string _pendingSigningCert;

    // Self-test diagnostic pipeline
    struct SelfTestEntry
    {
        std::string       name;
        OtaSelfTestHook_t hook;
    };
    std::vector<SelfTestEntry> _selfTests;
};
