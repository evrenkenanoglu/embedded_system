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

#include "PAL/Protocols/HTTP/IHttpClient.hpp"
#include "PAL/Protocols/OTA/IOtaTransport.hpp"

#include <string>

/**
 * @class OtaHttpTransport
 * @brief Concrete IOtaTransport implementing file download via HTTP/HTTPS.
 */
class OtaHttpTransport : public IOtaTransport
{
public:
    OtaHttpTransport(IHttpClient& httpClient, const std::string& url, const std::string& serverCert, uint32_t timeoutMs);

    ~OtaHttpTransport() override;

    sys_error_t connect() override;

    sys_error_t disconnect() override;

    sys_error_t startStream(OtaStreamCb_t callback) override;

    sys_error_t stopStream() override;

    size_t getExpectedSize() const override;

private:
    /**
     * @brief Direct extraction parser dividing URL targets.
     */
    sys_error_t _parseUrl(const std::string& url, std::string& outHost, std::string& outPath, int& outPort);

    IHttpClient& _httpClient;
    std::string  _url;
    std::string  _serverCert;
    uint32_t     _timeoutMs;

    std::string _host;
    std::string _path;
    int         _port;

    size_t        _expectedSize;
    bool          _isConnected;
    bool          _isStreaming;
    OtaStreamCb_t _streamCb;
};