/** @file       IBleGattProtocol.hpp
 *  @brief      Platform-independent GATT profile registration contract.
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

/**
 * @class IBleGattProtocol
 * @brief Registers portable GATT profiles and forwards BLE host events.
 *
 * This interface does not initialize, start, stop, or deinitialize the injected
 * BLE HAL. The composition root owns the HAL lifecycle.
 *
 * @note Thread-Safety: Calls that mutate the profile or callback state require
 *       application-task serialization. Implementations forward events in the
 *       BLE host callback context.
 */
class IBleGattProtocol
{
public:
    virtual ~IBleGattProtocol() = default;

    /**
     * @brief Register the configured GATT profiles and attach to BLE events.
     *
     * Must be called after HAL init() and before HAL start(). Profile storage
     * remains caller-owned and must outlive the BLE host.
     *
     * @return ERROR_SUCCESS on success, otherwise an error status.
     */
    virtual sys_error_t init() = 0;

    /**
     * @brief Detach the event callback without changing BLE host lifecycle.
     *
     * @return ERROR_SUCCESS on success, otherwise an error status.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Register a consumer for BLE host and GATT client events.
     *
     * Must be called before init(). Event payload pointers are valid only
     * during the callback.
     *
     * @param[in] callback Callback to receive events; nullptr disables forwarding.
     * @param[in] userData Opaque context passed to the callback.
     * @return ERROR_SUCCESS on success, otherwise an error status.
     */
    virtual sys_error_t setEventCallback(hal_ble_event_cb_t callback, void* userData = nullptr) = 0;
};
