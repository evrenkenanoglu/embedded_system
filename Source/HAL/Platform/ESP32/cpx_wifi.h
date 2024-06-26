/**
 * @file cpx_wifi.hpp
 * @brief Header file for cpx_wifi
 *
 * This file contains declarations for the cpx_wifi class and related data types and functions.
 */

#ifndef CPX_WIFI_HPP
#define CPX_WIFI_HPP

#include "HAL/IHal.h"
#include "System/error_definitions.h"
#include "esp_wifi_types.h"
#include <string>

class cpx_wifi : public IHAL_CPX
{
private:
    std::string   _ssid;
    std::string   _password;
    wifi_mode_t   _wifiMode;
    wifi_config_t _wifiConfig;

private:
    sys_error_t wifiInit();
    sys_error_t wifiStart();

public:
    cpx_wifi(void* config);
    ~cpx_wifi();

    sys_error_t start() override;

    void* get() override;

    void set(void* data) override;

    sys_error_t stop() override;

public:
    /**
     * @brief Set the Wifi Mode object
     *
     * @param mode
     */
    void setWifiMode(wifi_mode_t mode);
};

#endif /* CPX_WIFI_HPP */
