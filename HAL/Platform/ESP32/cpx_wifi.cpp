/**
 * @file cpx_wifi.cpp
 * @brief Source file for cpx_wifi
 *
 * This file contains definitions for the cpx_wifi class and related data types and functions.
 */

#include "cpx_wifi.h"

#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "HAL/Platform/ESP32/Library/logImpl.h"

// @todo change the HAL type to CPX, it doesn't fit to com device description.
cpx_wifi::cpx_wifi(void* config, LogHandler& logHandler) : _logHandler(logHandler), _wifiMode(WIFI_MODE_NULL)
{
    esp_netif_t* wifi_netif = NULL;
    // @todo later this configuration has to be passed
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t      wifi_config;

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_netif = esp_netif_create_default_wifi_sta();
    assert(wifi_netif);

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // wifi_default_config(&wifi_config);
}

cpx_wifi::~cpx_wifi()
{
    // destructor implementation
}

sys_error_t cpx_wifi::start()
{
    switch (_wifiMode)
    {
        case WIFI_MODE_STA: /**< WiFi station mode */
        {
            _logHandler.log(ILog::LogLevel::WARNING, " NOT IMPLEMENTED: WIFI_MODE_STA");
            return ERROR_NOT_IMPLEMENTED;
        }
        break;

        case WIFI_MODE_AP: /**< WiFi soft-AP mode */
        {
            _logHandler.log(ILog::LogLevel::WARNING, " NOT IMPLEMENTED: WIFI_MODE_AP");
            return ERROR_NOT_IMPLEMENTED;
        }
        break;

        case WIFI_MODE_APSTA: /**< WiFi station + soft-AP mode */
        {
            return ERROR_NOT_IMPLEMENTED;
        }
        break;

        default:
            _logHandler.log(ILog::LogLevel::ERROR, "WIFI Mode not set yet!");
            return ERROR_INVALID_CONFIG;
            break;
    }

    // @todo Take action depending on wifi mode!
}

void* cpx_wifi::get()
{

    return 0;
}

void cpx_wifi::set(void* data) {}

sys_error_t cpx_wifi::stop()
{
    return ERROR_SUCCESS;
}

void cpx_wifi::setSsid(std::string& ssid)
{
    _ssid = ssid;
}

void cpx_wifi::setPassword(std::string& password)
{
    _password = password;
}

void cpx_wifi::setWifiMode(void* mode)
{
    _wifiMode = *(wifi_mode_t*)mode;
}
