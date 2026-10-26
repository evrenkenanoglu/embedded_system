/** @file       uri_wifiConnect.hpp
 *  @brief      WiFi credentials provisioning HTTP URI (POST /wifiConnect).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#ifndef URI_WIFICONNECT_HPP
#define URI_WIFICONNECT_HPP

#include "App/Network/WifiConfigurationManager.hpp"
#include "HAL/IHAL/IHal.h"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriPost.hpp"

/**
 * @class UriWifiConnect
 * @brief Accepts SSID/password JSON, persists it and asks the WiFi manager to connect.
 */
class UriWifiConnect : public HttpUriPost
{
public:
    UriWifiConnect(WifiConfigurationManager& wifiMgr, IHAL_MEM& memDevice);
    ~UriWifiConnect();

private:
    uint16_t handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx) override;

private:
    WifiConfigurationManager& _wifiMgr;
    IHAL_MEM&                 _memDevice;
};

#endif // URI_WIFICONNECT_HPP
