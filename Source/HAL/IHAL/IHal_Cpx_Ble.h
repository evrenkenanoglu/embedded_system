/** @file       IHal_Cpx_Ble.h
 *  @brief      Specialized interface for Bluetooth Low Energy (BLE) peripheral
 *              operations supporting Matter commissioning.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal.h"
#include <cstddef>
#include <cstdint>

/**
 * @brief Asynchronous BLE events surfaced to the application layer.
 */
enum class hal_ble_event_t : uint8_t
{
    ADVERTISING_STARTED = 0, /*!< Advertising successfully started */
    ADVERTISING_STOPPED,     /*!< Advertising stopped (timeout, connect or explicit stop) */
    CONNECTED,               /*!< Central connected */
    DISCONNECTED,            /*!< Central disconnected */
    MTU_UPDATED,             /*!< ATT MTU negotiated/updated */
    DATA_RECEIVED            /*!< Payload received on the Matter BTP characteristic */
};

/**
 * @brief Event payload accompanying a hal_ble_event_t notification.
 */
typedef struct
{
    uint16_t       conn_handle;  /*!< Connection handle (valid for CONNECTED..DATA_RECEIVED) */
    uint16_t       mtu;          /*!< Negotiated ATT MTU (MTU_UPDATED) */
    int8_t         rssi;         /*!< Peer RSSI in dBm, 0 if unknown */
    uint8_t        peer_addr[6]; /*!< Peer BLE address (LSB first) */
    const uint8_t* payload;      /*!< Received data (DATA_RECEIVED only, else nullptr) */
    size_t         payload_len;  /*!< Received data length in bytes */
} hal_ble_event_data_t;

/**
 * @brief BLE event callback signature.
 *
 * @warning May be invoked from the BLE host task context. Handlers must be
 *          short, non-blocking and must not re-enter the BLE HAL.
 */
typedef void (*hal_ble_event_cb_t)(hal_ble_event_t event, const hal_ble_event_data_t& eventData, void* userData);

/**
 * @brief BLE peripheral configuration parameters.
 */
typedef struct
{
    char     device_name[32];     /*!< Advertised device name (NUL terminated) */
    uint16_t adv_interval_min_ms; /*!< Minimum advertising interval in milliseconds */
    uint16_t adv_interval_max_ms; /*!< Maximum advertising interval in milliseconds */
    int8_t   tx_power_dbm;        /*!< Requested TX power in dBm */
    uint16_t service_uuid_16;     /*!< 16-bit advertised service UUID (e.g. 0xFFF6) */
    uint16_t mtu_size;            /*!< Preferred ATT MTU in bytes */
} hal_ble_config_t;

/**
 * @class IHal_Cpx_Ble
 * @brief Specialization of IHAL_CPX providing BLE peripheral (GAP/GATT) control
 *        for Matter commissioning transports.
 *
 * Implementations must be safe for concurrent use from multiple tasks.
 */
class IHal_Cpx_Ble : public IHAL_CPX
{
public:
    /**
     * @brief Apply the BLE peripheral configuration.
     *
     * @param[in] config  Configuration parameters, must be valid.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t setConfig(const hal_ble_config_t& config) = 0;

    /**
     * @brief Register the asynchronous event callback.
     *
     * @param[in] callback  Callback function pointer, nullptr unregisters.
     * @param[in] userData  Opaque user context passed back to the callback.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t setEventCallback(hal_ble_event_cb_t callback, void* userData = nullptr) = 0;

    /**
     * @brief Start GAP advertising carrying the supplied raw advertising payload.
     *
     * @param[in] adv_payload  Raw advertising data bytes (may be nullptr when payload_len is 0).
     * @param[in] payload_len  Payload length in bytes (0..31 for legacy advertising).
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t startAdvertising(const uint8_t* adv_payload, size_t payload_len) = 0;

    /**
     * @brief Stop GAP advertising if active.
     *
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t stopAdvertising() = 0;

    /**
     * @brief Send an indication on the Matter BTP characteristic.
     *
     * @param[in] conn_handle  Target connection handle.
     * @param[in] data         Payload bytes, must not be nullptr when len > 0.
     * @param[in] len          Payload length in bytes (must fit negotiated MTU).
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t sendIndication(uint16_t conn_handle, const uint8_t* data, size_t len) = 0;

    /**
     * @brief Terminate an active BLE connection.
     *
     * @param[in] conn_handle  Connection handle to terminate.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t disconnect(uint16_t conn_handle) = 0;

    /**
     * @brief Check whether at least one BLE connection is active.
     *
     * @return true if a central is connected.
     * @return false otherwise.
     */
    virtual bool isConnected() = 0;

    virtual ~IHal_Cpx_Ble() = default;
};
