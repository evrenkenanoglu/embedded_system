/** @file       WifiConfigurationManager.hpp
 *  @brief      Platform-independent WiFi connection manager (application layer).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#ifndef WIFI_CONFIGURATION_MANAGER_HPP
#define WIFI_CONFIGURATION_MANAGER_HPP

#include "HAL/IHAL/IHal.h"
#include "HAL/IHAL/IHal_Cpx_Wifi.h"
#include "Process/Process.hpp"

#include "esp_bit_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/** MACROS ********************************************************************/

/**
 * @brief Internal worker-task mailboxes (implementation detail, not public API).
 *
 * Cross-thread signaling between the public API / HAL callback context and the
 * manager worker task. State transitions are reported through typed WifiEvent
 * publications; these bits are only the worker's command/completion queue.
 */
#define WIFI_CFG_CREDENTIALS_STORED BIT4 /*!< New credentials stored via storeCredentials() */
#define WIFI_CFG_SCAN_REQUESTED     BIT5 /*!< Scan requested via requestScan() */
#define WIFI_CFG_SCAN_DONE          BIT6 /*!< Scan finished, results available via getScanResults() */

/** TYPEDEFS ******************************************************************/

/**
 * @brief Connection manager operating states
 */
enum class WifiMgrState
{
    BOOT = 0,       /*!< Initial: try stored credentials */
    CONNECTING,     /*!< STA association in progress */
    CONNECTED,      /*!< STA associated and has IP */
    PROVISIONING,   /*!< Soft-AP up, waiting for credentials */
    RECONNECT_WAIT, /*!< Backing off before the next connect attempt */
};

/**
 * @brief Scan request flag (worker-consumed)
 */
enum class WifiScanRequest
{
    NONE = 0, /*!< No scan pending */
    ANY,      /*!< Scan requested by an external consumer */
};

/**
 * @brief Status snapshot exposed to consumers (URIs, network services)
 */
struct WifiMgrStatus
{
    WifiMgrState     state;
    hal_wifi_state_t halState;     /*!< Raw HAL connection state */
    hal_wifi_mode_t  mode;         /*!< Active mode (STATION / ACCESS_POINT / STATION_AP) */
    bool             provisioning; /*!< Soft-AP provisioning active */
    char             ssid[WIFI_SSID_MAX_LEN + 1];
    char             ip[WIFI_IP_ADDR_STR_LEN];
    int8_t           rssi;
    uint8_t          channel;
    uint8_t          connectAttempts; /*!< Attempts made for the current cycle */
};

/**
 * @brief Network events published by WifiConfigurationManager.
 *
 * Subscribers receive a serialized notification from the manager worker task;
 * callbacks must not block and must not call back into the manager.
 */
enum class WifiEvent
{
    AP_SETUP_READY,   /*!< Soft-AP interface is up (provisioning available) */
    AP_SETUP_FINISH,  /*!< Soft-AP interface shut down */
    STA_SETUP_READY,  /*!< Station interface started */
    STA_SETUP_FINISH, /*!< Station interface shut down */
    CONNECTED,        /*!< Station associated and got IP */
    DISCONNECTED,     /*!< Station lost association */
};

/**
 * @brief Subscription handle for WifiEvent notifications.
 *
 * Opaque token returned by subscribeEvent(); pass it to unsubscribeEvent().
 */
typedef uint32_t WifiEventSubscription;

/**
 * @brief Event subscriber callback
 *
 * Invoked from the manager worker task context; must not block.
 */
typedef void (*WifiEventCb)(WifiEvent event, const WifiMgrStatus& status, void* user_data);

/**
 * @brief Max simultaneous event subscribers (fixed, no heap growth at runtime).
 */
static constexpr size_t kWifiEventMaxSubscribers = 8;

/**
 * @brief Credentials access policy for the manager
 *
 * Persists and loads SSID/password pairs through the memory HAL. Keys are
 * owned by the manager; the defaults match the historic NVS layout.
 */
struct WifiCredentialsStore
{
    IHAL_MEM*   mem;
    const char* ssidKey;
    const char* passwordKey;

    static constexpr size_t kSsidLen     = WIFI_SSID_MAX_LEN + 1;
    static constexpr size_t kPasswordLen = WIFI_PASSWORD_MAX_LEN + 1;
};

/**
 * @class WifiConfigurationManager
 * @brief Application-layer WiFi connection manager (standard Process lifecycle).
 *
 * Owns the connection lifecycle: loads stored credentials, connects as a
 * station, falls back to soft-AP provisioning when credentials are missing
 * or invalid, and reconnects on link loss. All WiFi interaction goes
 * through the IHAL_CPX_WIFI interface, so the manager is fully
 * platform independent and testable with a mock HAL.
 *
 * @note Thread-Safety: All public methods may be called from any task.
 *       Internal shared state is guarded by the worker-task ownership
 *       model plus atomic/FreeRTOS primitives; status snapshots are
 *       copied out under a mutex.
 */
class WifiConfigurationManager : public Process
{
public:
    /**
     * @brief Construct the manager.
     *
     * @param wifi WiFi HAL interface reference (must outlive the manager)
     * @param credentials Credential store descriptor (mem may be nullptr to run without persistence)
     * @param apConfig Provisioning soft-AP configuration (may be nullptr to disable provisioning)
     */
    WifiConfigurationManager(IHAL_CPX_WIFI& wifi, const WifiCredentialsStore& credentials, const hal_wifi_ap_config_t* apConfig = nullptr);
    ~WifiConfigurationManager();

    // Block copy semantics (owns FreeRTOS resources and a worker task)
    WifiConfigurationManager(const WifiConfigurationManager&)            = delete;
    WifiConfigurationManager& operator=(const WifiConfigurationManager&) = delete;
    WifiConfigurationManager(WifiConfigurationManager&&)                 = delete;
    WifiConfigurationManager& operator=(WifiConfigurationManager&&)      = delete;

    /** LIFECYCLE *************************************************************/

    sys_error_t start() override;
    sys_error_t stop() override;
    sys_error_t pause() override;
    sys_error_t resume() override;

    /** PUBLIC API ************************************************************/

    /**
     * @brief Store new credentials and trigger a connect attempt.
     *
     * Thread-safe; may be called from any task (e.g. an HTTP POST handler).
     *
     * @param ssid Null-terminated SSID
     * @param password Null-terminated password
     * @return sys_error_t ERROR_SUCCESS on accept
     */
    sys_error_t storeCredentials(const char* ssid, const char* password);

    /**
     * @brief Request a WiFi scan.
     *
     * Thread-safe. The scan runs on the worker task; completion is signalled
     * on the internal event group (WIFI_CFG_SCAN_DONE). A scan requested
     * during provisioning is preferred and executes immediately; otherwise
     * it is executed between connection attempts.
     *
     * @param block If true, block the caller until the scan completes
     *              (bounded by kScanTimeoutMs).
     * @return sys_error_t ERROR_SUCCESS on scan completion (or queued when non-blocking)
     */
    sys_error_t requestScan(bool block = true);

    /**
     * @brief Copy out scan results collected by the last completed scan.
     *
     * @param records Destination buffer
     * @param max_records Capacity of the destination buffer
     * @param actual_records Number of records written
     * @return sys_error_t ERROR_SUCCESS when results were copied
     */
    sys_error_t getScanResults(hal_wifi_ap_record_t* records, uint16_t max_records, uint16_t& actual_records);

    /**
     * @brief Snapshot the current manager status.
     *
     * Thread-safe; copies the status struct under a mutex.
     *
     * @param status Destination snapshot
     */
    void getStatus(WifiMgrStatus& status) const;

    /**
     * @brief Subscribe to manager events.
     *
     * Thread-safe. The callback is invoked serialized from the manager worker
     * task whenever a subscribed event fires.
     *
     * @param callback Subscriber callback (must not be nullptr)
     * @param user_data User context passed back to the callback
     * @param subscription Out handle for unsubscribeEvent()
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_OUT_OF_MEMORY when full
     */
    sys_error_t subscribeEvent(WifiEventCb callback, void* user_data, WifiEventSubscription& subscription);

    /**
     * @brief Unsubscribe a previously registered event subscriber.
     *
     * @param subscription Handle returned by subscribeEvent()
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_ARG when unknown
     */
    sys_error_t unsubscribeEvent(WifiEventSubscription subscription);

private:
    /** INTERNAL TYPES ********************************************************/

    /** PRIVATE METHODS *******************************************************/

    static void _workerTrampoline(void* param);
    void        _worker();
    void        _applyEvent(const hal_wifi_event_t& event);
    static void _halEventCb(const hal_wifi_event_t* event, void* user_data);

    bool        _loadCredentials(hal_wifi_sta_config_t& sta);
    sys_error_t _persistCredentials(const char* ssid, const char* password);
    void        _enterProvisioning();
    void        _beginConnect();
    void        _publishStatus();
    void        _publishEvent(WifiEvent event);
    void        _clearEventBits(EventBits_t bits);

    /** MEMBERS ***************************************************************/

    IHAL_CPX_WIFI&       _wifi;
    WifiCredentialsStore _credentials;
    hal_wifi_ap_config_t _apConfig;
    bool                 _hasApConfig;

    TaskHandle_t       _workerHandle;
    EventGroupHandle_t _eventGroup;

    volatile WifiMgrState    _state;
    volatile uint8_t         _connectAttempts;
    volatile WifiScanRequest _scanRequest;
    volatile bool            _paused;

    hal_wifi_ap_record_t* _scanResults;
    volatile uint16_t     _scanResultCount;

    WifiMgrStatus             _status;
    mutable SemaphoreHandle_t _statusMutex;

    struct WifiEventSubscriber
    {
        WifiEventCb callback;
        void*       userData;
    };

    WifiEventSubscriber _subscribers[kWifiEventMaxSubscribers];
    SemaphoreHandle_t   _subscribersMutex;

    /** CONSTANTS *************************************************************/

    static constexpr uint16_t    kWorkerStackSize = 4096; // bytes
    static constexpr UBaseType_t kWorkerPriority  = 5;
    static constexpr const char* kWorkerName      = "wifiCfgMgr";

    static constexpr uint8_t  kMaxConnectAttempts = 3;
    static constexpr uint32_t kRetryDelayMs       = 3000;  // back-off between attempts
    static constexpr uint32_t kScanTimeoutMs      = 15000; // bounded wait for scan completion
    static constexpr uint32_t kProvisionIdleMs    = 1000;  // idle wait cadence
    static constexpr size_t   kMaxScanResults     = 16;    // matches WIFI_SCAN_MAX_RECORDS on ESP32
};

#endif /* WIFI_CONFIGURATION_MANAGER_HPP */
