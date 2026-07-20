/** @file       OtaManager.cpp
 *  @brief      Concrete implementation of the application-level firmware update orchestrator.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#include "App/Protocols/OTA/OtaManager.hpp"
#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

OtaManager::OtaManager(IOtaService& otaService, IHttpClient& httpClient)
    : _mutex()
    , _otaService(otaService)
    , _httpClient(httpClient)
    , _options()
    , _platformHooks()
    , _isInitialized(false)
    , _updatePending(false)
    , _pendingUrl("")
    , _pendingVersion("")
    , _pendingType("")
    , _pendingSize(0)
    , _pendingSignature("")
    , _pendingTargetSignature("")
    , _pendingSigningCert("")
{
}

OtaManager::~OtaManager()
{
    deInit();
}

sys_error_t OtaManager::init(const OtaManagerOptions_t& options, const OtaPlatformHooks_t& hooks)
{
    std::lock_guard<std::mutex> lock(_mutex);

    RETURN_IF_ERROR(
        (_isInitialized), // Expression
        ERROR_SUCCESS     // Error code
    );

    sys_error_t err = _otaService.init();
    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS),                                  // Expression
        err,                                                     // Error code
        SYS_LOG_E("Failed to initialize underlying OtaService!") // Error message
    );

    _options       = options;
    _platformHooks = hooks;
    _updatePending = false;
    _isInitialized = true;

    SYS_LOG_I("Platform-independent OTA Manager successfully initialized.");
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::deInit()
{
    std::lock_guard<std::mutex> lock(_mutex);

    RETURN_IF_ERROR(
        (!_isInitialized), // Expression
        ERROR_SUCCESS      // Error code
    );

    _otaService.deInit();
    _updatePending = false;
    _isInitialized = false;
    return ERROR_SUCCESS;
}

uint32_t OtaManager::calculateNextCheckInterval() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_options.jitterRangeSec == 0 || !_platformHooks.getRandomNumber)
    {
        return _options.baseCheckIntervalSec;
    }

    uint32_t randVal = _platformHooks.getRandomNumber();
    int32_t  offset  = (randVal % (_options.jitterRangeSec * 2)) - _options.jitterRangeSec;

    return _options.baseCheckIntervalSec + offset;
}

bool OtaManager::_isTwoStageConditionMet()
{
    if (_options.bypassConditions)
    {
        SYS_LOG_D("Handshake preconditions bypassed via configuration.");
        return true;
    }

    if (_platformHooks.getBatteryPercentage)
    {
        uint8_t currentBat = _platformHooks.getBatteryPercentage();
        if (currentBat < _options.minBatteryPct)
        {
            SYS_LOG_W("Handshake deferred: low battery capacity (%d%%, required: %d%%)", currentBat, _options.minBatteryPct);
            return false;
        }
    }

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
            SYS_LOG_W("Handshake deferred: outside of allowed traffic hour window (%02d:00 - %02d:00)", _options.allowedStartHour, _options.allowedEndHour);
            return false;
        }
    }

    return true;
}

sys_error_t OtaManager::checkForUpdates(OtaCheckResult& outResult)
{
    std::lock_guard<std::mutex> lock(_mutex);

    RETURN_IF_ERROR(
        (!_isInitialized),                        // Expression
        ERROR_NOT_INITIALIZED,                    // Error code
        SYS_LOG_E("OTA Manager not initialized!") // Error message
    );

    _updatePending = false;

    std::string host;
    std::string path;
    int         port    = 80;
    bool        isHttps = false;

    sys_error_t parseErr = _parseUrl(_options.gatewayUrl, host, path, port, isHttps);
    RETURN_IF_ERROR(
        (parseErr != ERROR_SUCCESS),                                              // Expression
        parseErr,                                                                 // Error code
        SYS_LOG_E("Failed to parse resource address targets from configuration!") // Error message
    );

    HttpClientOptions_t clientOptions{};
    clientOptions.host            = host;
    clientOptions.port            = port;
    clientOptions.use_tls         = isHttps;
    clientOptions.server_cert_pem = _options.serverCert.empty() ? nullptr : _options.serverCert.c_str();
    clientOptions.server_cert_len = 0;
    clientOptions.timeout_ms      = 10000;

    sys_error_t err = _httpClient.connect(clientOptions);
    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS),                                 // Expression
        err,                                                    // Error code
        SYS_LOG_E("Failed to connect to gateway update point.") // Error message
    );

    std::vector<HttpHeader> checkHeaders;
    checkHeaders.push_back({"X-Device-API-Key", _options.apiKey});
    checkHeaders.push_back({"x-ESP32-version", _options.currentVersion});
    checkHeaders.push_back({"x-ESP32-hardware", _options.hardwareType});
    checkHeaders.push_back({"x-ESP32-device-id", _options.deviceId});
    checkHeaders.push_back({"x-ESP32-channel", _options.channel});
    checkHeaders.push_back({"x-ESP32-hsvn", std::to_string(_options.currentHsvn)});

    int                  statusCode = 0;
    std::vector<uint8_t> responsePayload;

    err = _httpClient.sendRequest(IHttpUri::HttpMethod::GET, "/api/v1/ota/check", checkHeaders, nullptr, 0, statusCode, responsePayload);

    _httpClient.disconnect();

    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS || statusCode != 200),                                  // Expression
        (err != ERROR_SUCCESS) ? err : ERROR_FAIL,                                    // Error code
        SYS_LOG_E("Request rejected or communication failure during manifest query.") // Error message
    );

    RETURN_IF_ERROR(
        (responsePayload.max_size() == responsePayload.size()),               // Expression
        ERROR_OUT_OF_MEMORY,                                                  // Error code
        SYS_LOG_E("OOM constraint breached during parsing segment assembly.") // Error message
    );

    responsePayload.push_back('\0');
    std::string jsonStr(reinterpret_cast<const char*>(responsePayload.data()));

    return _parseCheckResponse(jsonStr, outResult);
}

sys_error_t OtaManager::_parseCheckResponse(const std::string& jsonStr, OtaCheckResult& outResult)
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
    cJSON* size      = cJSON_GetObjectItemCaseSensitive(root, "target_size");
    cJSON* sha       = cJSON_GetObjectItemCaseSensitive(root, "signature");
    cJSON* targetSha = cJSON_GetObjectItemCaseSensitive(root, "target_signature");
    cJSON* cert      = cJSON_GetObjectItemCaseSensitive(root, "signing_cert");

    if (!cJSON_IsString(url) || !cJSON_IsString(ver) || !cJSON_IsString(type) || !cJSON_IsNumber(size))
    {
        outResult = OtaCheckResult::ManifestParseError;
        cJSON_Delete(root);
        return ERROR_FAIL;
    }

    _pendingUrl             = url->valuestring;
    _pendingVersion         = ver->valuestring;
    _pendingType            = type->valuestring;
    _pendingSize            = static_cast<size_t>(size->valuedouble);
    _pendingSignature       = cJSON_IsString(sha) ? sha->valuestring : "";
    _pendingTargetSignature = cJSON_IsString(targetSha) ? targetSha->valuestring : "";
    _pendingSigningCert     = cJSON_IsString(cert) ? cert->valuestring : "";

    _updatePending = true;
    outResult      = OtaCheckResult::UpdateAvailable;

    SYS_LOG_I("Stage 1 complete. Target Version: %s, Type: %s, Size: %zu", _pendingVersion.c_str(), _pendingType.c_str(), _pendingSize);

    cJSON_Delete(root);
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::executeUpdate()
{
    std::lock_guard<std::mutex> lock(_mutex);

    RETURN_IF_ERROR(
        (!_isInitialized),                        // Expression
        ERROR_NOT_INITIALIZED,                    // Error code
        SYS_LOG_E("OTA Manager not initialized!") // Error message
    );

    RETURN_IF_ERROR(
        (!_updatePending),                                 // Expression
        ERROR_INVALID_STATE,                               // Error code
        SYS_LOG_E("No verified update pending execution.") // Error message
    );

    if (!_isTwoStageConditionMet())
    {
        SYS_LOG_I("Handshake check deferred execution: conditions not met yet.");
        return ERROR_INVALID_STATE;
    }

    SYS_LOG_I("Preconditions verified. Starting Stage 2 Stream Download: Type [%s]", _pendingType.c_str());

    OtaOptions_t serviceOptions{};
    serviceOptions.endpoint        = _pendingUrl;
    serviceOptions.serverCert      = _options.serverCert;
    serviceOptions.chunkSize       = 8192;
    serviceOptions.timeoutMs       = 30000;
    serviceOptions.signature       = _pendingSignature;
    serviceOptions.targetSignature = _pendingTargetSignature;
    serviceOptions.signingCert     = _pendingSigningCert;
    serviceOptions.isDelta         = (_pendingType == "delta");
    serviceOptions.targetSize      = _pendingSize;

    auto progressCallback = [](OtaState state, size_t received, size_t total)
    {
        if (total > 0)
        {
            float percent = (static_cast<float>(received) / static_cast<float>(total)) * 100.0f;
            SYS_LOG_D("Progress: %zu/%zu bytes (%.2f%%)", received, total, percent);
        }
    };

    sys_error_t err = _otaService.startUpdate(serviceOptions, progressCallback);

    if (err != ERROR_SUCCESS)
    {
        SYS_LOG_E("Download Stream execution transaction failed!");
        _sendTelemetryReport("failure", err);
        return err;
    }

    _sendTelemetryReport("success", ERROR_SUCCESS);

    _updatePending = false;
    SYS_LOG_I("Stage 2 and Stage 3 completed successfully. Ready for validation.");
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::_sendTelemetryReport(const std::string& statusStr, sys_error_t errorCode)
{
    std::string host;
    std::string path;
    int         port    = 80;
    bool        isHttps = false;

    sys_error_t parseErr = _parseUrl(_options.gatewayUrl, host, path, port, isHttps);
    RETURN_IF_ERROR(
        (parseErr != ERROR_SUCCESS),                                // Expression
        parseErr,                                                   // Error code
        SYS_LOG_E("Failed to parse telemetry gateway URL address!") // Error message
    );

    HttpClientOptions_t telemetryOptions{};
    telemetryOptions.host            = host;
    telemetryOptions.port            = port;
    telemetryOptions.use_tls         = isHttps;
    telemetryOptions.server_cert_pem = _options.serverCert.empty() ? nullptr : _options.serverCert.c_str();
    telemetryOptions.server_cert_len = 0;
    telemetryOptions.timeout_ms      = 10000;

    sys_error_t err = _httpClient.connect(telemetryOptions);
    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS),                                            // Expression
        err,                                                               // Error code
        SYS_LOG_E("Failed to connect telemetry client to gateway server!") // Error message
    );

    std::vector<HttpHeader> headers;
    headers.push_back({"X-Device-API-Key", _options.apiKey});
    headers.push_back({"Content-Type", "application/json"});

    cJSON* report = cJSON_CreateObject();
    if (report == nullptr)
    {
        _httpClient.disconnect();
        return ERROR_FAIL;
    }

    cJSON_AddStringToObject(report, "device_id", _options.deviceId.c_str());
    cJSON_AddStringToObject(report, "previous_version", _options.currentVersion.c_str());
    cJSON_AddStringToObject(report, "target_version", _pendingVersion.c_str());
    cJSON_AddStringToObject(report, "status", statusStr.c_str());
    cJSON_AddNumberToObject(report, "error_code", static_cast<double>(errorCode));

    char* jsonString = cJSON_PrintUnformatted(report);
    cJSON_Delete(report);

    if (jsonString == nullptr)
    {
        _httpClient.disconnect();
        return ERROR_FAIL;
    }

    int                  statusCode = 0;
    std::vector<uint8_t> response;

    err = _httpClient.sendRequest(
        IHttpUri::HttpMethod::POST, "/api/v1/ota/status", headers, reinterpret_cast<const uint8_t*>(jsonString), std::strlen(jsonString), statusCode, response);

    std::free(jsonString);
    _httpClient.disconnect();

    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS || statusCode != 200),                                // Expression
        ERROR_FAIL,                                                                 // Error code
        SYS_LOG_E("Failed to report telemetry status. HTTP Status: %d", statusCode) // Error message
    );

    SYS_LOG_I("Telemetry state reported successfully: %s", statusStr.c_str());
    return ERROR_SUCCESS;
}

sys_error_t OtaManager::validateCurrentFirmware()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return ERROR_SUCCESS;
}

void OtaManager::rebootSystem()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_platformHooks.rebootSystem)
    {
        SYS_LOG_I("Executing platform soft reboot via callback...");
        _platformHooks.rebootSystem();
        return;
    }

    SYS_LOG_E("System reboot failed: No system reset callback registered.");
}

sys_error_t OtaManager::_parseUrl(const std::string& url, std::string& outHost, std::string& outPath, int& outPort, bool& outIsHttps)
{
    RETURN_IF_ERROR(
        (url.empty()),                                    // Expression
        ERROR_INVALID_ARG,                                // Error code
        SYS_LOG_E("Address resolution parameters empty!") // Error message
    );

    const std::string protocolDelimiter = "://";
    size_t            protocolPos       = url.find(protocolDelimiter);
    size_t            hostStart         = (protocolPos == std::string::npos) ? 0 : protocolPos + protocolDelimiter.length();

    outIsHttps = false;
    if (protocolPos != std::string::npos)
    {
        std::string protocol = url.substr(0, protocolPos);
        std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);
        if (protocol == "https")
        {
            outIsHttps = true;
        }
    }

    size_t      pathStart = url.find('/', hostStart);
    std::string hostPortSegment;
    if (pathStart == std::string::npos)
    {
        hostPortSegment = url.substr(hostStart);
        outPath         = "/";
    }
    else
    {
        hostPortSegment = url.substr(hostStart, pathStart - hostStart);
        outPath         = url.substr(pathStart);
    }

    size_t colonPos = hostPortSegment.find(':');
    if (colonPos == std::string::npos)
    {
        outHost = hostPortSegment;
        outPort = outIsHttps ? 443 : 80;
    }
    else
    {
        outHost = hostPortSegment.substr(0, colonPos);

        std::string portStr = hostPortSegment.substr(colonPos + 1);
        if (portStr.empty())
        {
            return ERROR_INVALID_ARG;
        }

        char* endptr  = nullptr;
        long  portVal = std::strtol(portStr.c_str(), &endptr, 10);

        RETURN_IF_ERROR(
            (endptr == portStr.c_str() || *endptr != '\0' || portVal < 0 || portVal > 65535), // Expression
            ERROR_INVALID_ARG,                                                                // Error code
            SYS_LOG_E("Parsed port number is invalid or out of range!")                       // Error message
        );

        outPort = static_cast<int>(portVal);
    }

    return ERROR_SUCCESS;
}