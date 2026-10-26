/** @file       cpx_wifi.hpp
 *  @brief      ESP32 implementation of the platform-independent WiFi HAL interface.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#ifndef CPX_WIFI_HPP
#define CPX_WIFI_HPP

#include "HAL/IHAL/IHal_Cpx_Wifi.h"

#include "esp_netif_types.h"
#include "esp_wifi_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include <atomic>

/** MACROS ********************************************************************/

#define WIFI_SCAN_MAX_RECORDS 16
#define WIFI_SCAN_TIMEOUT_MS  1500 // milliseconds
#ifndef WIFI_SCAN_TIMEOUT
// Legacy alias consumed as a FreeRTOS tick count by xEventGroupWaitBits()
// (kept value-compatible with the historical definition).
#define WIFI_SCAN_TIMEOUT pdTICKS_TO_MS(WIFI_SCAN_TIMEOUT_MS)
#endif

// Internal event group bits (ESP32 implementation detail)
#define WIFI_CONNECTED    BIT0
#define WIFI_DISCONNECTED BIT1
#define WIFI_SCAN_DONE    BIT2
#define WIFI_STA_STARTED  BIT3

/** TYPEDEFS ******************************************************************/

/**
 * @brief Legacy scan result record used by existing consumers
 * @note Kept for backward compatibility with the scan results queue.
 */
typedef struct
{
    uint8_t ssid[33];
    uint8_t authmode;
} wifiApRecord_t;

/** CLASSES *******************************************************************/

/**
 * @class cpx_wifi
 * @brief ESP32 (ESP-IDF) implementation of the IHAL_CPX_WIFI contract.
 *
 * Maps the generic WiFi HAL onto the ESP-IDF WiFi driver. All ESP-specific
 * types are kept private; consumers use only the platform-independent
 * interface. Legacy APIs retained below the interface block keep existing
 * consumers building unchanged.
 *
 * @note Thread-Safety: Not thread-safe as a whole. WiFi event handlers run in
 *       the ESP-IDF event-loop task context, so the shared state they mutate
 *       (_wifiState) is atomic; all other members must be accessed from a
 *       single task or synchronized externally by the caller.
 */
class cpx_wifi : public IHAL_CPX_WIFI
{
private:
    /** MEMBERS *******************************************************************/

    wifi_mode_t                   _wifiMode;
    wifi_config_t                 _wifiConfigSta;
    wifi_config_t                 _wifiConfigAp;
    EventGroupHandle_t            _wifiEventGroup;
    bool                          _wifiInitialized;
    QueueHandle_t                 _apRecordsResult;
    esp_netif_t*                  _espNetifAp;
    esp_netif_t*                  _espNetifSta;
    std::atomic<hal_wifi_state_t> _wifiState; ///< Written from the event-loop task, read from application tasks
    hal_wifi_event_cb_t           _eventCallback;
    void*                         _eventCallbackUserData;

private:
    sys_error_t _wifiInit();
    sys_error_t _wifiStart();

    // ESP <-> HAL translation helpers
    static hal_wifi_mode_t      _espModeToHal(wifi_mode_t mode);
    static wifi_mode_t          _halModeToEsp(hal_wifi_mode_t mode);
    static hal_wifi_auth_mode_t _espAuthToHal(wifi_auth_mode_t auth);
    static wifi_auth_mode_t     _halAuthToEsp(hal_wifi_auth_mode_t auth);
    void                        _dispatchEvent(const hal_wifi_event_t& event);
    void                        _setState(hal_wifi_state_t state);

    // Event handlers (registered with the ESP event loop)
    static void _wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    static void _ipEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);

public:
    cpx_wifi(void* config = nullptr);
    ~cpx_wifi();

    // Block copy semantics (owns FreeRTOS/ESP driver resources)
    cpx_wifi(const cpx_wifi&)            = delete;
    cpx_wifi& operator=(const cpx_wifi&) = delete;

    // Block move semantics
    cpx_wifi(cpx_wifi&&)            = delete;
    cpx_wifi& operator=(cpx_wifi&&) = delete;

    // IHAL_CPX lifecycle
    sys_error_t init(void* params = nullptr) override;
    sys_error_t start() override;
    sys_error_t get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t stop() override;
    sys_error_t deInit() override;

    // IHAL_CPX_WIFI interface
    sys_error_t setMode(hal_wifi_mode_t mode) override;
    sys_error_t getMode(hal_wifi_mode_t& mode) override;
    sys_error_t setConfig(hal_wifi_mode_t interface_type, const hal_wifi_config_t* config) override;
    sys_error_t getConfig(hal_wifi_mode_t interface_type, hal_wifi_config_t* config) override;
    sys_error_t connect() override;
    sys_error_t disconnect() override;
    sys_error_t startScan(const hal_wifi_scan_config_t* config = nullptr, bool block = true) override;
    sys_error_t getScanResults(hal_wifi_ap_record_t* records, uint16_t max_records, uint16_t& actual_records) override;
    sys_error_t clearScanResults() override;
    sys_error_t getState(hal_wifi_state_t& state) override;
    sys_error_t getNetworkInfo(hal_wifi_network_info_t& info) override;
    sys_error_t setEventCallback(hal_wifi_event_cb_t callback, void* user_data = nullptr) override;
    sys_error_t setPowerSave(bool enable) override;
    sys_error_t getRSSI(int8_t& rssi) override;
    sys_error_t getMacAddress(hal_wifi_mode_t interface_type, uint8_t* mac) override;
    sys_error_t setMacAddress(hal_wifi_mode_t interface_type, const uint8_t* mac) override;
    sys_error_t getCapabilities(uint32_t& capabilities) override;
    bool        hasCapability(hal_wifi_capabilities_t capability) override;
    bool        isConnected() override;
    bool        isInitialized() override;

public:
    /**
     * @brief Legacy mode API kept for existing consumers.
     *
     * @note Operates on the raw ESP mode value. New code should use
     *       setMode()/getMode() with hal_wifi_mode_t instead.
     */
    void        setWifiMode(wifi_mode_t mode);
    wifi_mode_t getWifiMode();

    /**
     * @brief Legacy scan API kept for existing consumers.
     *
     * @param config Pointer to a native wifi_scan_config_t (or nullptr)
     * @return sys_error_t
     */
    sys_error_t scan(void* config);

    /**
     * @brief Get the internal WiFi event group
     *
     * @note Legacy API kept for existing consumers (event group based tasks).
     *       New code should use setEventCallback() instead.
     *
     * @return EventGroupHandle_t&
     */
    EventGroupHandle_t& getWifiEventGroup();

    /**
     * @brief Legacy scan API: push results into a FreeRTOS queue
     *
     * @param apRecordsResult Queue receiving wifiApRecord_t items
     * @return sys_error_t
     */
    sys_error_t getScanResults(QueueHandle_t apRecordsResult);
};

#endif /* CPX_WIFI_HPP */
