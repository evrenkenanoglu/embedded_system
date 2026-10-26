/** @file       cpx_wifi.cpp
 *  @brief      ESP32 implementation of the platform-independent WiFi HAL interface.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#include "cpx_wifi.h"

#include "Library/Common/helperConversions.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>

/** CONSTANTS *****************************************************************/

namespace
{
    constexpr uint16_t wifiScanMaxRecords = WIFI_SCAN_MAX_RECORDS;
} // namespace

/** ESP <-> HAL TRANSLATION HELPERS *******************************************/

hal_wifi_mode_t cpx_wifi::_espModeToHal(wifi_mode_t mode)
{
    switch (mode)
    {
        case WIFI_MODE_STA:
            return WIFI_MODE_STATION;
        case WIFI_MODE_AP:
            return WIFI_MODE_ACCESS_POINT;
        case WIFI_MODE_APSTA:
            return WIFI_MODE_STATION_AP;
        case WIFI_MODE_NULL:
        default:
            return WIFI_MODE_DISABLED;
    }
}

wifi_mode_t cpx_wifi::_halModeToEsp(hal_wifi_mode_t mode)
{
    switch (mode)
    {
        case WIFI_MODE_STATION:
            return WIFI_MODE_STA;
        case WIFI_MODE_ACCESS_POINT:
            return WIFI_MODE_AP;
        case WIFI_MODE_STATION_AP:
            return WIFI_MODE_APSTA;
        case WIFI_MODE_DISABLED:
        default:
            return WIFI_MODE_NULL;
    }
}

hal_wifi_auth_mode_t cpx_wifi::_espAuthToHal(wifi_auth_mode_t auth)
{
    switch (auth)
    {
        case WIFI_AUTH_OPEN:
            return HAL_WIFI_AUTH_OPEN;
        case WIFI_AUTH_WEP:
            return HAL_WIFI_AUTH_WEP;
        case WIFI_AUTH_WPA_PSK:
            return HAL_WIFI_AUTH_WPA_PSK;
        case WIFI_AUTH_WPA2_PSK:
            return HAL_WIFI_AUTH_WPA2_PSK;
        case WIFI_AUTH_WPA_WPA2_PSK:
            return HAL_WIFI_AUTH_WPA_WPA2_PSK;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return HAL_WIFI_AUTH_WPA2_ENTERPRISE;
        case WIFI_AUTH_WPA3_PSK:
            return HAL_WIFI_AUTH_WPA3_PSK;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return HAL_WIFI_AUTH_WPA2_WPA3_PSK;
        case WIFI_AUTH_WAPI_PSK:
            return HAL_WIFI_AUTH_WAPI_PSK;
        case WIFI_AUTH_OWE:
            return HAL_WIFI_AUTH_OWE;
        case WIFI_AUTH_WPA3_ENT_192:
            return HAL_WIFI_AUTH_WPA3_ENT_192;
        default:
            return HAL_WIFI_AUTH_OPEN;
    }
}

wifi_auth_mode_t cpx_wifi::_halAuthToEsp(hal_wifi_auth_mode_t auth)
{
    switch (auth)
    {
        case HAL_WIFI_AUTH_OPEN:
            return WIFI_AUTH_OPEN;
        case HAL_WIFI_AUTH_WEP:
            return WIFI_AUTH_WEP;
        case HAL_WIFI_AUTH_WPA_PSK:
            return WIFI_AUTH_WPA_PSK;
        case HAL_WIFI_AUTH_WPA2_PSK:
            return WIFI_AUTH_WPA2_PSK;
        case HAL_WIFI_AUTH_WPA_WPA2_PSK:
            return WIFI_AUTH_WPA_WPA2_PSK;
        case HAL_WIFI_AUTH_WPA2_ENTERPRISE:
            return WIFI_AUTH_WPA2_ENTERPRISE;
        case HAL_WIFI_AUTH_WPA3_PSK:
            return WIFI_AUTH_WPA3_PSK;
        case HAL_WIFI_AUTH_WPA2_WPA3_PSK:
            return WIFI_AUTH_WPA2_WPA3_PSK;
        case HAL_WIFI_AUTH_WAPI_PSK:
            return WIFI_AUTH_WAPI_PSK;
        case HAL_WIFI_AUTH_OWE:
            return WIFI_AUTH_OWE;
        case HAL_WIFI_AUTH_WPA3_ENT_192:
            return WIFI_AUTH_WPA3_ENT_192;
        default:
            return WIFI_AUTH_OPEN;
    }
}

void cpx_wifi::_setState(hal_wifi_state_t state)
{
    _wifiState = state;
}

void cpx_wifi::_dispatchEvent(const hal_wifi_event_t& event)
{
    if (_eventCallback != nullptr)
    {
        _eventCallback(&event, _eventCallbackUserData);
    }
}

/** LIFECYCLE *****************************************************************/

cpx_wifi::cpx_wifi(void* config)
    : _wifiMode(WIFI_MODE_NULL)
    , _wifiConfigSta{}
    , _wifiConfigAp{}
    , _wifiEventGroup(nullptr)
    , _wifiInitialized(false)
    , _apRecordsResult(nullptr)
    , _espNetifAp(nullptr)
    , _espNetifSta(nullptr)
    , _wifiState(WIFI_STATE_DISCONNECTED)
    , _eventCallback(nullptr)
    , _eventCallbackUserData(nullptr)
{
    _wifiEventGroup = xEventGroupCreate();

    if (config != nullptr)
    {
        set(config);
    }
}

cpx_wifi::~cpx_wifi()
{
    stop();
    deInit();

    if (_wifiEventGroup != nullptr)
    {
        vEventGroupDelete(_wifiEventGroup);
        _wifiEventGroup = nullptr;
    }
}

sys_error_t cpx_wifi::init(void* params)
{
    (void)params;
    return _wifiInit();
}

sys_error_t cpx_wifi::deInit()
{
    if (_wifiInitialized)
    {
        stop();
        esp_wifi_deinit();
        _wifiInitialized = false;
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::start()
{
    if (_wifiMode == WIFI_MODE_NULL)
    {
        SYS_LOG_E("WIFI Mode not set yet!");
        return ERROR_INVALID_CONFIG;
    }

    /// Bring up the driver, then start it.
    RETURN_ON_ERROR(_wifiInit()); // Expression

    sys_error_t error = _wifiStart();
    if (error == ERROR_SUCCESS)
    {
        SYS_LOG_I("WiFi Started!");
    }
    return error;
}

sys_error_t cpx_wifi::stop()
{
    if (_wifiInitialized)
    {
        SYS_LOG_D("Stopping WIFI!");
        esp_wifi_stop();
        esp_event_loop_delete_default();

        if (_espNetifSta != nullptr)
        {
            SYS_LOG_D("Deleting NETIFs STA!");
            esp_netif_destroy_default_wifi(_espNetifSta);
            _espNetifSta = nullptr;
        }

        if (_espNetifAp != nullptr)
        {
            SYS_LOG_D("Deleting NETIFs AP!");
            esp_netif_destroy_default_wifi(_espNetifAp);
            _espNetifAp = nullptr;
        }

        _wifiInitialized = false;
        _setState(WIFI_STATE_DISCONNECTED);
        SYS_LOG_D("WIFI STOPPED!");
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::get(void* data)
{
    if (data == nullptr)
    {
        SYS_LOG_E("Data pointer is null in get()");
        return ERROR_INVALID_ARG;
    }

    // Legacy API: return the active ESP configuration
    wifi_config_t* outConfig = static_cast<wifi_config_t*>(data);
    if (_wifiMode == WIFI_MODE_AP)
    {
        *outConfig = _wifiConfigAp;
    }
    else
    {
        *outConfig = _wifiConfigSta;
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::set(void* data)
{
    if (data == nullptr)
    {
        SYS_LOG_E("Data pointer is null in set()");
        return ERROR_INVALID_ARG;
    }

    // Legacy API: accept an ESP wifi_config_t and route it to the STA config
    const wifi_config_t* inConfig = static_cast<const wifi_config_t*>(data);
    _wifiConfigSta                = *inConfig;
    return ERROR_SUCCESS;
}

/** IHAL_CPX_WIFI INTERFACE ***************************************************/

sys_error_t cpx_wifi::setMode(hal_wifi_mode_t mode)
{
    _wifiMode = _halModeToEsp(mode);
    if (mode != WIFI_MODE_DISABLED && _wifiMode == WIFI_MODE_NULL)
    {
        SYS_LOG_E("Invalid WiFi mode requested");
        return ERROR_INVALID_ARG;
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::getMode(hal_wifi_mode_t& mode)
{
    mode = _espModeToHal(_wifiMode);
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::setConfig(hal_wifi_mode_t interface_type, const hal_wifi_config_t* config)
{
    if (config == nullptr)
    {
        SYS_LOG_E("Config pointer is null in setConfig()");
        return ERROR_INVALID_ARG;
    }

    switch (interface_type)
    {
        case WIFI_MODE_STATION:
        {
            const hal_wifi_sta_config_t* sta = &config->sta;
            memset(&_wifiConfigSta, 0, sizeof(_wifiConfigSta));
            strncpy(reinterpret_cast<char*>(_wifiConfigSta.sta.ssid), sta->ssid, sizeof(_wifiConfigSta.sta.ssid) - 1);
            strncpy(reinterpret_cast<char*>(_wifiConfigSta.sta.password), sta->password, sizeof(_wifiConfigSta.sta.password) - 1);
            _wifiConfigSta.sta.channel            = sta->channel;
            _wifiConfigSta.sta.listen_interval    = sta->listen_interval;
            _wifiConfigSta.sta.threshold.authmode = _halAuthToEsp(sta->threshold_authmode);
            _wifiConfigSta.sta.pmf_cfg.capable    = sta->pmf_capable;
            _wifiConfigSta.sta.pmf_cfg.required   = sta->pmf_required;
            _wifiConfigSta.sta.bssid_set          = sta->bssid_set;
            if (sta->bssid_set)
            {
                memcpy(_wifiConfigSta.sta.bssid, sta->bssid, WIFI_MAC_ADDR_LEN);
            }
            return ERROR_SUCCESS;
        }

        case WIFI_MODE_ACCESS_POINT:
        {
            const hal_wifi_ap_config_t* ap = &config->ap;
            memset(&_wifiConfigAp, 0, sizeof(_wifiConfigAp));
            strncpy(reinterpret_cast<char*>(_wifiConfigAp.ap.ssid), ap->ssid, sizeof(_wifiConfigAp.ap.ssid) - 1);
            _wifiConfigAp.ap.ssid_len = ap->ssid_len;
            strncpy(reinterpret_cast<char*>(_wifiConfigAp.ap.password), ap->password, sizeof(_wifiConfigAp.ap.password) - 1);
            _wifiConfigAp.ap.channel          = ap->channel;
            _wifiConfigAp.ap.authmode         = _halAuthToEsp(ap->authmode);
            _wifiConfigAp.ap.ssid_hidden      = ap->ssid_hidden ? 1 : 0;
            _wifiConfigAp.ap.max_connection   = ap->max_connection;
            _wifiConfigAp.ap.beacon_interval  = ap->beacon_interval;
            _wifiConfigAp.ap.pmf_cfg.capable  = ap->pmf_capable;
            _wifiConfigAp.ap.pmf_cfg.required = ap->pmf_required;
            return ERROR_SUCCESS;
        }

        default:
            SYS_LOG_E("setConfig: invalid interface type");
            return ERROR_INVALID_ARG;
    }
}

sys_error_t cpx_wifi::getConfig(hal_wifi_mode_t interface_type, hal_wifi_config_t* config)
{
    if (config == nullptr)
    {
        SYS_LOG_E("Config pointer is null in getConfig()");
        return ERROR_INVALID_ARG;
    }

    switch (interface_type)
    {
        case WIFI_MODE_STATION:
        {
            memset(config, 0, sizeof(hal_wifi_config_t));
            hal_wifi_sta_config_t* sta = &config->sta;
            strncpy(sta->ssid, reinterpret_cast<const char*>(_wifiConfigSta.sta.ssid), WIFI_SSID_MAX_LEN);
            strncpy(sta->password, reinterpret_cast<const char*>(_wifiConfigSta.sta.password), WIFI_PASSWORD_MAX_LEN);
            sta->channel            = _wifiConfigSta.sta.channel;
            sta->listen_interval    = _wifiConfigSta.sta.listen_interval;
            sta->threshold_authmode = _espAuthToHal(_wifiConfigSta.sta.threshold.authmode);
            sta->pmf_capable        = _wifiConfigSta.sta.pmf_cfg.capable;
            sta->pmf_required       = _wifiConfigSta.sta.pmf_cfg.required;
            sta->bssid_set          = _wifiConfigSta.sta.bssid_set;
            if (sta->bssid_set)
            {
                memcpy(sta->bssid, _wifiConfigSta.sta.bssid, WIFI_MAC_ADDR_LEN);
            }
            return ERROR_SUCCESS;
        }

        case WIFI_MODE_ACCESS_POINT:
        {
            memset(config, 0, sizeof(hal_wifi_config_t));
            hal_wifi_ap_config_t* ap = &config->ap;
            strncpy(ap->ssid, reinterpret_cast<const char*>(_wifiConfigAp.ap.ssid), WIFI_SSID_MAX_LEN);
            ap->ssid_len = _wifiConfigAp.ap.ssid_len;
            strncpy(ap->password, reinterpret_cast<const char*>(_wifiConfigAp.ap.password), WIFI_PASSWORD_MAX_LEN);
            ap->channel         = _wifiConfigAp.ap.channel;
            ap->authmode        = _espAuthToHal(_wifiConfigAp.ap.authmode);
            ap->ssid_hidden     = _wifiConfigAp.ap.ssid_hidden != 0;
            ap->max_connection  = _wifiConfigAp.ap.max_connection;
            ap->beacon_interval = _wifiConfigAp.ap.beacon_interval;
            ap->pmf_capable     = _wifiConfigAp.ap.pmf_cfg.capable;
            ap->pmf_required    = _wifiConfigAp.ap.pmf_cfg.required;
            return ERROR_SUCCESS;
        }

        default:
            SYS_LOG_E("getConfig: invalid interface type");
            return ERROR_INVALID_ARG;
    }
}

sys_error_t cpx_wifi::connect()
{
    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }

    /// Transition to CONNECTING before issuing the driver request.
    _setState(WIFI_STATE_CONNECTING);

    esp_err_t err = esp_wifi_connect();
    RETURN_IF_ERROR(
        (err != ESP_OK),                               // Expression
        TRANSLATE_ERROR(err),                          // Error code
        SYS_LOG_E("esp_wifi_connect failed: %d", err), // Error message
        _setState(WIFI_STATE_CONNECTION_FAILED)        // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::disconnect()
{
    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }

    esp_err_t err = esp_wifi_disconnect();
    RETURN_IF_ERROR(
        (err != ESP_OK),                                  // Expression
        TRANSLATE_ERROR(err),                             // Error code
        SYS_LOG_E("esp_wifi_disconnect failed: %d", err), // Error message
        _setState(WIFI_STATE_DISCONNECTED)                // Cleanup
    );
    _setState(WIFI_STATE_DISCONNECTED);
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::startScan(const hal_wifi_scan_config_t* config, bool block)
{
    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }

    wifi_scan_config_t scanConfig = {};
    if (config != nullptr)
    {
        scanConfig.show_hidden = config->show_hidden;
        scanConfig.channel     = config->channel;
        if (config->ssid != nullptr)
        {
            scanConfig.ssid = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(config->ssid));
        }
        if (config->bssid != nullptr)
        {
            scanConfig.bssid = const_cast<uint8_t*>(config->bssid);
        }
        if (config->scan_time_min_ms != 0)
        {
            scanConfig.scan_time.active.min = config->scan_time_min_ms;
        }
        if (config->scan_time_max_ms != 0)
        {
            scanConfig.scan_time.active.max = config->scan_time_max_ms;
        }
    }

    RETURN_IF_ERROR(
        (esp_wifi_clear_ap_list() != ESP_OK), // Expression
        ERROR_FAIL,                           // Error code
        SYS_LOG_E("Failed to clear AP list")  // Error message
    );

    /// Transition to SCANNING; on failure, restore the previous idle state.
    _setState(WIFI_STATE_SCANNING);
    esp_err_t err = esp_wifi_scan_start(&scanConfig, block);
    RETURN_IF_ERROR(
        (err != ESP_OK),                                  // Expression
        TRANSLATE_ERROR(err),                             // Error code
        SYS_LOG_E("esp_wifi_scan_start failed: %d", err), // Error message
        _setState(WIFI_STATE_DISCONNECTED)                // Cleanup
    );

    if (block)
    {
        _setState(WIFI_STATE_SCAN_DONE);
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::getScanResults(hal_wifi_ap_record_t* records, uint16_t max_records, uint16_t& actual_records)
{
    if (records == nullptr || max_records == 0)
    {
        return ERROR_INVALID_ARG;
    }

    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }

    std::unique_ptr<wifi_ap_record_t[]> espRecords(new (std::nothrow) wifi_ap_record_t[max_records]);
    RETURN_IF_ERROR(
        (!espRecords),                                             // Expression
        ERROR_OUT_OF_MEMORY,                                       // Error code
        SYS_LOG_E("Out of memory - Scan record allocation failed") // Error message
    );
    memset(espRecords.get(), 0, sizeof(wifi_ap_record_t) * max_records);

    uint16_t  scanCount = max_records;
    esp_err_t err       = esp_wifi_scan_get_ap_records(&scanCount, espRecords.get());
    RETURN_IF_ERROR(
        (err != ESP_OK),                                          // Expression
        TRANSLATE_ERROR(err),                                     // Error code
        SYS_LOG_E("esp_wifi_scan_get_ap_records failed: %d", err) // Error message
    );

    for (uint16_t i = 0; i < scanCount; i++)
    {
        hal_wifi_ap_record_t& record = records[i];
        memset(&record, 0, sizeof(record));

        memcpy(record.bssid, espRecords[i].bssid, WIFI_MAC_ADDR_LEN);
        strncpy(record.ssid, reinterpret_cast<const char*>(espRecords[i].ssid), WIFI_SSID_MAX_LEN);
        record.primary_channel = espRecords[i].primary;
        record.second_channel  = espRecords[i].second;
        record.rssi            = espRecords[i].rssi;
        record.authmode        = _espAuthToHal(espRecords[i].authmode);
        record.phy_11b         = espRecords[i].phy_11b != 0;
        record.phy_11g         = espRecords[i].phy_11g != 0;
        record.phy_11n         = espRecords[i].phy_11n != 0;
        record.phy_11ax        = espRecords[i].phy_11ax != 0;
        record.wps             = espRecords[i].wps != 0;
        memcpy(record.country, espRecords[i].country.cc, WIFI_COUNTRY_CODE_LEN);
    }

    actual_records = scanCount;

    SYS_LOG_I("Total APs scanned = %u", scanCount);
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::clearScanResults()
{
    if (!_wifiInitialized)
    {
        return ERROR_FAIL;
    }
    return TRANSLATE_ERROR(esp_wifi_clear_ap_list());
}

sys_error_t cpx_wifi::getState(hal_wifi_state_t& state)
{
    state = _wifiState;
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::getNetworkInfo(hal_wifi_network_info_t& info)
{
    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }

    memset(&info, 0, sizeof(info));

    esp_netif_t* netif = (_wifiMode == WIFI_MODE_AP) ? _espNetifAp : _espNetifSta;
    if (netif == nullptr)
    {
        return ERROR_FAIL;
    }

    esp_netif_ip_info_t ipInfo = {};
    RETURN_IF_ERROR(
        (esp_netif_get_ip_info(netif, &ipInfo) != ESP_OK), // Expression
        ERROR_FAIL,                                        // Error code
        SYS_LOG_E("esp_netif_get_ip_info failed")          // Error message
    );

    snprintf(info.ip.ip_address, WIFI_IP_ADDR_STR_LEN, IPSTR, IP2STR(&ipInfo.ip));
    snprintf(info.ip.netmask, WIFI_IP_ADDR_STR_LEN, IPSTR, IP2STR(&ipInfo.netmask));
    snprintf(info.ip.gateway, WIFI_IP_ADDR_STR_LEN, IPSTR, IP2STR(&ipInfo.gw));

    strncpy(info.ssid, reinterpret_cast<const char*>(_wifiConfigSta.sta.ssid), WIFI_SSID_MAX_LEN);
    info.state = _wifiState;

    int rssi = 0;
    if (esp_wifi_sta_get_rssi(&rssi) == ESP_OK)
    {
        info.rssi = static_cast<int8_t>(rssi);
    }

    uint8_t            primaryChannel = 0;
    wifi_second_chan_t secondChannel  = WIFI_SECOND_CHAN_NONE;
    if (esp_wifi_get_channel(&primaryChannel, &secondChannel) == ESP_OK)
    {
        info.channel = primaryChannel;
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::setEventCallback(hal_wifi_event_cb_t callback, void* user_data)
{
    _eventCallback         = callback;
    _eventCallbackUserData = user_data;
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::setPowerSave(bool enable)
{
    if (!_wifiInitialized)
    {
        return ERROR_FAIL;
    }
    return TRANSLATE_ERROR(esp_wifi_set_ps(enable ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE));
}

sys_error_t cpx_wifi::getRSSI(int8_t& rssi)
{
    if (!_wifiInitialized)
    {
        return ERROR_FAIL;
    }
    int       rssiValue = 0;
    esp_err_t err       = esp_wifi_sta_get_rssi(&rssiValue);
    RETURN_IF_ERROR(
        (err != ESP_OK),                                   // Expression
        TRANSLATE_ERROR(err),                              // Error code
        SYS_LOG_E("esp_wifi_sta_get_rssi failed: %d", err) // Error message
    );

    rssi = static_cast<int8_t>(rssiValue);
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::getMacAddress(hal_wifi_mode_t interface_type, uint8_t* mac)
{
    if (mac == nullptr)
    {
        return ERROR_INVALID_ARG;
    }
    wifi_interface_t iface = (interface_type == WIFI_MODE_ACCESS_POINT) ? WIFI_IF_AP : WIFI_IF_STA;
    return TRANSLATE_ERROR(esp_wifi_get_mac(iface, mac));
}

sys_error_t cpx_wifi::setMacAddress(hal_wifi_mode_t interface_type, const uint8_t* mac)
{
    if (mac == nullptr)
    {
        return ERROR_INVALID_ARG;
    }
    wifi_interface_t iface = (interface_type == WIFI_MODE_ACCESS_POINT) ? WIFI_IF_AP : WIFI_IF_STA;
    return TRANSLATE_ERROR(esp_wifi_set_mac(iface, const_cast<uint8_t*>(mac)));
}

sys_error_t cpx_wifi::getCapabilities(uint32_t& capabilities)
{
    capabilities = WIFI_CAP_STA | WIFI_CAP_AP | WIFI_CAP_CONCURRENT | WIFI_CAP_PMF | WIFI_CAP_WPA3 | WIFI_CAP_POWER_SAVE | WIFI_CAP_HIDDEN_SSID | WIFI_CAP_CHANNEL_SWITCH;
    return ERROR_SUCCESS;
}

bool cpx_wifi::hasCapability(hal_wifi_capabilities_t capability)
{
    uint32_t caps = 0;
    getCapabilities(caps);
    return (caps & static_cast<uint32_t>(capability)) != 0;
}

bool cpx_wifi::isConnected()
{
    return _wifiState == WIFI_STATE_CONNECTED;
}

bool cpx_wifi::isInitialized()
{
    return _wifiInitialized;
}

/** INTERNAL DRIVER STARTUP ***************************************************/

sys_error_t cpx_wifi::_wifiInit()
{
    if (_wifiInitialized)
    {
        return ERROR_SUCCESS;
    }

    /// Initialize TCP/IP Stack.
    RETURN_IF_ERROR(
        (esp_netif_init() != ESP_OK),      // Expression
        ERROR_INIT_FAILED,                 // Error code
        SYS_LOG_E("esp_netif_init failed") // Error message
    );
    SYS_LOG_D("TCP / IP Stack Initialized!");

    RETURN_IF_ERROR(
        (esp_event_loop_create_default() != ESP_OK),      // Expression
        ERROR_INIT_FAILED,                                // Error code
        SYS_LOG_E("esp_event_loop_create_default failed") // Error message
    );
    SYS_LOG_D("Event Loop Created!");

    switch (_wifiMode)
    {
        case WIFI_MODE_STA:
        {
            SYS_LOG_I("WIFI STA Initializing...!");
            _espNetifSta = esp_netif_create_default_wifi_sta();
            SYS_LOG_D("Default WIFI STA Created!");
        }
        break;

        case WIFI_MODE_AP:
        {
            SYS_LOG_I("WIFI SOFT AP Initializing...");
            _espNetifAp = esp_netif_create_default_wifi_ap();
            SYS_LOG_D("Default AP Created!");
        }
        break;

        case WIFI_MODE_APSTA:
        {
            SYS_LOG_I("WIFI SOFT APSTA Initializing...");
            _espNetifAp  = esp_netif_create_default_wifi_ap();
            _espNetifSta = esp_netif_create_default_wifi_sta();
            SYS_LOG_D("Default WIFI STA and AP Created!");
        }
        break;

        default:
            SYS_LOG_E("WIFI Mode not set yet!");
            return ERROR_INVALID_CONFIG;
    }

    /// Initialize the WiFi driver with the default configuration.
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    RETURN_IF_ERROR(
        (esp_wifi_init(&cfg) != ESP_OK),  // Expression
        ERROR_INIT_FAILED,                // Error code
        SYS_LOG_E("esp_wifi_init failed") // Error message
    );
    SYS_LOG_D("WIFI Initialized!");

    /// Register WiFi and IP event handlers with the default event loop.
    esp_event_handler_instance_t instanceWifi;
    esp_event_handler_instance_t instanceIp;
    RETURN_IF_ERROR(
        (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &cpx_wifi::_wifiEventHandler, this, &instanceWifi) != ESP_OK || // Expression
         esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &cpx_wifi::_ipEventHandler, this, &instanceIp) != ESP_OK),
        ERROR_INIT_FAILED,                             // Error code
        SYS_LOG_E("Failed to register event handlers") // Error message
    );

    _wifiInitialized = true;
    return ERROR_SUCCESS;
}

sys_error_t cpx_wifi::_wifiStart()
{
    RETURN_IF_ERROR(
        (esp_wifi_set_mode(_wifiMode) != ESP_OK), // Expression
        ERROR_FAIL,                               // Error code
        SYS_LOG_E("esp_wifi_set_mode failed")     // Error message
    );

    switch (_wifiMode)
    {
        case WIFI_MODE_STA:
        {
            RETURN_IF_ERROR(
                (esp_wifi_set_config(WIFI_IF_STA, &_wifiConfigSta) != ESP_OK), // Expression
                ERROR_FAIL,                                                    // Error code
                SYS_LOG_E("esp_wifi_set_config(STA) failed")                   // Error message
            );
        }
        break;

        case WIFI_MODE_AP:
        {
            RETURN_IF_ERROR(
                (esp_wifi_set_config(WIFI_IF_AP, &_wifiConfigAp) != ESP_OK), // Expression
                ERROR_FAIL,                                                  // Error code
                SYS_LOG_E("esp_wifi_set_config(AP) failed")                  // Error message
            );
        }
        break;

        case WIFI_MODE_APSTA:
        {
            RETURN_IF_ERROR(
                (esp_wifi_set_config(WIFI_IF_AP, &_wifiConfigAp) != ESP_OK || // Expression
                 esp_wifi_set_config(WIFI_IF_STA, &_wifiConfigSta) != ESP_OK),
                ERROR_FAIL,                                    // Error code
                SYS_LOG_E("esp_wifi_set_config(APSTA) failed") // Error message
            );
        }
        break;

        default:
            SYS_LOG_E("WIFI Mode not set yet!");
            return ERROR_NOT_IMPLEMENTED;
    }

    RETURN_IF_ERROR(
        (esp_wifi_start() != ESP_OK),      // Expression
        ERROR_FAIL,                        // Error code
        SYS_LOG_E("esp_wifi_start failed") // Error message
    );
    return ERROR_SUCCESS;
}

/** LEGACY APIS (kept for existing consumers) *********************************/

void cpx_wifi::setWifiMode(wifi_mode_t mode)
{
    _wifiMode = mode;
}

wifi_mode_t cpx_wifi::getWifiMode()
{
    return _wifiMode;
}

sys_error_t cpx_wifi::scan(void* config)
{
    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }
    RETURN_IF_ERROR(
        (esp_wifi_clear_ap_list() != ESP_OK),      // Expression
        ERROR_FAIL,                                // Error code
        SYS_LOG_E("esp_wifi_clear_ap_list failed") // Error message
    );
    RETURN_IF_ERROR(
        (esp_wifi_scan_start(static_cast<wifi_scan_config_t*>(config), true) != ESP_OK), // Expression
        ERROR_FAIL,                                                                      // Error code
        SYS_LOG_E("esp_wifi_scan_start failed")                                          // Error message
    );
    return ERROR_SUCCESS;
}

EventGroupHandle_t& cpx_wifi::getWifiEventGroup()
{
    return _wifiEventGroup;
}

sys_error_t cpx_wifi::getScanResults(QueueHandle_t apRecordsResult)
{
    if (apRecordsResult == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    if (!_wifiInitialized)
    {
        SYS_LOG_E("WiFi not initialized!");
        return ERROR_FAIL;
    }

    std::unique_ptr<wifi_ap_record_t[]> apInfo(new (std::nothrow) wifi_ap_record_t[wifiScanMaxRecords]);
    RETURN_IF_ERROR(
        (!apInfo),                                                 // Expression
        ERROR_OUT_OF_MEMORY,                                       // Error code
        SYS_LOG_E("Out of memory - Scan record allocation failed") // Error message
    );
    memset(apInfo.get(), 0, sizeof(wifi_ap_record_t) * wifiScanMaxRecords);

    uint16_t scanCountResult = wifiScanMaxRecords;
    RETURN_IF_ERROR(
        (esp_wifi_scan_get_ap_records(&scanCountResult, apInfo.get()) != ESP_OK), // Expression
        ERROR_FAIL,                                                               // Error code
        SYS_LOG_E("esp_wifi_scan_get_ap_records failed")                          // Error message
    );

    SYS_LOG_I("Total APs scanned = %u", scanCountResult);

    for (uint16_t i = 0; i < scanCountResult; i++)
    {
        wifiApRecord_t apRecord;
        memset(&apRecord, 0, sizeof(apRecord));
        memcpy(apRecord.ssid, apInfo[i].ssid, sizeof(apRecord.ssid));
        apRecord.authmode = static_cast<uint8_t>(apInfo[i].authmode);
        xQueueSendToBack(apRecordsResult, &apRecord, 0);
    }

    return ERROR_SUCCESS;
}

/** EVENT HANDLERS ************************************************************/

void cpx_wifi::_wifiEventHandler(void* arg, esp_event_base_t /*eventBase*/, int32_t eventId, void* eventData)
{
    cpx_wifi* self = static_cast<cpx_wifi*>(arg);
    if (self == nullptr)
    {
        return;
    }

    const EventGroupHandle_t& wifiEventGroup = self->_wifiEventGroup;
    hal_wifi_event_t          halEvent       = {};
    halEvent.event_type                      = HAL_WIFI_EVENT_MAX;
    halEvent.state                           = self->_wifiState;
    halEvent.timestamp_ms                    = static_cast<uint32_t>(esp_timer_get_time() / 1000);

    switch (eventId)
    {
            /***************************************************************
             *                  HANDLING WIFI STA EVENTS
             **************************************************************/

        case WIFI_EVENT_WIFI_READY: /**< WiFi ready */
        {
            SYS_LOG_I("WIFI_EVENT_WIFI_READY");
            halEvent.event_type = HAL_WIFI_EVENT_READY;
        }
        break;

        case WIFI_EVENT_SCAN_DONE: /**< Finished scanning AP */
        {
            SYS_LOG_I("WIFI_EVENT_SCAN_DONE");
            xEventGroupSetBits(wifiEventGroup, WIFI_SCAN_DONE);
            self->_setState(WIFI_STATE_SCAN_DONE);
            halEvent.event_type = HAL_WIFI_EVENT_SCAN_DONE;
            halEvent.state      = WIFI_STATE_SCAN_DONE;
        }
        break;

        case WIFI_EVENT_STA_START: /**< Station start */
        {
            SYS_LOG_I("WIFI_EVENT_STA_START");
            xEventGroupSetBits(wifiEventGroup, WIFI_STA_STARTED);
            halEvent.event_type = HAL_WIFI_EVENT_STA_START;
        }
        break;

        case WIFI_EVENT_STA_STOP: /**< Station stop */
        {
            SYS_LOG_I("WIFI_EVENT_STA_STOP");
            halEvent.event_type = HAL_WIFI_EVENT_STA_STOP;
        }
        break;

        case WIFI_EVENT_STA_CONNECTED: /**< Station connected to AP */
        {
            SYS_LOG_I("WIFI_EVENT_STA_CONNECTED");
            xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED);
            self->_setState(WIFI_STATE_CONNECTED);
            halEvent.event_type = HAL_WIFI_EVENT_STA_CONNECTED;
            halEvent.state      = WIFI_STATE_CONNECTED;
            if (eventData != nullptr)
            {
                wifi_event_sta_connected_t* event = static_cast<wifi_event_sta_connected_t*>(eventData);
                memcpy(halEvent.data.sta_connected.bssid, event->bssid, WIFI_MAC_ADDR_LEN);
                halEvent.data.sta_connected.channel = event->channel;
                strncpy(
                    halEvent.data.sta_connected.ssid, reinterpret_cast<const char*>(event->ssid), event->ssid_len < WIFI_SSID_MAX_LEN ? event->ssid_len : WIFI_SSID_MAX_LEN);
            }
        }
        break;

        case WIFI_EVENT_STA_DISCONNECTED: /**< Station disconnected from AP */
        {
            SYS_LOG_I("WIFI_EVENT_STA_DISCONNECTED");
            xEventGroupSetBits(wifiEventGroup, WIFI_DISCONNECTED);
            self->_setState(WIFI_STATE_DISCONNECTED);
            halEvent.event_type = HAL_WIFI_EVENT_STA_DISCONNECTED;
            halEvent.state      = WIFI_STATE_DISCONNECTED;
            if (eventData != nullptr)
            {
                wifi_event_sta_disconnected_t* event  = static_cast<wifi_event_sta_disconnected_t*>(eventData);
                halEvent.data.sta_disconnected.reason = event->reason;
            }
        }
        break;

        case WIFI_EVENT_STA_AUTHMODE_CHANGE: /**< the auth mode of AP connected by device's station changed */
        {
            SYS_LOG_I("WIFI_EVENT_STA_AUTHMODE_CHANGE");
            halEvent.event_type = HAL_WIFI_EVENT_STA_AUTHMODE_CHANGE;
        }
        break;

            /***************************************************************
             *                  HANDLING WIFI AP EVENTS
             **************************************************************/

        case WIFI_EVENT_AP_START: /**< Soft-AP start */
        {
            SYS_LOG_I("WIFI_EVENT_AP_START");
            halEvent.event_type = HAL_WIFI_EVENT_AP_START;
        }
        break;

        case WIFI_EVENT_AP_STOP: /**< Soft-AP stop */
        {
            SYS_LOG_I("WIFI_EVENT_AP_STOP");
            halEvent.event_type = HAL_WIFI_EVENT_AP_STOP;
        }
        break;

        case WIFI_EVENT_AP_STACONNECTED: /**< a station connected to Soft-AP */
        {
            SYS_LOG_I("WIFI_EVENT_AP_STACONNECTED");
            if (eventData != nullptr)
            {
                wifi_event_ap_staconnected_t* event = static_cast<wifi_event_ap_staconnected_t*>(eventData);
                SYS_LOG_I("Station " MACSTR " join, AID= %u", MAC2STR(event->mac), event->aid);
                halEvent.event_type = HAL_WIFI_EVENT_AP_STA_CONNECTED;
                memcpy(halEvent.data.ap_sta.mac, event->mac, WIFI_MAC_ADDR_LEN);
                halEvent.data.ap_sta.aid = event->aid;
            }
        }
        break;

        case WIFI_EVENT_AP_STADISCONNECTED: /**< a station disconnected from Soft-AP */
        {
            SYS_LOG_I("WIFI_EVENT_AP_STADISCONNECTED");
            if (eventData != nullptr)
            {
                wifi_event_ap_stadisconnected_t* event = static_cast<wifi_event_ap_stadisconnected_t*>(eventData);
                SYS_LOG_I("Station " MACSTR " leave, AID= %u", MAC2STR(event->mac), event->aid);
                halEvent.event_type = HAL_WIFI_EVENT_AP_STA_DISCONNECTED;
                memcpy(halEvent.data.ap_sta.mac, event->mac, WIFI_MAC_ADDR_LEN);
                halEvent.data.ap_sta.aid = event->aid;
            }
        }
        break;

        case WIFI_EVENT_AP_PROBEREQRECVED: /**< Receive probe request packet in soft-AP interface */
            SYS_LOG_D("WIFI_EVENT_AP_PROBEREQRECVED");
            return;

        default:
            return;
    }

    self->_dispatchEvent(halEvent);
}

void cpx_wifi::_ipEventHandler(void* arg, esp_event_base_t /*eventBase*/, int32_t eventId, void* eventData)
{
    cpx_wifi* self = static_cast<cpx_wifi*>(arg);
    if (self == nullptr)
    {
        return;
    }

    EventGroupHandle_t& wifiEventGroup = self->_wifiEventGroup;
    hal_wifi_event_t    halEvent       = {};
    halEvent.event_type                = HAL_WIFI_EVENT_MAX;
    halEvent.state                     = self->_wifiState;
    halEvent.timestamp_ms              = static_cast<uint32_t>(esp_timer_get_time() / 1000);

    switch (eventId)
    {
        case IP_EVENT_STA_GOT_IP: /*!< station got IP from connected AP */
        {
            ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
            SYS_LOG_I("IP_EVENT_STA_GOT_IP");

            char ipString[24] = "IP: ";
            snprintf(&ipString[4], sizeof(ipString) - 4, IPSTR, IP2STR(&event->ip_info.ip));
            SYS_LOG_I("%s", ipString);

            xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED);
            xEventGroupClearBits(wifiEventGroup, WIFI_DISCONNECTED);
            self->_setState(WIFI_STATE_CONNECTED);

            halEvent.event_type = HAL_WIFI_EVENT_STA_GOT_IP;
            halEvent.state      = WIFI_STATE_CONNECTED;
            snprintf(halEvent.data.got_ip.ip.ip_address, WIFI_IP_ADDR_STR_LEN, IPSTR, IP2STR(&event->ip_info.ip));
            snprintf(halEvent.data.got_ip.ip.netmask, WIFI_IP_ADDR_STR_LEN, IPSTR, IP2STR(&event->ip_info.netmask));
            snprintf(halEvent.data.got_ip.ip.gateway, WIFI_IP_ADDR_STR_LEN, IPSTR, IP2STR(&event->ip_info.gw));
        }
        break;

        case IP_EVENT_STA_LOST_IP: /*!< station lost IP and the IP is reset to 0 */
        {
            SYS_LOG_I("IP_EVENT_STA_LOST_IP");
            xEventGroupSetBits(wifiEventGroup, WIFI_DISCONNECTED);
            xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED);
            self->_setState(WIFI_STATE_DISCONNECTED);

            halEvent.event_type = HAL_WIFI_EVENT_STA_LOST_IP;
            halEvent.state      = WIFI_STATE_DISCONNECTED;
        }
        break;

        case IP_EVENT_AP_STAIPASSIGNED: /*!< soft-AP assign an IP to a connected station */
        {
            SYS_LOG_D("IP_EVENT_AP_STAIPASSIGNED");
            return;
        }
        break;

        default:
            return;
    }

    self->_dispatchEvent(halEvent);
}
