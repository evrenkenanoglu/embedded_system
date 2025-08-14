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
#include <memory>
#include <sstream>
#include <string>

#include "Library/Common/helperConversions.h"
#include "System/LogHandler.h"

static void        wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void        ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static std::string printAuthMode(int authmode);
namespace
{
constexpr uint16_t wifi_scan_get_result_timeout = WIFI_SCAN_TIMEOUT; // 1.5 seconds

} // namespace

cpx_wifi::cpx_wifi(void* config)
    : _wifiMode(WIFI_MODE_NULL)
{
    _wifiEventGroup  = xEventGroupCreate();
    _wifiInitialized = false;
    _apRecordsResult = nullptr;
    _espNetifAp      = nullptr;
    _espNetifSta     = nullptr;
}

cpx_wifi::~cpx_wifi()
{
    // destructor implementation
}

sys_error_t cpx_wifi::init(void* params)
{
    return wifiInit();
}

sys_error_t cpx_wifi::deInit()
{
    return ERROR_NOT_IMPLEMENTED;
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
            SYS_LOG_I("WiFi Started!");
            return ERROR_SUCCESS;
        }
        break;
        default:
            SYS_LOG_E("WIFI Mode not set yet!");
            return ERROR_INVALID_CONFIG;
            break;
    }
}

sys_error_t cpx_wifi::get(void* data)
{
    if (data == nullptr)
    {
        SYS_LOG_E("Data pointer is null in get()");
        return ERROR_INVALID_ARG;
    }
    data = reinterpret_cast<void*>(&_wifiConfig); // Cast to void pointer for return
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::set(void* data)
{
    _wifiConfig = *(wifi_config_t*)data;

    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::stop()
{
    if (_wifiInitialized)
    {
        std::cout << "DEBUG: Stopping WIFI!" << std::endl;
        ESP_ERROR_CHECK(esp_wifi_stop());

        std::cout << "DEBUG: Event Loop Deleting!" << std::endl;
        esp_event_loop_delete_default();

        if (_espNetifSta != nullptr) // Destroy Default WIFI STA
        {
            std::cout << "DEBUG: Deleting NETIFs STA!" << std::endl;
            esp_netif_destroy_default_wifi(_espNetifSta);
            _espNetifSta = nullptr;
        }

        if (_espNetifAp != nullptr) // Destroy Default WIFI AP
        {
            std::cout << "DEBUG: Deleting NETIFs AP!" << std::endl;
            esp_netif_destroy_default_wifi(_espNetifAp);
            _espNetifAp = nullptr;
        }
        std::cout << "DEBUG: WIFI STOPPED!" << std::endl;
    }
    return ERROR_SUCCESS;
}

void cpx_wifi::setWifiMode(wifi_mode_t mode)
{
    _wifiMode = mode;
}

wifi_mode_t cpx_wifi::getWifiMode()
{
    return _wifiMode;
}

sys_error_t cpx_wifi::wifiInit()
{
    // Initialize TCP/IP Stack
    ESP_ERROR_CHECK(esp_netif_init());
    std::cout << "DEBUG: TCP/IP Stack Initialized!" << std::endl;

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    std::cout << "DEBUG: Event Loop Created!" << std::endl;

    switch (_wifiMode)
    {
        case WIFI_MODE_STA:
        {
            std::stringstream ss;
            ss << "WIFI STA Initializing...!" << std::endl << "SSID: " << _wifiConfig.sta.ssid << std::endl << "PASSWORD: " << _wifiConfig.sta.password << std::endl;
            SYS_LOG_I(ss.str());
            _espNetifSta = esp_netif_create_default_wifi_sta();
            std::cout << "DEBUG: Default WIFI STA Created!" << std::endl;
        }
        break;

        case WIFI_MODE_AP:
        {
            std::stringstream ss;
            ss << "WIFI SOFT AP Initializing... " << std::endl << "SSID: " << _wifiConfig.ap.ssid << std::endl << "PASSWORD: " << _wifiConfig.ap.password << std::endl;
            SYS_LOG_I(ss.str());
            _espNetifAp = esp_netif_create_default_wifi_ap();
            std::cout << "DEBUG: Default AP Created!" << std::endl;
        }
        break;

        case WIFI_MODE_APSTA:
        {
            std::stringstream ss;
            ss << "WIFI SOFT APSTA Initializing... " << std::endl << "SSID: " << _wifiConfig.ap.ssid << std::endl << "PASSWORD: " << _wifiConfig.ap.password << std::endl;
            SYS_LOG_I(ss.str());
            _espNetifAp  = esp_netif_create_default_wifi_ap();
            _espNetifSta = esp_netif_create_default_wifi_sta();

            std::cout << "DEBUG: Default WIFI STA and AP Created!" << std::endl;
        }
        break;

        default:
            SYS_LOG_E("WIFI Mode not set yet!");
            return ERROR_INVALID_CONFIG;
            break;
    }

    // Wifi module init wit default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    std::cout << "DEBUG: WIFI Init Config Created!" << std::endl;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    std::cout << "DEBUG: WIFI Initialized!" << std::endl;

    // Event Handler Instances Created for WIFI and IP Events
    esp_event_handler_instance_t instance_any_id1;
    esp_event_handler_instance_t instance_any_id2;

    // Register Event Handlers For WIFI and IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, &instance_any_id1));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, this, &instance_any_id2));

    _wifiInitialized = true;
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::wifiStart()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(_wifiMode));
    if (_wifiMode == WIFI_MODE_STA)
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifiConfig));
        // Print wifiConfig ssid and password
        // std::cout << _wifiConfig.ssid << std::endl;
        // std::cout << _wifiConfig.password << std::endl;

        ESP_ERROR_CHECK(esp_wifi_start());
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
        SYS_LOG_E("WIFI Mode not set yet!");
        return ERROR_NOT_IMPLEMENTED;
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::connect()
{
    ESP_ERROR_CHECK(esp_wifi_connect());
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::disconnect()
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    return ERROR_SUCCESS;
}

EventGroupHandle_t& cpx_wifi::getWifiEventGroup()
{
    return _wifiEventGroup;
}

sys_error_t cpx_wifi::scan(void* config)
{
    if (_wifiInitialized)
    {
        ESP_ERROR_CHECK(esp_wifi_clear_ap_list());
        ESP_ERROR_CHECK(esp_wifi_scan_start(static_cast<wifi_scan_config_t*>(config), true));
        return ERROR_SUCCESS;
    }
    else
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }
}

sys_error_t cpx_wifi::getScanResults(QueueHandle_t apRecordsResult)
{
    std::unique_ptr<wifi_ap_record_t[]> ap_info(new wifi_ap_record_t[WIFI_SCAN_MAX_RECORDS]);
    memset(ap_info.get(), 0, sizeof(wifi_ap_record_t) * WIFI_SCAN_MAX_RECORDS);

    uint16_t scanCountResult = WIFI_SCAN_MAX_RECORDS; //
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&scanCountResult, ap_info.get()));

    std::stringstream ss;
    ss << "Total APs scanned = " << static_cast<int>(scanCountResult) << std::endl;
    SYS_LOG_I(ss.str());

    for (int i = 0; (i < WIFI_SCAN_MAX_RECORDS) && (i < scanCountResult); i++)
    {
        wifiApRecord_t apRecord;
        memcpy(&apRecord.ssid, ap_info[i].ssid, sizeof(ap_info[i].ssid));
        apRecord.authmode = ap_info[i].authmode;

        xQueueSendToBack(apRecordsResult, &apRecord, 0);

        // Info message for each AP
        ss << "SSID: " << ap_info[i].ssid << std::endl << "Auth Mode: " << printAuthMode(ap_info[i].authmode) << std::endl;
        std::cout << ss.str() << std::endl;
    }

    return ERROR_SUCCESS;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const EventGroupHandle_t& wifiEventGroup = ((cpx_wifi*)arg)->getWifiEventGroup();

    switch (event_id)
    {
            /***************************************************************
             *                  HANDLING WIFI STA EVENTS
             **************************************************************/

        case WIFI_EVENT_WIFI_READY: /**< WiFi ready */
        {
            SYS_LOG_I("WIFI_EVENT_WIFI_READY: ");
        }
        break;

        case WIFI_EVENT_SCAN_DONE: /**< Finished scanning AP */
        {
            SYS_LOG_I("WIFI_EVENT_SCAN_DONE: ");
            xEventGroupSetBits(wifiEventGroup, WIFI_SCAN_DONE);
        }
        break;

        case WIFI_EVENT_STA_START: /**< Station start */
        {
            SYS_LOG_I("WIFI_EVENT_STA_START: ");
            xEventGroupSetBits(wifiEventGroup, WIFI_STA_STARTED);
        }
        break;

        case WIFI_EVENT_STA_STOP: /**< Station stop */
        {
            SYS_LOG_I("WIFI_EVENT_STA_STOP: ");
        }
        break;

        case WIFI_EVENT_STA_CONNECTED: /**< Station connected to AP */
        {
            SYS_LOG_I("WIFI_EVENT_STA_CONNECTED: ");
            xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED);
        }
        break;

        case WIFI_EVENT_STA_DISCONNECTED: /**< Station disconnected from AP */
        {
            SYS_LOG_I("WIFI_EVENT_STA_DISCONNECTED: ");
            xEventGroupSetBits(wifiEventGroup, WIFI_DISCONNECTED);
        }
        break;

        case WIFI_EVENT_STA_AUTHMODE_CHANGE: /**< the auth mode of AP connected by device's station changed */
        {
            SYS_LOG_I("WIFI_EVENT_STA_AUTHMODE_CHANGE: ");
        }
        break;
            /***************************************************************
             *                  HANDLING WIFI AP EVENTS
             **************************************************************/

        case WIFI_EVENT_AP_START: /**< Soft-AP start */
        {
            SYS_LOG_I("WIFI_EVENT_AP_START: ");
        }
        break;

        case WIFI_EVENT_AP_STOP: /**< Soft-AP stop */
        {
            SYS_LOG_I("WIFI_EVENT_AP_STOP: ");
        }
        break;

        case WIFI_EVENT_AP_STACONNECTED: /**< a station connected to Soft-AP */
        {
            SYS_LOG_I("WIFI_EVENT_AP_STACONNECTED: ");
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;

            std::stringstream ss;
            ss << "Station " << mac::convertToMac(event->mac) << std::uppercase << " join, AID= " << event->aid;
            SYS_LOG_I(ss.str());
        }
        break;

        case WIFI_EVENT_AP_STADISCONNECTED: /**< a station disconnected from Soft-AP */
        {
            SYS_LOG_I("WIFI_EVENT_AP_STADISCONNECTED: ");
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;

            std::stringstream ss;
            ss << "Station " << mac::convertToMac(event->mac) << std::uppercase << " leave, AID= " << event->aid;
            SYS_LOG_I(ss.str());
        }
        break;

        case WIFI_EVENT_AP_PROBEREQRECVED: /**< Receive probe request packet in soft-AP interface */
            SYS_LOG_I("WIFI_EVENT_AP_PROBEREQRECVED:");
            break;

        default:
            break;
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    EventGroupHandle_t& wifiEventGroup = ((cpx_wifi*)arg)->getWifiEventGroup();

    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP: /*!< station got IP from connected AP */
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;

            SYS_LOG_I("IP_EVENT_STA_GOT_IP: ");

            char ipString[24] = "IP: ";
            sprintf(&ipString[4], IPSTR, IP2STR(&event->ip_info.ip));
            SYS_LOG_I(std::string(ipString));

            // Set Event Bit for Station Connected
            xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED);
            xEventGroupClearBits(wifiEventGroup, WIFI_DISCONNECTED);
        }
        break;

        case IP_EVENT_STA_LOST_IP: /*!< station lost IP and the IP is reset to 0 */
        {
            SYS_LOG_I("IP_EVENT_STA_LOST_IP: ");
            xEventGroupSetBits(wifiEventGroup, WIFI_DISCONNECTED);
            xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED);
        }
        break;

        case IP_EVENT_AP_STAIPASSIGNED: /*!< soft-AP assign an IP to a connected station */
        {
            SYS_LOG_I("IP_EVENT_AP_STAIPASSIGNED: ");
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
