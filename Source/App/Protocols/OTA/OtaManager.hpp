#pragma once

#include "PAL/Protocols/OTA/IOtaManager.hpp"
#include "PAL/Protocols/OTA/IOtaService.hpp"
#include "PAL/Protocols/IHttpClient.hpp"
#include <string>
#include <functional>
#include <vector>

struct OtaManagerOptions_t
{
    std::string gatewayUrl;                   // Gateway endpoint path
    std::string serverCert;                   // Trust anchor certificate
    uint32_t    baseCheckIntervalSec = 86400; // Periodic check baseline (24 hours)
    uint32_t    jitterRangeSec       = 1800;  // Maximum timing offset (30 minutes)
    uint8_t     minBatteryPct        = 80;    // Threshold for stage 2 download
    uint8_t     allowedStartHour     = 2;     // Low traffic window start hour (2 AM)
    uint8_t     allowedEndHour       = 4;     // Low traffic window end hour (4 AM)
    bool        bypassConditions     = false; // Bypasses conditions for local testing
};

// Platform abstraction hooks to decouple hardware peripherals
struct OtaPlatformHooks_t
{
    std::function<uint8_t()>  getBatteryPercentage = nullptr; // Returns capacity (0-100%)
    std::function<int()>      getCurrentLocalHour   = nullptr; // Returns local hour (0-23) or -1 if clock is unsynced
    std::function<uint32_t()> getRandomNumber       = nullptr; // Returns a 32-bit random integer
    std::function<void()>     rebootSystem          = nullptr; // Triggers system soft-reboot
};

class OtaManager : public IOtaManager
{
public:
    OtaManager(IOtaService& otaService, IHttpClient& httpClient);
    virtual ~OtaManager();

    sys_error_t init(const OtaManagerOptions_t& options, const OtaPlatformHooks_t& hooks);
    sys_error_t deInit() override;

    // Stage 1 Check: Evaluates update availability and metadata
    sys_error_t checkForUpdates(OtaCheckResult& outResult) override;

    // Stage 2 Download: Decouples streaming and checks conditions
    sys_error_t executeUpdate() override;

    sys_error_t validateCurrentFirmware() override;
    sys_error_t rebootSystem() override;

    // RULE 1: Spreads gateway request load over time
    uint32_t calculateNextCheckInterval() const;

private:
    bool isTwoStageConditionMet();
    sys_error_t parseCheckResponse(const std::string& jsonStr, OtaCheckResult& outResult);

    IOtaService&         _otaService;
    IHttpClient&         _httpClient;
    OtaManagerOptions_t  _options;
    OtaPlatformHooks_t   _platformHooks;
    bool                 _isInitialized;

    // Transient update context parameters
    bool                 _updatePending;
    std::string          _pendingUrl;
    std::string          _pendingVersion;
    std::string          _pendingType;       // "delta" or "full"
    size_t               _pendingSize;
    std::string          _pendingSha256;     // Stream payload checksum
    std::string          _pendingTargetSha;  // Reconstructed binary checksum (Rule 3)
};