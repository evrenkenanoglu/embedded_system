/**
 * @file cpx_wifi.cpp
 * @brief Source file for cpx_wifi
 *
 * This file contains definitions for the cpx_wifi class and related data types and functions.
 */

#include "cpx_wifi.h"

#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <iostream>
#include <sstream>

#include "HAL/Platform/ESP32/Library/logImpl.h"
#include "Library/Common/helperConversions.h"

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

// @todo change the HAL type to CPX, it doesn't fit to com device description.
cpx_wifi::cpx_wifi(void* config, LogHandler& logHandler) : _logHandler(logHandler), _wifiMode(WIFI_MODE_NULL) {}

cpx_wifi::~cpx_wifi()
{
    // destructor implementation
}

sys_error_t cpx_wifi::start()
{
    switch (_wifiMode)
    {
        case WIFI_MODE_STA: /**< WiFi station mode */
        case WIFI_MODE_AP:  /**< WiFi soft-AP mode */
        {
            ESP_ERROR_CHECK(wifiInit());
            ESP_ERROR_CHECK(wifiStart());
            std::stringstream ss;
            ss << "WiFi Started!" << std::endl;
            _logHandler.log(ILog::LogLevel::WARNING, ss.str());
            return ERROR_SUCCESS;
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
}

void* cpx_wifi::get()
{
    return &_wifiConfig;
}

void cpx_wifi::set(void* data)
{
    _wifiConfig = *(wifi_config_t*)data;
}

sys_error_t cpx_wifi::stop()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    return ERROR_SUCCESS;
}

void cpx_wifi::setWifiMode(wifi_mode_t mode)
{
    _wifiMode = mode;
}

sys_error_t cpx_wifi::wifiInit()
{
    // Initialize TCP/IP Stack
    ESP_ERROR_CHECK(esp_netif_init());

    //
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    switch (_wifiMode)
    {
        case WIFI_MODE_STA:
        {
            std::stringstream ss;
            ss << "WIFI SOFT STA Initializing...!" << std::endl << "SSID: " << _wifiConfig.sta.ssid << std::endl << "PASSWORD: " << _wifiConfig.sta.password << std::endl;
            _logHandler.log(ILog::LogLevel::WARNING, ss.str());
            esp_netif_create_default_wifi_sta();
        }
        break;

        case WIFI_MODE_AP:
        {
            std::stringstream ss;
            ss << "WIFI SOFT AP Initializing... " << std::endl << "SSID: " << _wifiConfig.ap.ssid << std::endl << "PASSWORD: " << _wifiConfig.ap.password << std::endl;
            _logHandler.log(ILog::LogLevel::WARNING, ss.str());
            esp_netif_create_default_wifi_ap();
        }
        break;

        default:
            _logHandler.log(ILog::LogLevel::ERROR, "WIFI Mode not set yet!");
            return ERROR_INVALID_CONFIG;
            break;
    }

    // Wifi module init wit default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Event Handler Instances Created for WIFI and IP Events
    esp_event_handler_instance_t instance_any_id1;
    esp_event_handler_instance_t instance_any_id2;

    // Register Event Handlers For WIFI and IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id1));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, &instance_any_id2));

    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::wifiStart()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(_wifiMode));
    if (_wifiMode == WIFI_MODE_STA)
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifiConfig));
    }
    else // if (_wifiMode == WIFI_MODE_AP)
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &_wifiConfig));
    }
    ESP_ERROR_CHECK(esp_wifi_start());
    return ERROR_SUCCESS;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{

    switch (event_id)
    {
            /***************************************************************
             *                  HANDLING WIFI STA EVENTS
             **************************************************************/

        case WIFI_EVENT_WIFI_READY: /**< WiFi ready */
        {
            std::cout << "WIFI_EVENT_WIFI_READY: " << std::endl;
        }
        break;

        case WIFI_EVENT_SCAN_DONE: /**< Finished scanning AP */
        {
            std::cout << "WIFI_EVENT_SCAN_DONE: " << std::endl;
        }
        break;

        case WIFI_EVENT_STA_START: /**< Station start */
        {
            std::cout << "WIFI_EVENT_STA_START: " << std::endl;
        }
        break;

        case WIFI_EVENT_STA_STOP: /**< Station stop */
        {
            std::cout << "WIFI_EVENT_STA_STOP: " << std::endl;
        }
        break;

        case WIFI_EVENT_STA_CONNECTED: /**< Station connected to AP */
        {
            std::cout << "WIFI_EVENT_STA_CONNECTED: " << std::endl;
        }
        break;

        case WIFI_EVENT_STA_DISCONNECTED: /**< Station disconnected from AP */
        {
            std::cout << "WIFI_EVENT_STA_DISCONNECTED: " << std::endl;
        }
        break;

        case WIFI_EVENT_STA_AUTHMODE_CHANGE: /**< the auth mode of AP connected by device's station changed */
        {
            std::cout << "WIFI_EVENT_STA_AUTHMODE_CHANGE: " << std::endl;
        }
        break;
            /***************************************************************
             *                  HANDLING WIFI AP EVENTS
             **************************************************************/

        case WIFI_EVENT_AP_START: /**< Soft-AP start */
        {
            std::cout << "WIFI_EVENT_AP_START: " << std::endl;
        }
        break;

        case WIFI_EVENT_AP_STOP: /**< Soft-AP stop */
        {
            std::cout << "WIFI_EVENT_AP_STOP: " << std::endl;
        }
        break;

        case WIFI_EVENT_AP_STACONNECTED: /**< a station connected to Soft-AP */
        {
            std::cout << "WIFI_EVENT_AP_STACONNECTED: " << std::endl;
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;

            std::stringstream ss;
            ss << "Station " << mac::convertToMac(event->mac) << std::uppercase << " join, AID= " << event->aid;
            std::cout << ss.str() << std::endl;
        }
        break;

        case WIFI_EVENT_AP_STADISCONNECTED: /**< a station disconnected from Soft-AP */
        {
            std::cout << "WIFI_EVENT_AP_STADISCONNECTED: " << std::endl;
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;

            std::stringstream ss;
            ss << "Station " << mac::convertToMac(event->mac) << std::uppercase << " leave, AID= " << event->aid;
            std::cout << ss.str() << std::endl;
        }
        break;

        case WIFI_EVENT_AP_PROBEREQRECVED: /**< Receive probe request packet in soft-AP interface */
            std::cout << "WIFI_EVENT_AP_PROBEREQRECVED:" << std::endl;
            break;

        default:
            break;
    }
    // if (event_id == WIFI_EVENT_AP_STACONNECTED)
    // {
    //     wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
    //     ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    // }
    // else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    // {
    //     wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
    //     ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    // }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {}