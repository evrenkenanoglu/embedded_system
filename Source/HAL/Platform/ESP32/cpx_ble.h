/** @file       cpx_ble.h
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
#ifndef CPX_BLE_HPP
#define CPX_BLE_HPP

#include "HAL/IHAL/IHal_Cpx_Ble.h"

#include <cstddef>
#include <cstdint>

struct ble_gatt_access_ctxt;
struct ble_gap_event;

/** MACROS ********************************************************************/

/** TYPEDEFS ******************************************************************/

/** CLASSES *******************************************************************/

/**
 * @class cpx_ble
 * @brief ESP32 NimBLE-host implementation of IHal_Cpx_Ble.
 *
 * Owns the NimBLE host lifecycle (HCI + port init/start/deinit), GAP
 * advertising and a GATT service exposing the Matter BTP characteristic.
 * Thread-safe via an internal mutex; event callbacks are dispatched from the
 * NimBLE host task context.
 */
class cpx_ble : public IHal_Cpx_Ble
{
public:
    cpx_ble();
    ~cpx_ble() override;

    /* Rule of Five: resource-owning class, non-copyable/non-movable */
    cpx_ble(const cpx_ble&)            = delete;
    cpx_ble& operator=(const cpx_ble&) = delete;
    cpx_ble(cpx_ble&&)                 = delete;
    cpx_ble& operator=(cpx_ble&&)      = delete;

    /* IHAL_CPX base contract */
    sys_error_t init(void* params = nullptr) override;
    sys_error_t start() override;
    sys_error_t get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t stop() override;
    sys_error_t deInit() override;

    /* IHal_Cpx_Ble contract */
    sys_error_t setConfig(const hal_ble_config_t& config) override;
    sys_error_t setEventCallback(hal_ble_event_cb_t callback, void* userData = nullptr) override;
    sys_error_t startAdvertising(const uint8_t* adv_payload, size_t payload_len) override;
    sys_error_t stopAdvertising() override;
    sys_error_t sendIndication(uint16_t conn_handle, const uint8_t* data, size_t len) override;
    sys_error_t disconnect(uint16_t conn_handle) override;
    bool        isConnected() override;

private:
    hal_ble_config_t   _config;
    hal_ble_event_cb_t _eventCb;
    void*              _eventUserData;
    void*              _mutex;
    bool               _initialized;
    bool               _hostSynced;
    bool               _advertising;
    uint16_t           _connHandle;
    uint16_t           _mtu;
    uint16_t           _btpValHandle;

    sys_error_t _lock();
    void        _unlock();
    void        _emitEvent(hal_ble_event_t event, const hal_ble_event_data_t& data);
    void        _resetState();

    static int  _gapEventHandler(struct ble_gap_event* event, void* arg);
    static int  _gattAccessHandler(uint16_t connHandle, uint16_t attrHandle, void* ctxt, void* arg);
    static void _onHostSync();
    static void _onHostReset(int reason);
    static void _onMtuExchange(uint16_t connHandle, uint16_t mtu);

public:
    static int _gattAccessHandler(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg);
};

#endif /* CPX_BLE_HPP */
