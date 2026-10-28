/** @file       cpx_ble.cpp
 *  @brief      ESP32 platform implementation of the BLE peripheral HAL
 *              (NimBLE host) supporting Matter commissioning.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

/*** INCLUDES ****************************************************************/

#include "cpx_ble.h"

#include "System/errorTranslateHandler.h"
#include "System/error_definitions.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "host/ble_att.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_id.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"

#include <cstring>

/*** MACROS ******************************************************************/

#define CPX_BLE_DEVICE_NAME_MAX_LEN 31U
#define CPX_BLE_ADV_PAYLOAD_MAX_LEN 31U
#define CPX_BLE_INVALID_CONN_HANDLE 0xFFFFU
#define CPX_BLE_MATTER_SVC_UUID16   0xFFF6U
#define CPX_BLE_MATTER_CHR_UUID128  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66, 0x00, 0x00}

/*** LOCAL VARIABLES *********************************************************/

static cpx_ble* s_instance = nullptr;

static ble_uuid128_t s_btpChrUuid    = BLE_UUID128_INIT(CPX_BLE_MATTER_CHR_UUID128);
static ble_uuid16_t  s_matterSvcUuid = BLE_UUID16_INIT(CPX_BLE_MATTER_SVC_UUID16);

static int cpx_ble_gatt_access_bridge(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg)
{
    return cpx_ble::_gattAccessHandler(connHandle, attrHandle, ctxt, arg);
}

static struct ble_gatt_chr_def s_btpChrs[] = {
    {
        .uuid         = (ble_uuid_t*)&s_btpChrUuid,
        .access_cb    = cpx_ble_gatt_access_bridge,
        .arg          = nullptr,
        .descriptors  = nullptr,
        .flags        = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_INDICATE,
        .min_key_size = 0,
        .val_handle   = nullptr,
        .cpfd         = nullptr,
    },
    {
        .uuid         = nullptr,
        .access_cb    = nullptr,
        .arg          = nullptr,
        .descriptors  = nullptr,
        .flags        = 0,
        .min_key_size = 0,
        .val_handle   = nullptr,
        .cpfd         = nullptr,
    },
};

static const struct ble_gatt_svc_def s_gattSvcs[] = {
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = (ble_uuid_t*)&s_matterSvcUuid,
        .includes        = nullptr,
        .characteristics = s_btpChrs,
    },
    {
        .type            = BLE_GATT_SVC_TYPE_END,
        .uuid            = nullptr,
        .includes        = nullptr,
        .characteristics = nullptr,
    },
};

/*** LOCAL FUNCTIONS *********************************************************/

static void cpx_ble_host_task(void* arg)
{
    (void)arg;
    nimble_port_run();
}

static void cpx_ble_onSync(void)
{
    if (s_instance != nullptr)
    {
        cpx_ble::_onHostSync();
    }
}

static void cpx_ble_onReset(int reason)
{
    if (s_instance != nullptr)
    {
        cpx_ble::_onHostReset(reason);
    }
}

/*** CLASS IMPLEMENTATION ****************************************************/

cpx_ble::cpx_ble()
    : _config{}
    , _eventCb(nullptr)
    , _eventUserData(nullptr)
    , _mutex(nullptr)
    , _initialized(false)
    , _hostSynced(false)
    , _advertising(false)
    , _connHandle(CPX_BLE_INVALID_CONN_HANDLE)
    , _mtu(23)
    , _btpValHandle(0)
{
    std::memset(&_config, 0, sizeof(_config));
    s_instance = this;
}

cpx_ble::~cpx_ble()
{
    (void)deInit();
    s_instance = nullptr;
}

sys_error_t cpx_ble::init(void* params)
{
    (void)params;

    if (_initialized)
    {
        return ERROR_INVALID_STATE;
    }

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr)
    {
        return ERROR_OUT_OF_MEMORY;
    }

    esp_err_t err = esp_nimble_hci_init();
    if (err != ESP_OK)
    {
        SYS_LOG_E("cpx_ble: esp_nimble_hci_init failed (%d)", (int)err);
        vSemaphoreDelete((SemaphoreHandle_t)_mutex);
        _mutex = nullptr;
        return TRANSLATE_ERROR(err);
    }

    err = nimble_port_init();
    if (err != ESP_OK)
    {
        SYS_LOG_E("cpx_ble: nimble_port_init failed (%d)", (int)err);
        (void)esp_nimble_hci_deinit();
        vSemaphoreDelete((SemaphoreHandle_t)_mutex);
        _mutex = nullptr;
        return TRANSLATE_ERROR(err);
    }

    ble_hs_cfg.reset_cb        = cpx_ble_onReset;
    ble_hs_cfg.sync_cb         = cpx_ble_onSync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    int rc = ble_gatts_count_cfg(s_gattSvcs);
    if (rc != 0)
    {
        SYS_LOG_E("cpx_ble: ble_gatts_count_cfg failed (%d)", rc);
        nimble_port_deinit();
        (void)esp_nimble_hci_deinit();
        vSemaphoreDelete((SemaphoreHandle_t)_mutex);
        _mutex = nullptr;
        return TRANSLATE_ERROR(rc);
    }

    rc = ble_gatts_add_svcs(s_gattSvcs);
    if (rc != 0)
    {
        SYS_LOG_E("cpx_ble: ble_gatts_add_svcs failed (%d)", rc);
        nimble_port_deinit();
        (void)esp_nimble_hci_deinit();
        vSemaphoreDelete((SemaphoreHandle_t)_mutex);
        _mutex = nullptr;
        return TRANSLATE_ERROR(rc);
    }

    _resetState();
    _initialized = true;

    SYS_LOG_I("cpx_ble: initialized");
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::start()
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }

    int rc = ble_hs_start();
    if (rc != 0)
    {
        SYS_LOG_E("cpx_ble: ble_hs_start failed (%d)", rc);
        return TRANSLATE_ERROR(rc);
    }

    nimble_port_freertos_init(cpx_ble_host_task);

    SYS_LOG_I("cpx_ble: host started");
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::get(void* data)
{
    if (data == nullptr)
    {
        return ERROR_NULL_POINTER;
    }

    hal_ble_config_t* out = static_cast<hal_ble_config_t*>(data);
    if (_lock() != ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }
    *out = _config;
    _unlock();
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::set(void* data)
{
    if (data == nullptr)
    {
        return ERROR_NULL_POINTER;
    }
    return setConfig(*static_cast<const hal_ble_config_t*>(data));
}

sys_error_t cpx_ble::stop()
{
    return stopAdvertising();
}

sys_error_t cpx_ble::deInit()
{
    if (!_initialized)
    {
        return ERROR_SUCCESS;
    }

    (void)stopAdvertising();

    if (_connHandle != CPX_BLE_INVALID_CONN_HANDLE)
    {
        (void)ble_gap_terminate(_connHandle, BLE_ERR_REM_USER_CONN_TERM);
        _connHandle = CPX_BLE_INVALID_CONN_HANDLE;
    }

    int rc = nimble_port_stop();
    if (rc != 0)
    {
        SYS_LOG_W("cpx_ble: nimble_port_stop returned %d", rc);
    }

    nimble_port_deinit();
    (void)esp_nimble_hci_deinit();

    if (_mutex != nullptr)
    {
        vSemaphoreDelete((SemaphoreHandle_t)_mutex);
        _mutex = nullptr;
    }

    _resetState();
    _initialized = false;
    _hostSynced  = false;

    SYS_LOG_I("cpx_ble: deinitialized");
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::setConfig(const hal_ble_config_t& config)
{
    if (config.device_name[0] == '\0')
    {
        return ERROR_INVALID_ARG;
    }
    if (config.adv_interval_min_ms == 0 || config.adv_interval_max_ms < config.adv_interval_min_ms)
    {
        return ERROR_INVALID_ARG;
    }
    if (config.mtu_size < 23)
    {
        return ERROR_INVALID_ARG;
    }

    if (_lock() != ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }
    _config                                          = config;
    _config.device_name[CPX_BLE_DEVICE_NAME_MAX_LEN] = '\0';
    _unlock();

    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::setEventCallback(hal_ble_event_cb_t callback, void* userData)
{
    if (_lock() != ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }
    _eventCb       = callback;
    _eventUserData = userData;
    _unlock();
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::startAdvertising(const uint8_t* adv_payload, size_t payload_len)
{
    if (!_initialized || !_hostSynced)
    {
        return ERROR_NOT_INITIALIZED;
    }
    if (payload_len > CPX_BLE_ADV_PAYLOAD_MAX_LEN)
    {
        return ERROR_BUFFER_OVERFLOW;
    }
    if (payload_len > 0 && adv_payload == nullptr)
    {
        return ERROR_NULL_POINTER;
    }

    if (_lock() != ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }

    if (_advertising)
    {
        _unlock();
        return ERROR_INVALID_STATE;
    }

    int rc = 0;
    if (payload_len > 0)
    {
        rc = ble_gap_adv_set_data(adv_payload, (int)payload_len);
    }
    else
    {
        struct ble_hs_adv_fields fields;
        std::memset(&fields, 0, sizeof(fields));
        fields.flags            = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
        fields.name             = (uint8_t*)_config.device_name;
        fields.name_len         = (uint8_t)strlen(_config.device_name);
        fields.name_is_complete = 1;
        fields.uuids16          = &s_matterSvcUuid;
        fields.num_uuids16      = 1;
        rc                      = ble_gap_adv_set_fields(&fields);
    }

    if (rc != 0)
    {
        _unlock();
        SYS_LOG_E("cpx_ble: adv set data failed (%d)", rc);
        return TRANSLATE_ERROR(rc);
    }

    struct ble_gap_adv_params advParams;
    std::memset(&advParams, 0, sizeof(advParams));
    advParams.conn_mode = BLE_GAP_CONN_MODE_UND;
    advParams.disc_mode = BLE_GAP_DISC_MODE_GEN;
    advParams.itvl_min  = (uint16_t)((_config.adv_interval_min_ms * 1000U) / 625U);
    advParams.itvl_max  = (uint16_t)((_config.adv_interval_max_ms * 1000U) / 625U);

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, nullptr, BLE_HS_FOREVER, &advParams, _gapEventHandler, this);
    if (rc != 0)
    {
        _unlock();
        SYS_LOG_E("cpx_ble: adv start failed (%d)", rc);
        return TRANSLATE_ERROR(rc);
    }

    _advertising = true;
    _unlock();

    hal_ble_event_data_t ev{};
    ev.conn_handle = CPX_BLE_INVALID_CONN_HANDLE;
    _emitEvent(hal_ble_event_t::ADVERTISING_STARTED, ev);

    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::stopAdvertising()
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }

    if (_lock() != ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }

    if (!_advertising)
    {
        _unlock();
        return ERROR_SUCCESS;
    }

    int rc       = ble_gap_adv_stop();
    _advertising = false;
    _unlock();

    if (rc != 0)
    {
        return TRANSLATE_ERROR(rc);
    }

    hal_ble_event_data_t ev{};
    ev.conn_handle = CPX_BLE_INVALID_CONN_HANDLE;
    _emitEvent(hal_ble_event_t::ADVERTISING_STOPPED, ev);

    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::sendIndication(uint16_t conn_handle, const uint8_t* data, size_t len)
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }
    if (data == nullptr || len == 0)
    {
        return ERROR_INVALID_ARG;
    }

    struct os_mbuf* om = ble_hs_mbuf_from_flat(data, len);
    if (om == nullptr)
    {
        return ERROR_OUT_OF_MEMORY;
    }

    int rc = ble_gatts_indicate_custom(conn_handle, _btpValHandle, om);
    if (rc != 0)
    {
        SYS_LOG_E("cpx_ble: indicate failed (%d)", rc);
        return TRANSLATE_ERROR(rc);
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::disconnect(uint16_t conn_handle)
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }

    int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (rc != 0)
    {
        return TRANSLATE_ERROR(rc);
    }
    return ERROR_SUCCESS;
}

bool cpx_ble::isConnected()
{
    return _connHandle != CPX_BLE_INVALID_CONN_HANDLE;
}

sys_error_t cpx_ble::_lock()
{
    if (_mutex == nullptr)
    {
        return ERROR_NOT_INITIALIZED;
    }
    return (xSemaphoreTake((SemaphoreHandle_t)_mutex, portMAX_DELAY) == pdTRUE) ? ERROR_SUCCESS : ERROR_FAIL;
}

void cpx_ble::_unlock()
{
    if (_mutex != nullptr)
    {
        (void)xSemaphoreGive((SemaphoreHandle_t)_mutex);
    }
}

void cpx_ble::_emitEvent(hal_ble_event_t event, const hal_ble_event_data_t& data)
{
    hal_ble_event_cb_t cb = _eventCb;
    void*              ud = _eventUserData;
    if (cb != nullptr)
    {
        cb(event, data, ud);
    }
}

void cpx_ble::_resetState()
{
    _advertising  = false;
    _connHandle   = CPX_BLE_INVALID_CONN_HANDLE;
    _mtu          = 23;
    _btpValHandle = 0;
}

int cpx_ble::_gapEventHandler(struct ble_gap_event* gapEvent, void* arg)
{
    (void)arg;
    if (gapEvent == nullptr || s_instance == nullptr)
    {
        return 0;
    }

    hal_ble_event_data_t ev{};
    std::memset(&ev, 0, sizeof(ev));

    switch (gapEvent->type)
    {
        case BLE_GAP_EVENT_CONNECT:
        {
            if (gapEvent->connect.status == 0)
            {
                s_instance->_connHandle = gapEvent->connect.conn_handle;
                ev.conn_handle          = gapEvent->connect.conn_handle;
                s_instance->_emitEvent(hal_ble_event_t::CONNECTED, ev);
            }
            else
            {
                s_instance->_advertising = false;
                s_instance->_emitEvent(hal_ble_event_t::ADVERTISING_STOPPED, ev);
            }
            break;
        }
        case BLE_GAP_EVENT_DISCONNECT:
        {
            ev.conn_handle          = gapEvent->disconnect.conn.conn_handle;
            s_instance->_connHandle = CPX_BLE_INVALID_CONN_HANDLE;
            s_instance->_emitEvent(hal_ble_event_t::DISCONNECTED, ev);
            break;
        }
        case BLE_GAP_EVENT_MTU:
        {
            s_instance->_mtu = gapEvent->mtu.value;
            ev.conn_handle   = gapEvent->mtu.conn_handle;
            ev.mtu           = gapEvent->mtu.value;
            s_instance->_emitEvent(hal_ble_event_t::MTU_UPDATED, ev);
            break;
        }
        case BLE_GAP_EVENT_SUBSCRIBE:
        {
            if (gapEvent->subscribe.attr_handle == s_instance->_btpValHandle && gapEvent->subscribe.cur_indicate)
            {
                ev.conn_handle = gapEvent->subscribe.conn_handle;
                s_instance->_emitEvent(hal_ble_event_t::DATA_RECEIVED, ev);
            }
            break;
        }
        default:
            break;
    }

    return 0;
}

int cpx_ble::_gattAccessHandler(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg)
{
    (void)arg;
    (void)attrHandle;

    if (s_instance == nullptr || ctxt == nullptr)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    struct ble_gatt_access_ctxt* accessCtxt = ctxt;

    if (accessCtxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        struct os_mbuf* om       = accessCtxt->om;
        uint16_t        totalLen = OS_MBUF_PKTLEN(om);
        if (totalLen == 0)
        {
            return 0;
        }

        uint8_t buffer[512];
        if (totalLen > sizeof(buffer))
        {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        int rc = ble_hs_mbuf_to_flat(om, buffer, sizeof(buffer), &totalLen);
        if (rc != 0)
        {
            return BLE_ATT_ERR_UNLIKELY;
        }

        hal_ble_event_data_t ev{};
        ev.conn_handle = connHandle;
        ev.mtu         = s_instance->_mtu;
        ev.payload     = buffer;
        ev.payload_len = totalLen;
        s_instance->_emitEvent(hal_ble_event_t::DATA_RECEIVED, ev);
    }

    return 0;
}

void cpx_ble::_onHostSync()
{
    if (s_instance == nullptr)
    {
        return;
    }

    s_instance->_hostSynced = true;

    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0)
    {
        SYS_LOG_E("cpx_ble: ensure_addr failed (%d)", rc);
    }

    uint8_t addrType = 0;
    (void)ble_hs_id_infer_auto(0, &addrType);

    uint16_t defHandle = 0;
    uint16_t valHandle = 0;
    rc                 = ble_gatts_find_chr((ble_uuid_t*)&s_matterSvcUuid, (ble_uuid_t*)&s_btpChrUuid, &defHandle, &valHandle);
    if (rc == 0)
    {
        s_instance->_btpValHandle = valHandle;
        SYS_LOG_I("cpx_ble: BTP characteristic val_handle=%u", valHandle);
    }
    else
    {
        SYS_LOG_W("cpx_ble: ble_gatts_find_chr failed (%d)", rc);
    }

    SYS_LOG_I("cpx_ble: host synced");
}

void cpx_ble::_onHostReset(int reason)
{
    if (s_instance == nullptr)
    {
        return;
    }

    SYS_LOG_W("cpx_ble: host reset (reason %d)", reason);
    s_instance->_hostSynced  = false;
    s_instance->_advertising = false;
    s_instance->_connHandle  = CPX_BLE_INVALID_CONN_HANDLE;
}

void cpx_ble::_onMtuExchange(uint16_t connHandle, uint16_t mtu)
{
    if (s_instance == nullptr)
    {
        return;
    }

    s_instance->_mtu = mtu;

    hal_ble_event_data_t ev{};
    ev.conn_handle = connHandle;
    ev.mtu         = mtu;
    s_instance->_emitEvent(hal_ble_event_t::MTU_UPDATED, ev);
}
