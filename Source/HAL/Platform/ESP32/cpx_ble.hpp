/** @file       cpx_ble.hpp
 *  @brief      ESP32 NimBLE implementation of the generic BLE HAL.
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

#include <atomic>
#include <cstddef>
#include <cstdint>

struct ble_gap_event;
struct ble_gatt_access_ctxt;
struct ble_gatt_attr;
struct ble_gatt_chr;
struct ble_gatt_dsc;
struct ble_gatt_error;
struct ble_gatt_svc;

/**
 * @class cpx_ble
 * @brief Owns one ESP32 NimBLE host/controller instance for standalone BLE use.
 *
 * NimBLE is process-global on ESP32; only one cpx_ble instance may own it.
 * This driver must not be initialized when ESP-Matter's native BLE manager
 * owns the host/controller.
 *
 * @note Thread-Safety: Application calls must be serialized. BLE event and
 *       GATT access callbacks run in the NimBLE host context and must not block
 *       or re-enter this driver.
 */
class cpx_ble final : public IHal_Cpx_Ble
{
public:
    /**
     * @brief Construct an uninitialized standalone NimBLE driver.
     */
    cpx_ble();

    /**
     * @brief Release owned NimBLE resources when possible.
     */
    ~cpx_ble() override;

    cpx_ble(const cpx_ble&)            = delete;
    cpx_ble& operator=(const cpx_ble&) = delete;
    cpx_ble(cpx_ble&&)                 = delete;
    cpx_ble& operator=(cpx_ble&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t start() override;
    sys_error_t get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t stop() override;
    sys_error_t deInit() override;

    sys_error_t setConfig(const hal_ble_config_t& config) override;
    sys_error_t setEventCallback(hal_ble_event_cb_t callback, void* userData = nullptr) override;
    sys_error_t getCapabilities(hal_ble_capabilities_t& capabilities) const override;
    sys_error_t startScan(const hal_ble_scan_config_t& config) override;
    sys_error_t stopScan() override;
    sys_error_t startAdvertising(const hal_ble_advertising_config_t& config) override;
    sys_error_t stopAdvertising() override;
    sys_error_t connect(const hal_ble_address_t& address, std::uint32_t timeoutMs = 0U) override;
    sys_error_t disconnect(hal_ble_connection_id_t connectionId) override;
    sys_error_t secureConnection(hal_ble_connection_id_t connectionId) override;
    sys_error_t respondToPairing(hal_ble_connection_id_t connectionId, bool accept, std::uint32_t passkey = 0U) override;
    sys_error_t registerGattServices(const hal_ble_gatt_service_t* services, std::size_t serviceCount) override;
    sys_error_t getLocalCharacteristicHandle(std::size_t serviceIndex, std::size_t characteristicIndex, std::uint16_t& valueHandle) const override;
    sys_error_t discoverServices(hal_ble_connection_id_t connectionId, hal_ble_request_id_t& requestId, const hal_ble_uuid_t* uuidFilter = nullptr) override;
    sys_error_t discoverCharacteristics(
        hal_ble_connection_id_t connectionId, std::uint16_t startHandle, std::uint16_t endHandle, hal_ble_request_id_t& requestId,
        const hal_ble_uuid_t* uuidFilter = nullptr) override;
    sys_error_t discoverDescriptors(hal_ble_connection_id_t connectionId, std::uint16_t startHandle, std::uint16_t endHandle, hal_ble_request_id_t& requestId) override;
    sys_error_t readCharacteristic(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, hal_ble_request_id_t& requestId) override;
    sys_error_t writeCharacteristic(
        hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength, hal_ble_gatt_write_type_t writeType,
        hal_ble_request_id_t& requestId) override;
    sys_error_t subscribe(
        hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, std::uint16_t cccdHandle, bool notifications, bool indications,
        hal_ble_request_id_t& requestId) override;
    sys_error_t sendNotification(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength) override;
    sys_error_t sendIndication(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength) override;
    bool        isInitialized() const override;

private:
    enum class procedure_state_t : std::uint8_t
    {
        IDLE = 0,
        STARTING,
        ACTIVE,
        STOPPING
    };

    static void _hostTask(void* arg);
    static void _onHostSync();
    static void _onHostReset(int reason);
    static int  _gapEventHandler(struct ble_gap_event* event, void* arg);
    static int  _gattAccessHandler(std::uint16_t connectionHandle, std::uint16_t attributeHandle, struct ble_gatt_access_ctxt* context, void* arg);
    static int  _serviceDiscoveryCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, const struct ble_gatt_svc* service, void* arg);
    static int  _characteristicDiscoveryCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, const struct ble_gatt_chr* characteristic, void* arg);
    static int  _descriptorDiscoveryCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, const struct ble_gatt_dsc* descriptor, void* arg);
    static int  _readCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, struct ble_gatt_attr* attribute, void* arg);
    static int  _writeCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, struct ble_gatt_attr* attribute, void* arg);

    sys_error_t _validateConfig(const hal_ble_config_t& config) const;
    void        _emitEvent(hal_ble_event_type_t event, const hal_ble_event_data_t& eventData);

    hal_ble_config_t                  _config;
    hal_ble_event_cb_t                _eventCallback;
    void*                             _eventUserData;
    std::atomic_bool                  _initialized;
    std::atomic_bool                  _hostStarted;
    std::atomic_bool                  _hostSynced;
    std::atomic<std::uint32_t>        _activeEventCallbacks;
    std::atomic<procedure_state_t>    _scanState;
    std::atomic<procedure_state_t>    _advertisingState;
    std::atomic_bool                  _servicesRegistered;
    std::atomic<hal_ble_request_id_t> _nextRequestId;
};
