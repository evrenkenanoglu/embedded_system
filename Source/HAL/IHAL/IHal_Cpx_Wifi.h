/**
 * @file IHal_Cpx_Wifi.h
 * @brief Platform-independent WiFi Interface for Hardware Abstraction Layer
 *
 * This file contains declarations for the WiFi complex operations interface.
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
typedef enum
{
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK,
    WIFI_AUTH_WPA2_WPA3_PSK,
    WIFI_AUTH_WAPI_PSK,
    WIFI_AUTH_OWE,
    WIFI_AUTH_WPA3_ENT_192,
    WIFI_AUTH_MAX
} hal_wifi_auth_mode_t;

// WiFi event types
typedef enum
{
    WIFI_EVENT_READY = 0,
    WIFI_EVENT_SCAN_DONE,
    WIFI_EVENT_STA_START,
    WIFI_EVENT_STA_STOP,
    WIFI_EVENT_STA_CONNECTED,
    WIFI_EVENT_STA_DISCONNECTED,
    WIFI_EVENT_STA_AUTHMODE_CHANGE,
    WIFI_EVENT_STA_GOT_IP,
    WIFI_EVENT_STA_LOST_IP,
    WIFI_EVENT_AP_START,
    WIFI_EVENT_AP_STOP,
    WIFI_EVENT_AP_STA_CONNECTED,
    WIFI_EVENT_AP_STA_DISCONNECTED,
    WIFI_EVENT_MAX
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

// WiFi event bits for event groups
#define WIFI_CONNECTED_BIT    (1 << 0)
#define WIFI_DISCONNECTED_BIT (1 << 1)
#define WIFI_SCAN_DONE_BIT    (1 << 2)
#define WIFI_STA_STARTED_BIT  (1 << 3)
#define WIFI_AP_STARTED_BIT   (1 << 4)
#define WIFI_GOT_IP_BIT       (1 << 5)
#define WIFI_LOST_IP_BIT      (1 << 6)

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
    uint16_t             listen_interval;                     /*!< Listen interval for ESP32 station to receive beacon when WIFI_PS_MAX_MODEM is set */
    hal_wifi_auth_mode_t threshold_authmode;                  /*!< The weakest authmode to accept in the scan mode */
    bool                 pmf_cfg_required;                    /*!< Configuration for Protected Management Frame */
    bool                 pmf_cfg_capable;                     /*!< Configuration for Protected Management Frame */
} hal_wifi_sta_config_t;

/**
 * @brief WiFi access point configuration
 */
typedef struct
{
    char                 ssid[WIFI_SSID_MAX_LEN + 1];         /*!< SSID of ESP32 soft-AP. */
    char                 password[WIFI_PASSWORD_MAX_LEN + 1]; /*!< Password of ESP32 soft-AP. */
    uint8_t              ssid_len;                            /*!< Optional length of SSID field. */
    uint8_t              channel;                             /*!< Channel of ESP32 soft-AP */
    hal_wifi_auth_mode_t authmode;                            /*!< Auth mode of ESP32 soft-AP. */
    uint8_t              ssid_hidden;                         /*!< Broadcast SSID or not */
    uint8_t              max_connection;                      /*!< Max number of stations allowed to connect in */
    uint16_t             beacon_interval;                     /*!< Beacon interval which should be multiples of 100. Unit: TU(time unit, 1 TU = 1024 us) */
    bool                 pmf_cfg_required;                    /*!< Configuration for Protected Management Frame */
    bool                 pmf_cfg_capable;                     /*!< Configuration for Protected Management Frame */
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
    char*    ssid;          /*!< SSID of AP */
    uint8_t* bssid;         /*!< MAC address of AP */
    uint8_t  channel;       /*!< Channel, scan the specific channel */
    bool     show_hidden;   /*!< Enable to scan AP whose SSID is hidden */
    uint32_t scan_time_min; /*!< Minimum scan time per channel, units: millisecond */
    uint32_t scan_time_max; /*!< Maximum scan time per channel, units: millisecond */
} hal_wifi_scan_config_t;

/**
 * @brief WiFi access point record
 */
typedef struct
{
    uint8_t              bssid[WIFI_MAC_ADDR_LEN];    /*!< MAC address of AP */
    char                 ssid[WIFI_SSID_MAX_LEN + 1]; /*!< SSID of AP */
    uint8_t              primary;                     /*!< Channel of AP */
    uint8_t              second;                      /*!< Second channel of AP */
    int8_t               rssi;                        /*!< Signal strength of AP */
    hal_wifi_auth_mode_t authmode;                    /*!< Authmode of AP */
    uint32_t             pairwise_cipher;             /*!< Pairwise cipher of AP */
    uint32_t             group_cipher;                /*!< Group cipher of AP */
    uint32_t             ant;                         /*!< Antenna used to receive beacon from AP */
    uint16_t             phy_11b       : 1;           /*!< Bit: 0 flag to identify if 11b mode is enabled or not */
    uint16_t             phy_11g       : 1;           /*!< Bit: 1 flag to identify if 11g mode is enabled or not */
    uint16_t             phy_11n       : 1;           /*!< Bit: 2 flag to identify if 11n mode is enabled or not */
    uint16_t             phy_lr        : 1;           /*!< Bit: 3 flag to identify if low rate is enabled or not */
    uint16_t             phy_11ax      : 1;           /*!< Bit: 4 flag to identify if 11ax mode is enabled or not */
    uint16_t             wps           : 1;           /*!< Bit: 5 flag to identify if WPS is supported or not */
    uint16_t             ftm_responder : 1;           /*!< Bit: 6 flag to identify if FTM is supported in responder mode */
    uint16_t             ftm_initiator : 1;           /*!< Bit: 7 flag to identify if FTM is supported in initiator mode */
    uint16_t             reserved      : 8;           /*!< Bit: 8..15 reserved */
    uint8_t              country[3];                  /*!< Country information of AP */
} hal_wifi_ap_record_t;

/**
 * @brief WiFi event structure
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
            char    ssid[33];
            uint8_t bssid[6];
            uint8_t channel;
            int8_t  rssi;
        } connected;

        struct
        {
            uint8_t reason;
        } disconnected;

        struct
        {
            uint32_t ip_address;
            uint32_t netmask;
            uint32_t gateway;
        } got_ip;

        struct
        {
            uint8_t mac[6];
            uint8_t aid;
        } ap_sta_connected;
    } data;
} hal_wifi_event_t;

/**
 * @brief WiFi network information
 */
typedef struct
{
    char                 ssid[33];
    char                 ip_address[16];
    char                 gateway[16];
    char                 netmask[16];
    int8_t               rssi;
    uint8_t              channel;
    hal_wifi_auth_mode_t auth_mode;
    hal_wifi_state_t     state;
} hal_wifi_network_info_t;

/** CLASSES *******************************************************************/

/**
 * @class IHAL_CPX_WIFI
 * @brief Platform-independent WiFi Interface for complex operations
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
     * @brief Get WiFi event queue handle
     * @param event_queue Reference to store event queue handle
     * @return sys_error_t Error code
     */
    virtual sys_error_t getEventQueue(os_queue_handle_t& event_queue) = 0;

    /**
     * @brief Wait for specific WiFi event
     * @param event_bits Event bits to wait for
     * @param timeout_ms Timeout in milliseconds
     * @param clear_bits Whether to clear bits after receiving
     * @return sys_error_t Error code
     */
    virtual sys_error_t waitForEvent(uint32_t event_bits, uint32_t timeout_ms, bool clear_bits = true) = 0;

    /**
     * @brief Set event callback function
     * @param callback Callback function for WiFi events
     * @param user_data User data to pass to callback
     * @return sys_error_t Error code
     */
    virtual sys_error_t setEventCallback(void (*callback)(const hal_wifi_event_t* event, void* user_data), void* user_data = nullptr) = 0;

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
     * @param mac Buffer to store MAC address (6 bytes)
     * @return sys_error_t Error code
     */
    virtual sys_error_t getMacAddress(hal_wifi_mode_t interface_type, uint8_t* mac) = 0;

    /**
     * @brief Set MAC address
     * @param interface_type Interface type (STA or AP)
     * @param mac MAC address to set (6 bytes)
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