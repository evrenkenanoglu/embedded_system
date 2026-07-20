/** @file       OtaHttpTransport.hpp
 *  @brief      Concrete implementation of the HTTP(S) data streaming transport.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       27/06/2026
 */

#pragma once

// 1. Local Project / Protocol / HAL Headers
#include "PAL/Protocols/HTTP/IHttpClient.hpp"
#include "PAL/Protocols/OTA/IOtaTransport.hpp"

// 2. C++ Standard Library Headers
#include <string>
#include <vector>

/**
 * @class OtaHttpTransport
 * @brief Concrete IOtaTransport implementing file download via HTTP/HTTPS.
 *
 * @note Thread-Safety: This class is not thread-safe and must be externally synchronized
 *       if accessed by concurrent tasks.
 */
class OtaHttpTransport : public IOtaTransport
{
public:
    /**
     * @brief Construct a new OtaHttpTransport object.
     *
     * @param[in] httpClient Reference to the underlying HTTP client interface.
     * @param[in] serverCert Root certificate string for server validation.
     * @param[in] timeoutMs  Socket timeout in milliseconds.
     */
    OtaHttpTransport(IHttpClient& httpClient, const std::string& serverCert, uint32_t timeoutMs);

    /**
     * @brief Destroy the OtaHttpTransport object.
     */
    ~OtaHttpTransport() override;

    // Explicitly block copy mechanics to enforce unique ownership
    OtaHttpTransport(const OtaHttpTransport&)            = delete;
    OtaHttpTransport& operator=(const OtaHttpTransport&) = delete;

    // Explicitly block move mechanics unless specifically designed
    OtaHttpTransport(OtaHttpTransport&&)            = delete;
    OtaHttpTransport& operator=(OtaHttpTransport&&) = delete;

public:
    sys_error_t connect(const std::string& endpoint) override;
    sys_error_t disconnect() override;
    sys_error_t startStream(OtaStreamCb_t callback) override;
    sys_error_t stopStream() override;
    size_t      getExpectedSize() const override;

    /**
     * @brief Configure custom verification headers to be sent with the stream download request.
     *
     * @param[in] headers Vector of custom HTTP headers.
     */
    void setHeaders(const std::vector<HttpHeader>& headers);

private:
    /**
     * @brief Direct extraction parser dividing URL targets.
     *
     * @param[in]  url        Target URL string to parse.
     * @param[out] outHost    Extracted host name or IP address.
     * @param[out] outPath    Extracted resource path.
     * @param[out] outPort    Extracted port number.
     * @param[out] outIsHttps Set to true if the schema is HTTPS.
     *
     * @return sys_error_t ERROR_SUCCESS if parsing succeeded, otherwise an error status code.
     */
    sys_error_t _parseUrl(const std::string& url, std::string& outHost, std::string& outPath, int& outPort, bool& outIsHttps);

private:
    IHttpClient& _httpClient;
    std::string  _serverCert;
    uint32_t     _timeoutMs;

    std::string _host;
    std::string _path;
    int         _port;

    size_t                  _expectedSize;
    bool                    _isConnected;
    bool                    _isStreaming;
    OtaStreamCb_t           _streamCb;
    std::vector<HttpHeader> _customHeaders;
};