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

#define WIFI_SCAN_MAX_RECORDS 16
#define WIFI_SCAN_TIMEOUT     pdTICKS_TO_MS(1500) // 1.5 seconds in ticks

#define WIFI_CONNECTED        BIT0
#define WIFI_DISCONNECTED     BIT1
#define WIFI_SCAN_DONE        BIT2

typedef struct
{
    uint8_t ssid[33];
    uint8_t authmode;
} wifiApRecord_t;

class cpx_wifi : public IHAL_CPX
{
private:
    std::string        _ssid;
    std::string        _password;
    wifi_mode_t        _wifiMode;
    wifi_config_t      _wifiConfig;
    EventGroupHandle_t _wifiEventGroup;
    bool               _wifiInitialized;
    QueueHandle_t     _apRecordsResult;

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

    /**
     * @brief Get the Wifi Mode object
     *
     * @return wifi_mode_t
     */
    wifi_mode_t getWifiMode();

    /**
     * @brief Scan for available WiFi networks.
     *
     * @param config wifi_scan_config_t
     * @param result wifi_ap_record_t
     * @param scanCount Number of APs to scan for
     * @return sys_error_t
     */
    sys_error_t scan(void* config);

    /**
     * @brief Get the Scan Results object
     *
     * @param apRecordsResult
     * @return sys_error_t
     */
    sys_error_t getScanResults(QueueHandle_t apRecordsResult);


    sys_error_t clearScanResults();

    /**
     * @brief Connect to a WiFi network.
     *
     * @return sys_error_t
     */
    sys_error_t connect();

    /**
     * @brief Disconnect from the current WiFi network.
     *
     * @return sys_error_t
     */
    sys_error_t disconnect();

    /**
     * @brief Get Wifi Event Group object
     *
     * @return EventGroupHandle_t&
     */
    EventGroupHandle_t& getWifiEventGroup();
};

#endif /* CPX_WIFI_HPP */
