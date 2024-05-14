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
#include <string>

#include "HAL/Platform/ESP32/Library/logImpl.h"
#include "Library/Common/helperConversions.h"

static void        wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void        ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static std::string printAuthMode(int authmode);

cpx_wifi::cpx_wifi(void* config) : _wifiMode(WIFI_MODE_NULL) {}

cpx_wifi::~cpx_wifi()
{
    // destructor implementation
}

sys_error_t cpx_wifi::start()
{
    switch (_wifiMode)
    {
        case WIFI_MODE_STA:   /**< WiFi station mode */
        case WIFI_MODE_AP:    /**< WiFi soft-AP mode */
        case WIFI_MODE_APSTA: /**< WiFi station + soft-AP mode */
        {
            ESP_ERROR_CHECK(wifiInit());
            ESP_ERROR_CHECK(wifiStart());
            std::stringstream ss;
            ss << "WiFi Started!" << std::endl;
            logger().log(ILog::LogLevel::WARNING, ss.str());
            return ERROR_SUCCESS;
        }
        break;

            {
                ESP_ERROR_CHECK(wifiInit());
                ESP_ERROR_CHECK(wifiStart());
                std::stringstream ss;
                ss << "WiFi Started!" << std::endl;
                logger().log(ILog::LogLevel::WARNING, ss.str());
                return ERROR_SUCCESS;
            }
            break;

        default:
            logger().log(ILog::LogLevel::ERROR, "WIFI Mode not set yet!");
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
            logger().log(ILog::LogLevel::INFO, ss.str());
            esp_netif_create_default_wifi_sta();
        }
        break;

        case WIFI_MODE_AP:
        {
            std::stringstream ss;
            ss << "WIFI SOFT AP Initializing... " << std::endl << "SSID: " << _wifiConfig.ap.ssid << std::endl << "PASSWORD: " << _wifiConfig.ap.password << std::endl;
            logger().log(ILog::LogLevel::INFO, ss.str());
            esp_netif_create_default_wifi_ap();
        }
        break;

        case WIFI_MODE_APSTA:
        {
            std::stringstream ss;
            ss << "WIFI SOFT APSTA Initializing... " << std::endl << "SSID: " << _wifiConfig.ap.ssid << std::endl << "PASSWORD: " << _wifiConfig.ap.password << std::endl;
            logger().log(ILog::LogLevel::INFO, ss.str());
            esp_netif_create_default_wifi_ap();
            esp_netif_create_default_wifi_sta();
        }
        break;

        default:
            logger().log(ILog::LogLevel::ERROR, "WIFI Mode not set yet!");
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
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if (_wifiMode == WIFI_MODE_AP)
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &_wifiConfig));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    else if (_wifiMode == WIFI_MODE_APSTA)
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &_wifiConfig));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifiConfig));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    else
    {
        logger().log(ILog::LogLevel::ERROR, "WIFI Mode not set yet!");
        return ERROR_NOT_IMPLEMENTED;
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::scan(void* config, void* result, uint16_t scanCount)
{
    ESP_ERROR_CHECK(esp_wifi_scan_start(static_cast<wifi_scan_config_t*>(config), true));

    wifi_ap_record_t* ap_info  = static_cast<wifi_ap_record_t*>(result);
    uint16_t          ap_count = 0;
    memset(ap_info, 0, sizeof(wifi_ap_record_t) * scanCount);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&scanCount, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    std::stringstream ss;
    ss << "Total APs scanned = " << ap_count << std::endl;
    logger().log(ILog::LogLevel::INFO, ss.str());

    for (int i = 0; (i < scanCount) && (i < ap_count); i++)
    {
        std::stringstream ss;
        ss << "SSID: " << ap_info[i].ssid << std::endl
           << "RSSI: " << static_cast<int>(ap_info[i].rssi) << std::endl
           << "Auth Mode: " << printAuthMode(ap_info[i].authmode) << std::endl
           << "Channel: " << static_cast<int>(ap_info[i].primary) << std::endl;
        std::cout << ss.str() << std::endl;
    }

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
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_WIFI_READY: ");
        }
        break;

        case WIFI_EVENT_SCAN_DONE: /**< Finished scanning AP */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_SCAN_DONE: ");
        }
        break;

        case WIFI_EVENT_STA_START: /**< Station start */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_STA_START: ");
        }
        break;

        case WIFI_EVENT_STA_STOP: /**< Station stop */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_STA_STOP: ");
        }
        break;

        case WIFI_EVENT_STA_CONNECTED: /**< Station connected to AP */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_STA_CONNECTED: ");
        }
        break;

        case WIFI_EVENT_STA_DISCONNECTED: /**< Station disconnected from AP */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_STA_DISCONNECTED: ");
        }
        break;

        case WIFI_EVENT_STA_AUTHMODE_CHANGE: /**< the auth mode of AP connected by device's station changed */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_STA_AUTHMODE_CHANGE: ");
        }
        break;
            /***************************************************************
             *                  HANDLING WIFI AP EVENTS
             **************************************************************/

        case WIFI_EVENT_AP_START: /**< Soft-AP start */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_AP_START: ");
        }
        break;

        case WIFI_EVENT_AP_STOP: /**< Soft-AP stop */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_AP_STOP: ");
        }
        break;

        case WIFI_EVENT_AP_STACONNECTED: /**< a station connected to Soft-AP */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_AP_STACONNECTED: ");
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;

            std::stringstream ss;
            ss << "Station " << mac::convertToMac(event->mac) << std::uppercase << " join, AID= " << event->aid;
            logger().log(ILog::LogLevel::INFO, ss.str());
        }
        break;

        case WIFI_EVENT_AP_STADISCONNECTED: /**< a station disconnected from Soft-AP */
        {
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_AP_STADISCONNECTED: ");
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;

            std::stringstream ss;
            ss << "Station " << mac::convertToMac(event->mac) << std::uppercase << " leave, AID= " << event->aid;
            logger().log(ILog::LogLevel::INFO, ss.str());
        }
        break;

        case WIFI_EVENT_AP_PROBEREQRECVED: /**< Receive probe request packet in soft-AP interface */
            logger().log(ILog::LogLevel::INFO, "WIFI_EVENT_AP_PROBEREQRECVED:");
            break;

        default:
            break;
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{

    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP: /*!< station got IP from connected AP */
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;

            logger().log(ILog::LogLevel::INFO, "IP_EVENT_STA_GOT_IP: ");

            char ipString[20];
            sprintf(ipString, IPSTR, IP2STR(&event->ip_info.ip));
            std::stringstream ss;
            ss << "IP: " << std::string(ipString);
            logger().log(ILog::LogLevel::INFO, ss.str());
        }
        break;

        case IP_EVENT_STA_LOST_IP: /*!< station lost IP and the IP is reset to 0 */
        {
            logger().log(ILog::LogLevel::INFO, "IP_EVENT_STA_LOST_IP: ");
        }
        break;

        case IP_EVENT_AP_STAIPASSIGNED: /*!< soft-AP assign an IP to a connected station */
        {
            logger().log(ILog::LogLevel::INFO, "IP_EVENT_AP_STAIPASSIGNED: ");
        }
        break;

        default:
            break;
    }
}

static std::string printAuthMode(int authmode)
{
    switch (authmode)
    {
        case WIFI_AUTH_OPEN:
            return "WIFI_AUTH_OPEN";
        case WIFI_AUTH_WEP:
            return "WIFI_AUTH_WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WIFI_AUTH_WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WIFI_AUTH_WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WIFI_AUTH_WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WIFI_AUTH_WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK:
            return "WIFI_AUTH_WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WIFI_AUTH_WPA2_WPA3_PSK";
    }
    return "";
}