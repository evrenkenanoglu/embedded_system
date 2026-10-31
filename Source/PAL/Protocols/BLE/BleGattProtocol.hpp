/** @file       BleGattProtocol.hpp
 *  @brief      Generic GATT profile coordinator over the BLE HAL.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Cpx_Ble.h"
#include "PAL/Protocols/BLE/IBleGattProtocol.hpp"

/**
 * @class BleGattProtocol
 * @brief Validates and registers reusable GATT profiles over an injected BLE HAL.
 *
 * The composition root retains ownership of the HAL and its host lifecycle.
 * Profile definitions and their callback context must remain valid while the
 * BLE host is running.
 *
 * @note Thread-Safety: Initialize and configure callbacks from the application
 *       task. Event delivery occurs asynchronously in the BLE host context;
 *       external synchronization is required when sharing this instance.
 */
class BleGattProtocol final : public IBleGattProtocol
{
public:
    /**
     * @brief Create a GATT profile coordinator over an externally owned HAL.
     *
     * @param[in] bleHal BLE HAL instance.
     * @param[in] services Caller-owned service definitions.
     * @param[in] serviceCount Number of services.
     */
    BleGattProtocol(IHal_Cpx_Ble& bleHal, const hal_ble_gatt_service_t* services, std::size_t serviceCount);

    /**
     * @brief Detach the BLE event callback before releasing the coordinator.
     */
    ~BleGattProtocol() override;

    BleGattProtocol(const BleGattProtocol&)            = delete;
    BleGattProtocol& operator=(const BleGattProtocol&) = delete;
    BleGattProtocol(BleGattProtocol&&)                 = delete;
    BleGattProtocol& operator=(BleGattProtocol&&)      = delete;

    sys_error_t init() override;
    sys_error_t deInit() override;
    sys_error_t setEventCallback(hal_ble_event_cb_t callback, void* userData = nullptr) override;

private:
    static void _halEventThunk(hal_ble_event_type_t event, const hal_ble_event_data_t& eventData, void* userData);
    static bool _isUuidValid(const hal_ble_uuid_t& uuid);
    sys_error_t _validateProfiles() const;

    IHal_Cpx_Ble&                 _bleHal;
    const hal_ble_gatt_service_t* _services;
    std::size_t                   _serviceCount;
    hal_ble_event_cb_t            _eventCallback;
    void*                         _eventUserData;
    bool                          _initialized;
    bool                          _profilesRegistered;
};
