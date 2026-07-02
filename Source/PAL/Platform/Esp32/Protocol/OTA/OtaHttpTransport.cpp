/** @file       OtaHttpTransport.cpp
 *  @brief      Concrete implementation of the HTTP(S) data streaming transport.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#include "PAL/Platform/Esp32/Protocol/OTA/OtaHttpTransport.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <esp_http_client.h>

#include <algorithm>
#include <cstdlib>

OtaHttpTransport::OtaHttpTransport(IHttpClient& httpClient, const std::string& url, const std::string& serverCert, uint32_t timeoutMs)
    : _httpClient(httpClient)
    , _url(url)
    , _serverCert(serverCert)
    , _timeoutMs(timeoutMs)
    , _host("")
    , _path("")
    , _port(0)
    , _expectedSize(0)
    , _isConnected(false)
    , _isStreaming(false)
    , _streamCb(nullptr)
{
}

OtaHttpTransport::~OtaHttpTransport()
{
    disconnect();
}

sys_error_t OtaHttpTransport::connect()
{
    RETURN_IF_ERROR((_isConnected), ERROR_SUCCESS);

    /// Dynamic URL Resolution
    bool        isHttps = false;
    sys_error_t err     = _parseUrl(_url, _host, _path, _port, isHttps);
    RETURN_IF_ERROR((err != ERROR_SUCCESS), err, SYS_LOG_E("Failed to parse resource address targets!"));

    /// Connection Setup
    HttpClientOptions_t clientOptions{};
    clientOptions.host       = _host;
    clientOptions.port       = _port;
    clientOptions.use_tls    = isHttps; // Set by parsing results instead of hardcoded port verification
    clientOptions.timeout_ms = _timeoutMs;
    clientOptions.keep_alive = true;

    if (clientOptions.use_tls && !_serverCert.empty())
    {
        clientOptions.server_cert_pem = _serverCert.c_str();
        clientOptions.server_cert_len = 0; // Must be 0 for PEM certificates
    }

    err = _httpClient.connect(clientOptions);
    RETURN_IF_ERROR((err != ERROR_SUCCESS), err, SYS_LOG_E("Underlying HTTP socket failed to connect!"));

    _isConnected = true;
    return ERROR_SUCCESS;
}

sys_error_t OtaHttpTransport::disconnect()
{
    RETURN_IF_ERROR((!_isConnected), ERROR_SUCCESS);

    _httpClient.disconnect();
    _isConnected = false;
    _isStreaming = false;

    return ERROR_SUCCESS;
}

sys_error_t OtaHttpTransport::startStream(OtaStreamCb_t callback)
{
    RETURN_IF_ERROR((!_isConnected), ERROR_NOT_INITIALIZED, SYS_LOG_E("HTTP Transport not connected!"));

    _streamCb     = callback;
    _isStreaming  = true;
    _expectedSize = 0;

    int                     statusCode = 0;
    std::vector<HttpHeader> headers;

    /// Server Configuration Header Mapping
    headers.push_back({"X-Device-API-Key", "secure-device-token-abcde"}); // Matches server API_KEY
    headers.push_back({"x-ESP32-version", "1.0.0"});                      // Matches server HEADER_VERSION_KEY
    headers.push_back({"x-ESP32-hardware", "ESP32-S3-WROOM"});            // Matches server HEADER_HARDWARE_KEY

    /// Inline Wrapper Pipeline
    auto intermediateCb = [this](const uint8_t* chunk, size_t chunkLen, bool isLastChunk) -> sys_error_t
    {
        if (!_isStreaming)
        {
            return ERROR_FAIL;
        }

        /// Content Length Calculation
        if (_expectedSize == 0 && _httpClient.nativeHandle() != nullptr)
        {
            esp_http_client_handle_t espClient = reinterpret_cast<esp_http_client_handle_t>(_httpClient.nativeHandle());
            int64_t                  len       = esp_http_client_get_content_length(espClient);
            if (len > 0)
            {
                _expectedSize = static_cast<size_t>(len);
            }
        }

        if (_streamCb)
        {
            return _streamCb(chunk, chunkLen, isLastChunk);
        }

        return ERROR_SUCCESS;
    };

    sys_error_t err = _httpClient.sendRequest(
        IHttpUri::HttpMethod::GET,
        _path,
        headers, // Updated vector containing server authentication headers
        nullptr,
        0,
        statusCode,
        intermediateCb);

    _isStreaming = false;

    RETURN_IF_ERROR(
        (err != ERROR_SUCCESS || statusCode != 200), (err != ERROR_SUCCESS) ? err : ERROR_FAIL, SYS_LOG_E("HTTP request rejected by OTA Server! Status: %d", statusCode));

    return ERROR_SUCCESS;
}

sys_error_t OtaHttpTransport::stopStream()
{
    _isStreaming = false;
    return ERROR_SUCCESS;
}

size_t OtaHttpTransport::getExpectedSize() const
{
    return _expectedSize;
}

sys_error_t OtaHttpTransport::_parseUrl(const std::string& url, std::string& outHost, std::string& outPath, int& outPort, bool& outIsHttps)
{
    RETURN_IF_ERROR((url.empty()), ERROR_INVALID_ARG, SYS_LOG_E("Address resolution parameters empty!"));

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

        /// Non-throwing Integer Parsing
        char* endptr  = nullptr;
        long  portVal = std::strtol(portStr.c_str(), &endptr, 10);

        RETURN_IF_ERROR(
            (endptr == portStr.c_str() || *endptr != '\0' || portVal < 0 || portVal > 65535), ERROR_INVALID_ARG, SYS_LOG_E("Parsed port number is invalid or out of range!"));

        outPort = static_cast<int>(portVal);
    }

    return ERROR_SUCCESS;
}