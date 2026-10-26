/** @file       uri_wifiScan.hpp
 *  @brief      WiFi scan HTTP URI (GET /wifiScan).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#ifndef URI_WIFISCAN_HPP
#define URI_WIFISCAN_HPP

#include "App/Network/WifiConfigurationManager.hpp"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"

/**
 * @class UriWiFiScan
 * @brief Serves the WiFi access-point scan as JSON.
 *
 * Requests a scan through the platform-independent connection manager and
 * formats the typed HAL scan results; no FreeRTOS primitives leak here.
 */
class UriWiFiScan : public HttpUriGet
{
public:
    explicit UriWiFiScan(WifiConfigurationManager& wifiMgr);
    ~UriWiFiScan();

private:
    uint16_t handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx) override;

private:
    WifiConfigurationManager& _wifiMgr;
};

#endif // URI_WIFISCAN_HPP
