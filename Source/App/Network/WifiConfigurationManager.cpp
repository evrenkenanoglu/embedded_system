/** @file       WifiConfigurationManager.cpp
 *  @brief      Platform-independent WiFi connection manager (application layer).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#include "WifiConfigurationManager.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include "freertos/task.h"

#include <cstring>

/** CONSTANTS *****************************************************************/

namespace
{
    // NVS keys shared with the historic provisioning layout.
    const char* const kDefaultSsidKey     = "wifiSsid";
    const char* const kDefaultPasswordKey = "wifiPassword";
} // namespace

/** CLASSES *******************************************************************/

WifiConfigurationManager::WifiConfigurationManager(IHAL_CPX_WIFI& wifi, const WifiCredentialsStore& credentials, const hal_wifi_ap_config_t* apConfig)
    : _wifi(wifi)
    , _credentials(credentials)
    , _apConfig{}
    , _hasApConfig(apConfig != nullptr)
    , _workerHandle(nullptr)
    , _eventGroup(nullptr)
    , _state(WifiMgrState::BOOT)
    , _connectAttempts(0)
    , _scanRequest(WifiScanRequest::NONE)
    , _scanResults(nullptr)
    , _scanResultCount(0)
    , _status{}
    , _statusMutex(nullptr)
    , _subscribers{}
    , _subscribersMutex(nullptr)
{
    if (_credentials.mem != nullptr && (_credentials.ssidKey == nullptr || _credentials.passwordKey == nullptr))
    {
        // Normalize the descriptor so callers may pass a bare memory HAL.
        _credentials.ssidKey     = kDefaultSsidKey;
        _credentials.passwordKey = kDefaultPasswordKey;
    }

    if (_hasApConfig)
    {
        _apConfig = *apConfig;
    }

    _eventGroup  = xEventGroupCreate();
    _statusMutex = xSemaphoreCreateMutex();
    _scanResults = new (std::nothrow) hal_wifi_ap_record_t[kMaxScanResults];

    _status.state           = _state;
    _status.halState        = WIFI_STATE_DISCONNECTED;
    _status.mode            = WIFI_MODE_DISABLED;
    _status.provisioning    = false;
    _status.ssid[0]         = '\0';
    _status.ip[0]           = '\0';
    _status.rssi            = 0;
    _status.channel         = 0;
    _status.connectAttempts = 0;
}

WifiConfigurationManager::~WifiConfigurationManager()
{
    stop();

    if (_eventGroup != nullptr)
    {
        vEventGroupDelete(_eventGroup);
        _eventGroup = nullptr;
    }

    if (_statusMutex != nullptr)
    {
        vSemaphoreDelete(_statusMutex);
        _statusMutex = nullptr;
    }

    if (_subscribersMutex != nullptr)
    {
        vSemaphoreDelete(_subscribersMutex);
        _subscribersMutex = nullptr;
    }

    delete[] _scanResults;
    _scanResults     = nullptr;
    _scanResultCount = 0;
}

/** LIFECYCLE *************************************************************/

sys_error_t WifiConfigurationManager::start()
{
    RETURN_IF_ERROR(
        (_eventGroup == nullptr || _statusMutex == nullptr || _scanResults == nullptr), // Expression
        ERROR_NOT_INITIALIZED,                                                          // Error code
        SYS_LOG_E("WiFi manager resources not allocated")                               // Error message
    );

    // The HAL delivers events in driver context; the worker owns all HAL calls.
    RETURN_IF_ERROR(
        (_wifi.setEventCallback(_halEventCb, this) != ERROR_SUCCESS), // Expression
        ERROR_FAIL,                                                   // Error code
        SYS_LOG_E("Failed to register WiFi event callback")           // Error message
    );

    RETURN_IF_ERROR(
        (xTaskCreate(_workerTrampoline, kWorkerName, kWorkerStackSize, this, kWorkerPriority, &_workerHandle) != pdPASS), // Expression
        ERROR_FAIL,                                                                                                       // Error code
        SYS_LOG_E("Failed to create WiFi manager worker task")                                                            // Error message
    );

    setState(State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t WifiConfigurationManager::stop()
{
    if (_workerHandle != nullptr)
    {
        vTaskDelete(_workerHandle);
        _workerHandle = nullptr;
    }

    setState(State::STOPPED);

    // Unregister the observer so no callback can fire after destruction.
    (void)_wifi.setEventCallback(nullptr, nullptr);

    if (_eventGroup != nullptr)
    {
        xEventGroupClearBits(_eventGroup, WIFI_CFG_CREDENTIALS_STORED | WIFI_CFG_SCAN_REQUESTED | WIFI_CFG_SCAN_DONE);
    }

    return ERROR_SUCCESS;
}

sys_error_t WifiConfigurationManager::pause()
{
    // Keep the current WiFi association alive but stop reacting to events and
    // reconnect attempts; the worker parks until resume() is called.
    if (getState() != State::RUNNING)
    {
        return ERROR_INVALID_STATE;
    }

    setState(State::PAUSED);
    xEventGroupClearBits(_eventGroup, WIFI_CFG_SCAN_REQUESTED | WIFI_CFG_CREDENTIALS_STORED);
    _paused = true;
    return ERROR_SUCCESS;
}

sys_error_t WifiConfigurationManager::resume()
{
    if (getState() != State::PAUSED)
    {
        return ERROR_INVALID_STATE;
    }

    _paused = false;
    setState(State::RUNNING);
    return ERROR_SUCCESS;
}

/** PUBLIC API ************************************************************/

sys_error_t WifiConfigurationManager::storeCredentials(const char* ssid, const char* password)
{
    RETURN_IF_ERROR(
        (ssid == nullptr || password == nullptr || ssid[0] == '\0'), // Expression
        ERROR_INVALID_ARG,                                           // Error code
        SYS_LOG_E("storeCredentials: invalid arguments")             // Error message
    );

    if (_credentials.mem != nullptr)
    {
        RETURN_IF_ERROR(
            (_persistCredentials(ssid, password) != ERROR_SUCCESS), // Expression
            ERROR_FAIL,                                             // Error code
            SYS_LOG_E("Failed to persist WiFi credentials")         // Error message
        );
    }

    xEventGroupSetBits(_eventGroup, WIFI_CFG_CREDENTIALS_STORED);
    return ERROR_SUCCESS;
}

sys_error_t WifiConfigurationManager::applyCommissionedCredentials(const char* ssid, const char* password)
{
    RETURN_IF_ERROR(
        (ssid == nullptr || password == nullptr || ssid[0] == '\0'), // Expression
        ERROR_INVALID_ARG,                                           // Error code
        SYS_LOG_E("applyCommissionedCredentials: invalid arguments") // Error message
    );

    RETURN_IF_ERROR(
        (!hal_wifi_validate_ssid(ssid)),                                  // Expression
        ERROR_INVALID_ARG,                                                // Error code
        SYS_LOG_E("applyCommissionedCredentials: SSID validation failed") // Error message
    );

    RETURN_IF_ERROR(
        (!hal_wifi_validate_password(password, HAL_WIFI_AUTH_WPA2_PSK)),      // Expression
        ERROR_INVALID_ARG,                                                    // Error code
        SYS_LOG_E("applyCommissionedCredentials: password validation failed") // Error message
    );

    if (_credentials.mem != nullptr)
    {
        RETURN_IF_ERROR(
            (_persistCredentials(ssid, password) != ERROR_SUCCESS), // Expression
            ERROR_FAIL,                                             // Error code
            SYS_LOG_E("Failed to persist commissioned credentials") // Error message
        );
    }

    xEventGroupSetBits(_eventGroup, WIFI_CFG_CREDENTIALS_STORED);
    return ERROR_SUCCESS;
}

sys_error_t WifiConfigurationManager::requestScan(bool block)
{
    xEventGroupClearBits(_eventGroup, WIFI_CFG_SCAN_DONE);
    _scanRequest = WifiScanRequest::ANY;
    xEventGroupSetBits(_eventGroup, WIFI_CFG_SCAN_REQUESTED);

    if (!block)
    {
        return ERROR_SUCCESS;
    }

    EventBits_t bits = xEventGroupWaitBits(_eventGroup, WIFI_CFG_SCAN_DONE, pdTRUE, pdFALSE, pdMS_TO_TICKS(kScanTimeoutMs));
    RETURN_IF_ERROR(
        ((bits & WIFI_CFG_SCAN_DONE) == 0),          // Expression
        ERROR_TIMEOUT,                               // Error code
        SYS_LOG_E("Timed out waiting for WiFi scan") // Error message
    );
    return ERROR_SUCCESS;
}

sys_error_t WifiConfigurationManager::getScanResults(hal_wifi_ap_record_t* records, uint16_t max_records, uint16_t& actual_records)
{
    RETURN_IF_ERROR(
        (records == nullptr || max_records == 0),      // Expression
        ERROR_INVALID_ARG,                             // Error code
        SYS_LOG_E("getScanResults: invalid arguments") // Error message
    );

    RETURN_IF_ERROR(
        (_scanResultCount == 0),                          // Expression
        ERROR_INVALID_STATE,                              // Error code
        SYS_LOG_E("getScanResults: no results available") // Error message
    );

    uint16_t count = (_scanResultCount < max_records) ? _scanResultCount : max_records;
    memcpy(records, _scanResults, sizeof(hal_wifi_ap_record_t) * count);
    actual_records = count;
    return ERROR_SUCCESS;
}

void WifiConfigurationManager::getStatus(WifiMgrStatus& status) const
{
    if (xSemaphoreTake(_statusMutex, portMAX_DELAY) == pdTRUE)
    {
        status = _status;
        xSemaphoreGive(_statusMutex);
    }
}

sys_error_t WifiConfigurationManager::subscribeEvent(WifiEventCb callback, void* user_data, WifiEventSubscription& subscription)
{
    if (callback == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    if (xSemaphoreTake(_subscribersMutex, portMAX_DELAY) != pdTRUE)
    {
        return ERROR_TIMEOUT;
    }

    sys_error_t result = ERROR_OUT_OF_MEMORY;
    for (size_t i = 0; i < kWifiEventMaxSubscribers; ++i)
    {
        if (_subscribers[i].callback == nullptr)
        {
            _subscribers[i].callback = callback;
            _subscribers[i].userData = user_data;
            subscription             = static_cast<WifiEventSubscription>(i + 1); // 0 reserved as "invalid"
            result                   = ERROR_SUCCESS;
            break;
        }
    }

    xSemaphoreGive(_subscribersMutex);
    return result;
}

sys_error_t WifiConfigurationManager::unsubscribeEvent(WifiEventSubscription subscription)
{
    if (subscription == 0 || subscription > kWifiEventMaxSubscribers)
    {
        return ERROR_INVALID_ARG;
    }

    if (xSemaphoreTake(_subscribersMutex, portMAX_DELAY) != pdTRUE)
    {
        return ERROR_TIMEOUT;
    }

    _subscribers[subscription - 1].callback = nullptr;
    _subscribers[subscription - 1].userData = nullptr;

    xSemaphoreGive(_subscribersMutex);
    return ERROR_SUCCESS;
}

/** PRIVATE METHODS *******************************************************/

void WifiConfigurationManager::_workerTrampoline(void* param)
{
    static_cast<WifiConfigurationManager*>(param)->_worker();
}

void WifiConfigurationManager::_halEventCb(const hal_wifi_event_t* event, void* user_data)
{
    // Runs in HAL driver context: hand the event to the worker via its
    // notification channel (event-driven, no relay task).
    if (event == nullptr || user_data == nullptr)
    {
        return;
    }

    WifiConfigurationManager* self = static_cast<WifiConfigurationManager*>(user_data);
    self->_applyEvent(*event);
}

void WifiConfigurationManager::_applyEvent(const hal_wifi_event_t& event)
{
    // NOTE: called from the HAL event context. Only touches state the
    // worker re-reads; heavier reactions happen in the worker loop.
    switch (event.event_type)
    {
        case HAL_WIFI_EVENT_STA_GOT_IP:
        {
            _state = WifiMgrState::CONNECTED;
            _publishEvent(WifiEvent::CONNECTED);
            break;
        }

        case HAL_WIFI_EVENT_STA_DISCONNECTED:
        {
            if (_state == WifiMgrState::CONNECTED || _state == WifiMgrState::CONNECTING)
            {
                _state = WifiMgrState::RECONNECT_WAIT;
            }
            _publishEvent(WifiEvent::DISCONNECTED);
            break;
        }

        case HAL_WIFI_EVENT_SCAN_DONE:
        {
            // HAL event callback must not call back into the interface:
            // the worker fetches the results when it observes SCAN_DONE.
            xEventGroupSetBits(_eventGroup, WIFI_CFG_SCAN_DONE);
            break;
        }

        default:
            break;
    }
}

bool WifiConfigurationManager::_loadCredentials(hal_wifi_sta_config_t& sta)
{
    if (_credentials.mem == nullptr)
    {
        return false;
    }

    memset(&sta, 0, sizeof(sta));
    uint8_t ssid[WifiCredentialsStore::kSsidLen]         = {0};
    uint8_t password[WifiCredentialsStore::kPasswordLen] = {0};

    if (_credentials.mem->readData(_credentials.ssidKey, ssid, sizeof(ssid)) != ERROR_SUCCESS ||
        _credentials.mem->readData(_credentials.passwordKey, password, sizeof(password)) != ERROR_SUCCESS)
    {
        return false;
    }

    if (ssid[0] == '\0' || password[0] == '\0')
    {
        return false;
    }

    strncpy(sta.ssid, reinterpret_cast<const char*>(ssid), sizeof(sta.ssid) - 1);
    strncpy(sta.password, reinterpret_cast<const char*>(password), sizeof(sta.password) - 1);
    sta.threshold_authmode = HAL_WIFI_AUTH_OPEN;
    return true;
}

sys_error_t WifiConfigurationManager::_persistCredentials(const char* ssid, const char* password)
{
    RETURN_ON_ERROR(_credentials.mem->writeData(_credentials.ssidKey, reinterpret_cast<const uint8_t*>(ssid), strlen(ssid) + 1));
    RETURN_ON_ERROR(_credentials.mem->writeData(_credentials.passwordKey, reinterpret_cast<const uint8_t*>(password), strlen(password) + 1));
    return ERROR_SUCCESS;
}

void WifiConfigurationManager::_enterProvisioning()
{
    SYS_LOG_I("WiFi manager: entering provisioning (soft-AP)");

    (void)_wifi.stop();
    (void)_wifi.setMode(WIFI_MODE_STATION_AP);

    if (_hasApConfig)
    {
        (void)_wifi.setConfig(WIFI_MODE_ACCESS_POINT, reinterpret_cast<const hal_wifi_config_t*>(&_apConfig));
    }

    (void)_wifi.start();

    if (xSemaphoreTake(_statusMutex, portMAX_DELAY) == pdTRUE)
    {
        _status.provisioning = true;
        xSemaphoreGive(_statusMutex);
    }

    _state = WifiMgrState::PROVISIONING;
    _publishEvent(WifiEvent::AP_SETUP_READY);
}

void WifiConfigurationManager::_beginConnect()
{
    hal_wifi_sta_config_t sta = {};
    if (!_loadCredentials(sta))
    {
        SYS_LOG_W("WiFi manager: no valid stored credentials");
        _enterProvisioning();
        return;
    }

    (void)_wifi.stop();
    (void)_wifi.setMode(WIFI_MODE_STATION);
    (void)_wifi.setConfig(WIFI_MODE_STATION, reinterpret_cast<const hal_wifi_config_t*>(&sta));
    (void)_wifi.start();

    _state = WifiMgrState::CONNECTING;

    // Leaving soft-AP mode (if it was up) and starting the station.
    _publishEvent(WifiEvent::AP_SETUP_FINISH);
    _publishEvent(WifiEvent::STA_SETUP_READY);

    sys_error_t error = _wifi.connect();
    if (error != ERROR_SUCCESS)
    {
        SYS_LOG_E("WiFi manager: connect request failed (%d)", static_cast<int>(error));
        _state = WifiMgrState::RECONNECT_WAIT;
    }

    if (xSemaphoreTake(_statusMutex, portMAX_DELAY) == pdTRUE)
    {
        strncpy(_status.ssid, sta.ssid, sizeof(_status.ssid) - 1);
        _status.ssid[sizeof(_status.ssid) - 1] = '\0';
        _status.provisioning                   = false;
        _status.connectAttempts                = _connectAttempts;
        xSemaphoreGive(_statusMutex);
    }
}

void WifiConfigurationManager::_publishStatus()
{
    if (xSemaphoreTake(_statusMutex, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    _status.state = _state;

    hal_wifi_state_t halState = WIFI_STATE_DISCONNECTED;
    if (_wifi.getState(halState) == ERROR_SUCCESS)
    {
        _status.halState = halState;
    }

    hal_wifi_mode_t mode = WIFI_MODE_DISABLED;
    if (_wifi.getMode(mode) == ERROR_SUCCESS)
    {
        _status.mode = mode;
    }

    if (_state == WifiMgrState::CONNECTED)
    {
        hal_wifi_network_info_t info = {};
        if (_wifi.getNetworkInfo(info) == ERROR_SUCCESS)
        {
            strncpy(_status.ip, info.ip.ip_address, sizeof(_status.ip) - 1);
            _status.ip[sizeof(_status.ip) - 1] = '\0';
            _status.rssi                       = info.rssi;
            _status.channel                    = info.channel;
            if (info.ssid[0] != '\0')
            {
                strncpy(_status.ssid, info.ssid, sizeof(_status.ssid) - 1);
                _status.ssid[sizeof(_status.ssid) - 1] = '\0';
            }
        }
    }

    xSemaphoreGive(_statusMutex);
}

void WifiConfigurationManager::_publishEvent(WifiEvent event)
{
    WifiMgrStatus snapshot;
    getStatus(snapshot);

    if (xSemaphoreTake(_subscribersMutex, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    // Snapshot the subscriber list under the mutex, then invoke outside it so
    // a subscriber may unsubscribe synchronously without deadlocking.
    WifiEventSubscriber local[kWifiEventMaxSubscribers];
    for (size_t i = 0; i < kWifiEventMaxSubscribers; ++i)
    {
        local[i] = _subscribers[i];
    }
    xSemaphoreGive(_subscribersMutex);

    for (size_t i = 0; i < kWifiEventMaxSubscribers; ++i)
    {
        if (local[i].callback != nullptr)
        {
            local[i].callback(event, snapshot, local[i].userData);
        }
    }
}

void WifiConfigurationManager::_clearEventBits(EventBits_t bits)
{
    xEventGroupClearBits(_eventGroup, bits);
}

void WifiConfigurationManager::_worker()
{
    TickType_t retryDeadline = 0;
    bool       backoffActive = false;
    TickType_t lastPublish   = 0;

    for (;;)
    {
        if (_paused)
        {
            // Parked by pause(): hold the current WiFi association and ignore
            // requests until resume() clears the flag.
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        // React to externally requested actions (scan/credentials) promptly;
        // otherwise idle at the provisioning cadence or until back-off ends.
        TickType_t timeout;
        if (backoffActive)
        {
            TickType_t now = xTaskGetTickCount();
            timeout        = (retryDeadline > now) ? (retryDeadline - now) : 0;
        }
        else
        {
            timeout = pdMS_TO_TICKS(kProvisionIdleMs);
        }

        EventBits_t requestBits = xEventGroupWaitBits(
            _eventGroup,
            WIFI_CFG_SCAN_REQUESTED | WIFI_CFG_CREDENTIALS_STORED | WIFI_CFG_SCAN_DONE,
            pdTRUE,  // clear on exit
            pdFALSE, // any
            timeout);

        const bool scanRequested  = (requestBits & WIFI_CFG_SCAN_REQUESTED) != 0;
        const bool credsRequested = (requestBits & WIFI_CFG_CREDENTIALS_STORED) != 0;
        const bool scanCompleted  = (requestBits & WIFI_CFG_SCAN_DONE) != 0;

        // Handle scan requests in any state.
        if (scanRequested || _scanRequest == WifiScanRequest::ANY)
        {
            _scanRequest = WifiScanRequest::NONE;
            (void)_wifi.startScan(nullptr, true);
            // Completion arrives via the HAL callback (WIFI_CFG_SCAN_DONE);
            // a same-iteration fallback covers HALs without the event.
        }

        if (scanCompleted || _scanRequest == WifiScanRequest::ANY)
        {
            uint16_t actual = 0;
            if (_wifi.getScanResults(_scanResults, kMaxScanResults, actual) == ERROR_SUCCESS)
            {
                _scanResultCount = actual;
            }
            else
            {
                _scanResultCount = 0;
            }
            xEventGroupSetBits(_eventGroup, WIFI_CFG_SCAN_DONE);
        }

        // Credentials stored: reconnect immediately.
        if (credsRequested)
        {
            _connectAttempts = 0;
            backoffActive    = false;
            _state           = WifiMgrState::BOOT;
        }

        switch (_state)
        {
            case WifiMgrState::BOOT:
            {
                SYS_LOG_I("WiFi manager: booting (trying stored credentials)");
                _connectAttempts = 0;
                backoffActive    = false;
                _beginConnect();
            }
            break;

            case WifiMgrState::CONNECTING:
            {
                _publishStatus();
            }
            break;

            case WifiMgrState::CONNECTED:
            {
                _publishStatus();
            }
            break;

            case WifiMgrState::PROVISIONING:
            {
                // Idle in AP mode; the credentials event wakes us up.
                _publishStatus();
            }
            break;

            case WifiMgrState::RECONNECT_WAIT:
            {
                if (!backoffActive)
                {
                    backoffActive = true;
                    retryDeadline = xTaskGetTickCount() + pdMS_TO_TICKS(kRetryDelayMs);
                }
                else
                {
                    backoffActive = false;
                    _connectAttempts++;
                    if (_connectAttempts >= kMaxConnectAttempts)
                    {
                        SYS_LOG_W("WiFi manager: connect attempts exhausted, provisioning");
                        _enterProvisioning();
                    }
                    else
                    {
                        _beginConnect();
                    }
                }
            }
            break;
        }

        // Publish status at most every second in steady states.
        TickType_t now = xTaskGetTickCount();
        if ((now - lastPublish) >= pdMS_TO_TICKS(1000))
        {
            lastPublish = now;
            _publishStatus();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
