/** @file       IHal_Cpx_Wifi.h
 *  @brief      Platform-independent WiFi interface for the Hardware Abstraction Layer.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#ifndef IHAL_CPX_WIFI_H
#define IHAL_CPX_WIFI_H

#include "IHal.h"

/** MACROS ********************************************************************/
#ifndef WIFI_SSID_MAX_LEN
#define WIFI_SSID_MAX_LEN 32
#endif

#ifndef WIFI_PASSWORD_MAX_LEN
#define WIFI_PASSWORD_MAX_LEN 64
#endif

#ifndef WIFI_MAC_ADDR_LEN
#define WIFI_MAC_ADDR_LEN 6
#endif

#ifndef WIFI_IP_ADDR_STR_LEN
#define WIFI_IP_ADDR_STR_LEN 16 /*!< "255.255.255.255" + NUL */
#endif

#ifndef WIFI_COUNTRY_CODE_LEN
#define WIFI_COUNTRY_CODE_LEN 3
#endif

/** CONSTANTS *****************************************************************/

// WiFi modes
typedef enum
{
    WIFI_MODE_DISABLED = 0,
    WIFI_MODE_STATION,
    WIFI_MODE_ACCESS_POINT,
    WIFI_MODE_STATION_AP
} hal_wifi_mode_t;

// WiFi connection states
typedef enum
{
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_CONNECTION_FAILED,
    WIFI_STATE_SCANNING,
    WIFI_STATE_SCAN_DONE
} hal_wifi_state_t;

// WiFi authentication modes
// Members prefixed HAL_WIFI_ to avoid collisions with platform-native
// enums (e.g. ESP-IDF wifi_auth_mode_t WIFI_AUTH_*).
typedef enum
{
    HAL_WIFI_AUTH_OPEN = 0,
    HAL_WIFI_AUTH_WEP,
    HAL_WIFI_AUTH_WPA_PSK,
    HAL_WIFI_AUTH_WPA2_PSK,
    HAL_WIFI_AUTH_WPA_WPA2_PSK,
    HAL_WIFI_AUTH_WPA2_ENTERPRISE,
    HAL_WIFI_AUTH_WPA3_PSK,
    HAL_WIFI_AUTH_WPA2_WPA3_PSK,
    HAL_WIFI_AUTH_WAPI_PSK,
    HAL_WIFI_AUTH_OWE,
    HAL_WIFI_AUTH_WPA3_ENT_192,
    HAL_WIFI_AUTH_MAX
} hal_wifi_auth_mode_t;

// WiFi event types
// Members prefixed HAL_WIFI_EVENT_ to avoid collisions with platform-native
// enums (e.g. ESP-IDF wifi_event_t WIFI_EVENT_*).
typedef enum
{
    HAL_WIFI_EVENT_READY = 0,
    HAL_WIFI_EVENT_SCAN_DONE,
    HAL_WIFI_EVENT_STA_START,
    HAL_WIFI_EVENT_STA_STOP,
    HAL_WIFI_EVENT_STA_CONNECTED,
    HAL_WIFI_EVENT_STA_DISCONNECTED,
    HAL_WIFI_EVENT_STA_AUTHMODE_CHANGE,
    HAL_WIFI_EVENT_STA_GOT_IP,
    HAL_WIFI_EVENT_STA_LOST_IP,
    HAL_WIFI_EVENT_AP_START,
    HAL_WIFI_EVENT_AP_STOP,
    HAL_WIFI_EVENT_AP_STA_CONNECTED,
    HAL_WIFI_EVENT_AP_STA_DISCONNECTED,
    HAL_WIFI_EVENT_MAX
} hal_wifi_event_type_t;

/**
 * @brief WiFi capabilities flags
 */
typedef enum
{
    WIFI_CAP_STA            = (1 << 0), /*!< Station mode support */
    WIFI_CAP_AP             = (1 << 1), /*!< Access Point mode support */
    WIFI_CAP_CONCURRENT     = (1 << 2), /*!< Concurrent STA+AP support */
    WIFI_CAP_WPS            = (1 << 3), /*!< WPS support */
    WIFI_CAP_PMF            = (1 << 4), /*!< Protected Management Frame */
    WIFI_CAP_WPA3           = (1 << 5), /*!< WPA3 support */
    WIFI_CAP_ENTERPRISE     = (1 << 6), /*!< Enterprise auth support */
    WIFI_CAP_POWER_SAVE     = (1 << 7), /*!< Power save modes */
    WIFI_CAP_HIDDEN_SSID    = (1 << 8), /*!< Hidden SSID broadcast */
    WIFI_CAP_CHANNEL_SWITCH = (1 << 9)  /*!< Dynamic channel switching */
} hal_wifi_capabilities_t;

/** TYPEDEFS ******************************************************************/

/**
 * @brief WiFi station configuration
 */
typedef struct
{
    char                 ssid[WIFI_SSID_MAX_LEN + 1];         /*!< SSID of target AP. */
    char                 password[WIFI_PASSWORD_MAX_LEN + 1]; /*!< Password of target AP. */
    bool                 bssid_set;                           /*!< Whether target AP's MAC address is set. */
    uint8_t              bssid[WIFI_MAC_ADDR_LEN];            /*!< MAC address of target AP */
    uint8_t              channel;                             /*!< Channel of target AP. Set to 1~13 to scan starting from the specified channel before connecting to AP */
    uint16_t             listen_interval;                     /*!< Listen interval in beacon periods for power-save operation. 0 = platform default. */
    hal_wifi_auth_mode_t threshold_authmode;                  /*!< The weakest authmode to accept in the scan mode */
    bool                 pmf_required;                        /*!< Require Protected Management Frame */
    bool                 pmf_capable;                         /*!< Support Protected Management Frame */
} hal_wifi_sta_config_t;

/**
 * @brief WiFi access point configuration
 */
typedef struct
{
    char                 ssid[WIFI_SSID_MAX_LEN + 1];         /*!< SSID of the soft-AP. */
    char                 password[WIFI_PASSWORD_MAX_LEN + 1]; /*!< Password of the soft-AP. */
    uint8_t              ssid_len;                            /*!< Optional length of SSID field. */
    uint8_t              channel;                             /*!< Channel of the soft-AP */
    hal_wifi_auth_mode_t authmode;                            /*!< Auth mode of the soft-AP. */
    bool                 ssid_hidden;                         /*!< Broadcast SSID or not */
    uint8_t              max_connection;                      /*!< Max number of stations allowed to connect */
    uint16_t             beacon_interval;                     /*!< Beacon interval in TU (1 TU = 1024 us), multiples of 100. 0 = platform default. */
    bool                 pmf_required;                        /*!< Require Protected Management Frame */
    bool                 pmf_capable;                         /*!< Support Protected Management Frame */
} hal_wifi_ap_config_t;

/**
 * @brief WiFi configuration union
 */
typedef union
{
    hal_wifi_sta_config_t sta; /*!< Configuration of STA */
    hal_wifi_ap_config_t  ap;  /*!< Configuration of AP */
} hal_wifi_config_t;

/**
 * @brief WiFi scan configuration
 */
typedef struct
{
    const char*    ssid;             /*!< SSID of AP to scan for (nullptr = scan all) */
    const uint8_t* bssid;            /*!< MAC address of AP to scan for (nullptr = any) */
    uint8_t        channel;          /*!< Channel to scan (0 = all channels) */
    bool           show_hidden;      /*!< Enable to scan AP whose SSID is hidden */
    uint32_t       scan_time_min_ms; /*!< Minimum scan time per channel in ms (0 = platform default) */
    uint32_t       scan_time_max_ms; /*!< Maximum scan time per channel in ms (0 = platform default) */
} hal_wifi_scan_config_t;

/**
 * @brief WiFi access point record
 *
 * Generic scan result. Platform-specific details (PHY bitfields, FTM,
 * antenna diversity, raw cipher suites) are intentionally abstracted;
 * platforms expose such extras via getCapabilities() or platform_config.
 */
typedef struct
{
    uint8_t              bssid[WIFI_MAC_ADDR_LEN];       /*!< MAC address of AP */
    char                 ssid[WIFI_SSID_MAX_LEN + 1];    /*!< SSID of AP */
    uint8_t              primary_channel;                /*!< Primary channel of AP */
    uint8_t              second_channel;                 /*!< Secondary channel offset of AP (0 = none) */
    int8_t               rssi;                           /*!< Signal strength of AP */
    hal_wifi_auth_mode_t authmode;                       /*!< Authmode of AP */
    bool                 phy_11b;                        /*!< 11b mode supported */
    bool                 phy_11g;                        /*!< 11g mode supported */
    bool                 phy_11n;                        /*!< 11n mode supported */
    bool                 phy_11ax;                       /*!< 11ax mode supported */
    bool                 wps;                            /*!< WPS supported */
    uint8_t              country[WIFI_COUNTRY_CODE_LEN]; /*!< Country code of AP */
} hal_wifi_ap_record_t;

/**
 * @brief WiFi IP configuration (dotted-decimal strings, platform independent)
 */
typedef struct
{
    char ip_address[WIFI_IP_ADDR_STR_LEN]; /*!< IPv4 address */
    char netmask[WIFI_IP_ADDR_STR_LEN];    /*!< IPv4 netmask */
    char gateway[WIFI_IP_ADDR_STR_LEN];    /*!< IPv4 gateway */
} hal_wifi_ip_info_t;

/**
 * @brief WiFi event payload
 *
 * Delivered via the event callback. Only the union member matching the
 * event_type is valid.
 */
typedef struct
{
    hal_wifi_event_type_t event_type;
    hal_wifi_state_t      state;
    uint32_t              timestamp_ms;
    union
    {
        struct
        {
            char    ssid[WIFI_SSID_MAX_LEN + 1];
            uint8_t bssid[WIFI_MAC_ADDR_LEN];
            uint8_t channel;
            int8_t  rssi;
        } sta_connected;

        struct
        {
            uint8_t reason; /*!< Platform-specific disconnect reason code */
        } sta_disconnected;

        struct
        {
            hal_wifi_ip_info_t ip; /*!< IP settings assigned to the station */
        } got_ip;

        struct
        {
            uint8_t mac[WIFI_MAC_ADDR_LEN]; /*!< MAC of the station joining/leaving the AP */
            uint8_t aid;                    /*!< Association ID */
        } ap_sta;
    } data;
} hal_wifi_event_t;

/**
 * @brief WiFi network information
 */
typedef struct
{
    char                 ssid[WIFI_SSID_MAX_LEN + 1];
    hal_wifi_ip_info_t   ip;
    int8_t               rssi;
    uint8_t              channel;
    hal_wifi_auth_mode_t auth_mode;
    hal_wifi_state_t     state;
} hal_wifi_network_info_t;

/**
 * @brief WiFi event callback
 *
 * Invoked by the implementation when a WiFi event occurs. Depending on the
 * platform this may run in an interrupt or driver context: the callback must
 * be short, must not block, and must not call back into the same interface.
 *
 * @param event Pointer to the event (valid only for the duration of the call)
 * @param user_data User context supplied to setEventCallback()
 */
typedef void (*hal_wifi_event_cb_t)(const hal_wifi_event_t* event, void* user_data);

/** CLASSES *******************************************************************/

/**
 * @class IHAL_CPX_WIFI
 * @brief Platform-independent WiFi Interface for complex operations
 *
 * Event delivery is callback-based only; no RTOS queue, event group or
 * blocking wait primitives are part of the contract. Synchronous callers can
 * implement their own waiting on top of the callback or poll getState().
 */
class IHAL_CPX_WIFI : public IHAL_CPX
{
public:
    /**
     * @brief Set WiFi operating mode
     * @param mode WiFi mode (STA, AP, STA+AP)
     * @return sys_error_t Error code
     */
    virtual sys_error_t setMode(hal_wifi_mode_t mode) = 0;

    /**
     * @brief Get current WiFi operating mode
     * @param mode Reference to store current mode
     * @return sys_error_t Error code
     */
    virtual sys_error_t getMode(hal_wifi_mode_t& mode) = 0;

    /**
     * @brief Set WiFi configuration
     * @param interface_type Interface type (STA or AP)
     * @param config WiFi configuration
     * @return sys_error_t Error code
     */
    virtual sys_error_t setConfig(hal_wifi_mode_t interface_type, const hal_wifi_config_t* config) = 0;

    /**
     * @brief Get WiFi configuration
     * @param interface_type Interface type (STA or AP)
     * @param config Reference to store configuration
     * @return sys_error_t Error code
     */
    virtual sys_error_t getConfig(hal_wifi_mode_t interface_type, hal_wifi_config_t* config) = 0;

    /**
     * @brief Connect to WiFi network (STA mode)
     * @return sys_error_t Error code
     */
    virtual sys_error_t connect() = 0;

    /**
     * @brief Disconnect from WiFi network
     * @return sys_error_t Error code
     */
    virtual sys_error_t disconnect() = 0;

    /**
     * @brief Start WiFi scan
     * @param config Scan configuration (can be nullptr for default scan)
     * @param block Whether to block until scan is complete
     * @return sys_error_t Error code
     */
    virtual sys_error_t startScan(const hal_wifi_scan_config_t* config = nullptr, bool block = true) = 0;

    /**
     * @brief Get scan results
     * @param records Buffer to store scan results
     * @param max_records Maximum number of records to retrieve
     * @param actual_records Reference to store actual number of records retrieved
     * @return sys_error_t Error code
     */
    virtual sys_error_t getScanResults(hal_wifi_ap_record_t* records, uint16_t max_records, uint16_t& actual_records) = 0;

    /**
     * @brief Clear scan results
     * @return sys_error_t Error code
     */
    virtual sys_error_t clearScanResults() = 0;

    /**
     * @brief Get current WiFi state
     * @param state Reference to store current state
     * @return sys_error_t Error code
     */
    virtual sys_error_t getState(hal_wifi_state_t& state) = 0;

    /**
     * @brief Get network information
     * @param info Reference to store network information
     * @return sys_error_t Error code
     */
    virtual sys_error_t getNetworkInfo(hal_wifi_network_info_t& info) = 0;

    /**
     * @brief Set event callback function
     * @param callback Callback function for WiFi events (nullptr to unregister)
     * @param user_data User data to pass to callback
     * @return sys_error_t Error code
     */
    virtual sys_error_t setEventCallback(hal_wifi_event_cb_t callback, void* user_data = nullptr) = 0;

    /**
     * @brief Enable/disable power save mode
     * @param enable True to enable power save, false to disable
     * @return sys_error_t Error code
     */
    virtual sys_error_t setPowerSave(bool enable) = 0;

    /**
     * @brief Get signal strength (RSSI)
     * @param rssi Reference to store RSSI value
     * @return sys_error_t Error code
     */
    virtual sys_error_t getRSSI(int8_t& rssi) = 0;

    /**
     * @brief Get MAC address
     * @param interface_type Interface type (STA or AP)
     * @param mac Buffer to store MAC address (WIFI_MAC_ADDR_LEN bytes)
     * @return sys_error_t Error code
     */
    virtual sys_error_t getMacAddress(hal_wifi_mode_t interface_type, uint8_t* mac) = 0;

    /**
     * @brief Set MAC address
     * @param interface_type Interface type (STA or AP)
     * @param mac MAC address to set (WIFI_MAC_ADDR_LEN bytes)
     * @return sys_error_t Error code
     */
    virtual sys_error_t setMacAddress(hal_wifi_mode_t interface_type, const uint8_t* mac) = 0;

    /**
     * @brief Get platform WiFi capabilities
     * @param capabilities Reference to store capability flags
     * @return sys_error_t Error code
     */
    virtual sys_error_t getCapabilities(uint32_t& capabilities) = 0;

    /**
     * @brief Check if specific capability is supported
     * @param capability Capability to check
     * @return bool True if supported, false otherwise
     */
    virtual bool hasCapability(hal_wifi_capabilities_t capability) = 0;

    /**
     * @brief Check if WiFi is connected
     * @return bool True if connected, false otherwise
     */
    virtual bool isConnected() = 0;

    /**
     * @brief Check if WiFi is initialized
     * @return bool True if initialized, false otherwise
     */
    virtual bool isInitialized() = 0;

    /**
     * @brief Virtual destructor
     */
    virtual ~IHAL_CPX_WIFI() = default;
};

/** HELPER FUNCTIONS **********************************************************/

/**
 * @brief Convert authentication mode to string
 * @param auth_mode Authentication mode
 * @return const char* String representation
 */
const char* hal_wifi_auth_mode_to_string(hal_wifi_auth_mode_t auth_mode);

/**
 * @brief Convert WiFi state to string
 * @param state WiFi state
 * @return const char* String representation
 */
const char* hal_wifi_state_to_string(hal_wifi_state_t state);

/**
 * @brief Convert WiFi mode to string
 * @param mode WiFi mode
 * @return const char* String representation
 */
const char* hal_wifi_mode_to_string(hal_wifi_mode_t mode);

/**
 * @brief Validate SSID
 * @param ssid SSID to validate
 * @return bool True if valid, false otherwise
 */
bool hal_wifi_validate_ssid(const char* ssid);

/**
 * @brief Validate password for given auth mode
 * @param password Password to validate
 * @param auth_mode Authentication mode
 * @return bool True if valid, false otherwise
 */
bool hal_wifi_validate_password(const char* password, hal_wifi_auth_mode_t auth_mode);

#endif // IHAL_CPX_WIFI_H
