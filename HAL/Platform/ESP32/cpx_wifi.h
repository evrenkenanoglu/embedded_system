/**
 * @file cpx_wifi.hpp
 * @brief Header file for cpx_wifi
 *
 * This file contains declarations for the cpx_wifi class and related data types and functions.
 */

#ifndef CPX_WIFI_HPP
#define CPX_WIFI_HPP

#include "HAL/IHal.h"
#include "System/ILog.h"
#include "System/system.h"
#include "esp_wifi_types.h"
#include <string>

class cpx_wifi : public IHAL_CPX
{
private:
    std::string _ssid;
    std::string _password;
    LogHandler& _logHandler;
    wifi_mode_t _wifiMode;

public:
    cpx_wifi(void* config, LogHandler& logHandler);
    ~cpx_wifi();

    sys_error_t start() override;

    void* get() override;

    void set(void* data) override;

    sys_error_t stop() override;

public:
    /**
     * @brief Set the Ssid object
     *
     * @param ssid
     */
    void setSsid(std::string& ssid);
    /**
     * @brief Set the Password object
     *
     * @param password
     */
    void setPassword(std::string& password);
    /**
     * @brief Set the Wifi Mode object
     *
     * @param mode
     */
    void setWifiMode(void* mode);
};

#endif /* CPX_WIFI_HPP */
