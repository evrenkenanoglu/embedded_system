#include "Application/OTA/OtaManager.hpp"
#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

OtaManager::OtaManager(IOtaService& otaService, IHttpClient& httpClient)
    : _otaService(otaService)
    , _httpClient(httpClient)
    , _options()
    , _platformHooks()
    , _isInitialized(false)
    , _updatePending(false)
    , _pendingUrl("")
    , _pendingVersion("")
    , _pendingType("")
    , _pendingSize(0)
    , _pendingSha256("")
    , _pendingTargetSha("")
{
}

OtaManager::~OtaManager()
{
    deInit();
}

sys_error_t OtaManager::init(const OtaManagerOptions_t& options, const OtaPlatformHooks_t& hooks)
{
    RETURN_IF_ERROR((_isInitialized), ERROR_SUCCESS);

    _options        = options;
    _platformHooks  = hooks;
    _updatePending  = false;
    _isInitialized  = true;

    SYS_LOG_I("Platform-independent OTA Manager successfully initialized.");
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::deInit()
{
    RETURN_IF_ERROR((!_isInitialized), ERROR_SUCCESS);
    _updatePending = false;
    _isInitialized = false;
    return ERROR_SUCCESS;
}

uint32_t OtaManager::calculateNextCheckInterval() const
{
    if (_options.jitterRangeSec == 0 || !_platformHooks.getRandomNumber)
    {
        return _options.baseCheckIntervalSec;
    }

    // RULE 1: Jitter offset calculation using injected random source
    uint32_t randVal = _platformHooks.getRandomNumber();
    int32_t  offset  = (randVal % (_options.jitterRangeSec * 2)) - _options.jitterRangeSec;
    
    return _options.baseCheckIntervalSec + offset;
}

bool OtaManager::isTwoStageConditionMet()
{
    if (_options.bypassConditions)
    {
        SYS_LOG_D("Handshake preconditions bypassed via configuration.");
        return true;
    }

    // 1. Verify Battery Capacity Threshold
    if (_platformHooks.getBatteryPercentage)
    {
        uint8_t currentBat = _platformHooks.getBatteryPercentage();
        if (currentBat < _options.minBatteryPct)
        {
            SYS_LOG_W("Handshake deferred: low battery capacity (%d%%, required: %d%%)", 
                      currentBat, _options.minBatteryPct);
            return false;
        }
    }

    // 2. Verify Time Window Threshold
    if (_platformHooks.getCurrentLocalHour)
    {
        int currentHour = _platformHooks.getCurrentLocalHour();
        if (currentHour < 0)
        {
            SYS_LOG_W("Handshake deferred: system clock is currently unsynchronized.");
            return false;
        }
        if (currentHour < _options.allowedStartHour || currentHour >= _options.allowedEndHour)
        {
            SYS_LOG_W("Handshake deferred: outside of allowed traffic hour window (%02d:00 - %02d:00)", 
                      _options.allowedStartHour, _options.allowedEndHour);
            return false;
        }
    }

    return true;
}

sys_error_t OtaManager::checkForUpdates(OtaCheckResult& outResult)
{
    RETURN_IF_ERROR((!_isInitialized), ERROR_NOT_INITIALIZED, SYS_LOG_E("OTA Manager not initialized!"));

    _updatePending = false;

    // Standard client configuration options mapping
    HttpClientOptions_t clientOptions{};
    clientOptions.host            = "192.168.0.172"; 
    clientOptions.port            = 8443;
    clientOptions.use_tls         = true;
    clientOptions.server_cert_pem = _options.serverCert.c_str();
    clientOptions.server_cert_len = 0;
    clientOptions.timeout_ms      = 10000;

    sys_error_t err = _httpClient.connect(clientOptions);
    RETURN_IF_ERROR((err != ERROR_SUCCESS), err, SYS_LOG_E("Failed to connect to gateway update point."));

    std::vector<HttpHeader> checkHeaders;
    checkHeaders.push_back({"X-Device-API-Key", "secure-device-token-abcde"});
    checkHeaders.push_back({"x-ESP32-version", "1.0.0"}); 
    checkHeaders.push_back({"x-ESP32-hardware", "ESP32-S3-WROOM"});

    int                  statusCode = 0;
    std::vector<uint8_t> responsePayload;

    err = _httpClient.sendRequest(
        IHttpUri::HttpMethod::GET,
        "/api/v1/ota/check",
        checkHeaders,
        nullptr,
        0,
        statusCode,
        responsePayload
    );

    _httpClient.disconnect();

    if (err != ERROR_SUCCESS || statusCode != 200)
    {
        outResult = OtaCheckResult::ManifestFetchError;
        return ERROR_FAIL;
    }

    responsePayload.push_back('\0');
    std::string jsonStr(reinterpret_cast<const char*>(responsePayload.data()));

    return parseCheckResponse(jsonStr, outResult);
}

sys_error_t OtaManager::parseCheckResponse(const std::string& jsonStr, OtaCheckResult& outResult)
{
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (root == nullptr)
    {
        outResult = OtaCheckResult::ManifestParseError;
        return ERROR_FAIL;
    }

    cJSON* avail = cJSON_GetObjectItemCaseSensitive(root, "update_available");
    if (!cJSON_IsBool(avail) || !cJSON_IsTrue(avail))
    {
        outResult = OtaCheckResult::NoUpdateAvailable;
        cJSON_Delete(root);
        return ERROR_SUCCESS;
    }

    cJSON* url       = cJSON_GetObjectItemCaseSensitive(root, "url");
    cJSON* ver       = cJSON_GetObjectItemCaseSensitive(root, "version");
    cJSON* type      = cJSON_GetObjectItemCaseSensitive(root, "update_type");
    cJSON* size      = cJSON_GetObjectItemCaseSensitive(root, "size");
    cJSON* sha       = cJSON_GetObjectItemCaseSensitive(root, "sha256");
    cJSON* targetSha = cJSON_GetObjectItemCaseSensitive(root, "target_sha256");

    if (!cJSON_IsString(url) || !cJSON_IsString(ver) || !cJSON_IsString(type) || !cJSON_IsNumber(size))
    {
        outResult = OtaCheckResult::ManifestParseError;
        cJSON_Delete(root);
        return ERROR_FAIL;
    }

    _pendingUrl       = url->valuestring;
    _pendingVersion   = ver->valuestring;
    _pendingType      = type->valuestring;
    _pendingSize      = static_cast<size_t>(size->valuedouble);
    _pendingSha256    = cJSON_IsString(sha) ? sha->valuestring : "";
    _pendingTargetSha = cJSON_IsString(targetSha) ? targetSha->valuestring : "";

    _updatePending = true;
    outResult      = OtaCheckResult::UpdateAvailable;

    SYS_LOG_I("Stage 1 complete. Detected Pending Update Version: %s, Type: %s, Size: %zu bytes", 
              _pendingVersion.c_str(), _pendingType.c_str(), _pendingSize);

    cJSON_Delete(root);
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::executeUpdate()
{
    RETURN_IF_ERROR((!_isInitialized), ERROR_NOT_INITIALIZED, SYS_LOG_E("OTA Manager not initialized!"));
    RETURN_IF_ERROR((!_updatePending), ERROR_INVALID_STATE, SYS_LOG_E("No verified update pending execution."));

    // RULE 2: Evaluating Two-Stage Handshake preconditions
    if (!isTwoStageConditionMet())
    {
        SYS_LOG_I("Handshake check deferred execution: conditions not met yet.");
        return ERROR_SUCCESS; 
    }

    SYS_LOG_I("Preconditions verified. Starting Stage 2 Stream Download: Type [%s]", _pendingType.c_str());

    OtaOptions_t serviceOptions{};
    serviceOptions.chunkSize = 8192;
    serviceOptions.timeoutMs = 30000;

    auto progressCallback = [](OtaState state, size_t received, size_t total) {
        SYS_LOG_D("Download Progress: %zu/%zu bytes inside State: [%s]", received, total, stateToString(state));
    };

    sys_error_t err = _otaService.startUpdate(serviceOptions, progressCallback);
    if (err != ERROR_SUCCESS)
    {
        SYS_LOG_E("Download Stream execution transaction failed!");
        return err;
    }

    _updatePending = false;
    SYS_LOG_I("Stage 2 completed successfully. Ready for validation.");
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::validateCurrentFirmware()
{
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::rebootSystem()
{
    if (_platformHooks.rebootSystem)
    {
        SYS_LOG_I("Executing platform soft reboot via callback...");
        _platformHooks.rebootSystem();
        return ERROR_SUCCESS;
    }
    SYS_LOG_E("System reboot failed: No system reset callback registered.");
    return ERROR_FAIL;
}