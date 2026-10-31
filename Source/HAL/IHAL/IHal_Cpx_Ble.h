/** @file       IHal_Cpx_Ble.h
 *  @brief      Platform-independent Bluetooth Low Energy GAP and GATT contract.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal.h"

#include <cstddef>
#include <cstdint>

/** CONSTANTS *****************************************************************/

constexpr std::size_t   HAL_BLE_ADDRESS_LENGTH             = 6U;
constexpr std::size_t   HAL_BLE_LEGACY_ADV_DATA_MAX_LENGTH = 31U;
constexpr std::size_t   HAL_BLE_DEVICE_NAME_MAX_LENGTH     = 32U;
constexpr std::uint16_t HAL_BLE_INVALID_CONNECTION_ID      = 0xFFFFU;

/** TYPEDEFS ******************************************************************/

/**
 * @brief Bluetooth UUID representation independent of the platform stack.
 *
 * UUID128 bytes use the canonical Bluetooth byte order (least-significant
 * octet first, matching the on-air/GATT representation).
 */
enum class hal_ble_uuid_type_t : std::uint8_t
{
    UUID16 = 0,
    UUID32,
    UUID128
};

struct hal_ble_uuid_t
{
    hal_ble_uuid_type_t type;
    union
    {
        std::uint16_t uuid16;
        std::uint32_t uuid32;
        std::uint8_t  uuid128[16];
    } value;
};

enum class hal_ble_address_type_t : std::uint8_t
{
    PUBLIC = 0,
    RANDOM,
    PUBLIC_IDENTITY,
    RANDOM_IDENTITY,
    UNKNOWN = 0xFF
};

struct hal_ble_address_t
{
    hal_ble_address_type_t type;
    std::uint8_t           bytes[HAL_BLE_ADDRESS_LENGTH];
};

enum class hal_ble_role_t : std::uint8_t
{
    UNKNOWN    = 0x00,
    CENTRAL    = 0x01,
    PERIPHERAL = 0x02
};

enum class hal_ble_event_type_t : std::uint8_t
{
    HOST_READY = 0,
    HOST_RESET,
    SCAN_RESULT,
    SCAN_COMPLETE,
    ADVERTISING_STOPPED,
    CONNECTED,
    DISCONNECTED,
    MTU_CHANGED,
    SECURITY_CHANGED,
    PASSKEY_ACTION,
    PAIRING_COMPLETE,
    GATT_SERVICE_DISCOVERED,
    GATT_SERVICE_DISCOVERY_COMPLETE,
    GATT_CHARACTERISTIC_DISCOVERED,
    GATT_CHARACTERISTIC_DISCOVERY_COMPLETE,
    GATT_DESCRIPTOR_DISCOVERED,
    GATT_DESCRIPTOR_DISCOVERY_COMPLETE,
    GATT_CLIENT_READ_COMPLETE,
    GATT_CLIENT_WRITE_COMPLETE,
    GATT_CLIENT_SUBSCRIPTION_COMPLETE,
    GATT_NOTIFICATION_RECEIVED,
    GATT_INDICATION_RECEIVED,
    GATT_NOTIFICATION_SENT,
    GATT_SERVER_SUBSCRIPTION_CHANGED,
    GATT_INDICATION_CONFIRMED,
    OPERATION_FAILED
};

enum class hal_ble_gatt_service_type_t : std::uint8_t
{
    PRIMARY = 0,
    SECONDARY
};

enum class hal_ble_gatt_write_type_t : std::uint8_t
{
    WITH_RESPONSE = 0,
    WITHOUT_RESPONSE
};

enum class hal_ble_io_capability_t : std::uint8_t
{
    DISPLAY_ONLY     = 0,
    DISPLAY_YES_NO   = 1,
    KEYBOARD_ONLY    = 2,
    NONE             = 3,
    KEYBOARD_DISPLAY = 4
};

enum class hal_ble_event_operation_t : std::uint8_t
{
    NONE = 0,
    SCAN,
    ADVERTISING,
    CONNECT,
    DISCONNECT,
    SERVICE_DISCOVERY,
    CHARACTERISTIC_DISCOVERY,
    READ,
    WRITE,
    SUBSCRIBE,
    NOTIFY,
    INDICATE,
    SECURITY
};

enum class hal_ble_gatt_status_t : std::uint8_t
{
    SUCCESS                     = 0x00,
    INVALID_HANDLE              = 0x01,
    READ_NOT_PERMITTED          = 0x02,
    WRITE_NOT_PERMITTED         = 0x03,
    INVALID_PDU                 = 0x04,
    INSUFFICIENT_AUTHENTICATION = 0x05,
    REQUEST_NOT_SUPPORTED       = 0x06,
    INVALID_OFFSET              = 0x07,
    INSUFFICIENT_AUTHORIZATION  = 0x08,
    PREPARE_QUEUE_FULL          = 0x09,
    ATTRIBUTE_NOT_FOUND         = 0x0A,
    ATTRIBUTE_NOT_LONG          = 0x0B,
    INSUFFICIENT_KEY_SIZE       = 0x0C,
    INVALID_ATTRIBUTE_LENGTH    = 0x0D,
    UNLIKELY_ERROR              = 0x0E,
    INSUFFICIENT_ENCRYPTION     = 0x0F,
    UNSUPPORTED_GROUP_TYPE      = 0x10,
    INSUFFICIENT_RESOURCES      = 0x11,
    APPLICATION_ERROR           = 0x80
};

enum hal_ble_gatt_property_t : std::uint16_t
{
    HAL_BLE_GATT_PROPERTY_READ                       = 1U << 0U,
    HAL_BLE_GATT_PROPERTY_WRITE                      = 1U << 1U,
    HAL_BLE_GATT_PROPERTY_WRITE_WITHOUT_RSP          = 1U << 2U,
    HAL_BLE_GATT_PROPERTY_NOTIFY                     = 1U << 3U,
    HAL_BLE_GATT_PROPERTY_INDICATE                   = 1U << 4U,
    HAL_BLE_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE = 1U << 5U
};

enum hal_ble_gatt_permission_t : std::uint16_t
{
    HAL_BLE_GATT_PERMISSION_READ                = 1U << 0U,
    HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED      = 1U << 1U,
    HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED  = 1U << 2U,
    HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED     = 1U << 3U,
    HAL_BLE_GATT_PERMISSION_WRITE               = 1U << 4U,
    HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED     = 1U << 5U,
    HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED = 1U << 6U,
    HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED    = 1U << 7U
};

enum hal_ble_capability_t : std::uint32_t
{
    HAL_BLE_CAP_CENTRAL              = 1U << 0U,
    HAL_BLE_CAP_PERIPHERAL           = 1U << 1U,
    HAL_BLE_CAP_GATT_CLIENT          = 1U << 2U,
    HAL_BLE_CAP_GATT_SERVER          = 1U << 3U,
    HAL_BLE_CAP_BONDING              = 1U << 4U,
    HAL_BLE_CAP_SECURE_CONNECTIONS   = 1U << 5U,
    HAL_BLE_CAP_EXTENDED_ADVERTISING = 1U << 6U,
    HAL_BLE_CAP_MULTIPLE_CONNECTIONS = 1U << 7U
};

enum class hal_ble_pairing_action_t : std::uint8_t
{
    NONE = 0,
    DISPLAY_PASSKEY,
    INPUT_PASSKEY,
    NUMERIC_COMPARISON,
    OOB_DATA
};

using hal_ble_connection_id_t = std::uint16_t;
using hal_ble_request_id_t    = std::uint32_t;

struct hal_ble_security_config_t
{
    bool                    enabled;
    bool                    bonding;
    bool                    mitmProtection;
    bool                    secureConnections;
    hal_ble_io_capability_t ioCapability;
    std::uint8_t            minimumKeySize;
};

struct hal_ble_connection_params_t
{
    std::uint16_t minIntervalMs;
    std::uint16_t maxIntervalMs;
    std::uint16_t latency;
    std::uint16_t supervisionTimeoutMs;
};

struct hal_ble_config_t
{
    char                        deviceName[HAL_BLE_DEVICE_NAME_MAX_LENGTH];
    std::uint8_t                roleMask;
    std::uint8_t                maxConnections;
    std::uint16_t               preferredMtu;
    hal_ble_connection_params_t connectionParams;
    hal_ble_security_config_t   security;
};

struct hal_ble_capabilities_t
{
    std::uint32_t flags;
    std::uint8_t  maxConnections;
    std::uint8_t  maxServices;
    std::uint8_t  maxCharacteristics;
    std::uint8_t  maxDescriptors;
    std::uint16_t maxMtu;
    std::uint16_t maxAdvertisingDataLength;
};

struct hal_ble_scan_config_t
{
    bool          active;
    bool          filterDuplicates;
    std::uint16_t intervalMs;
    std::uint16_t windowMs;
    std::uint32_t durationMs;
};

struct hal_ble_advertising_config_t
{
    bool                connectable;
    bool                scannable;
    std::uint16_t       intervalMinMs;
    std::uint16_t       intervalMaxMs;
    std::uint32_t       durationMs;
    const std::uint8_t* advertisingData;
    std::size_t         advertisingDataLength;
    const std::uint8_t* scanResponseData;
    std::size_t         scanResponseDataLength;
};

struct hal_ble_gatt_characteristic_t;
struct hal_ble_gatt_service_t;

/**
 * @brief Synchronous GATT server read callback.
 *
 * Called in BLE host context. The callback must be bounded, non-blocking, and
 * must not re-enter the same BLE driver. The output buffer is owned by the
 * driver and valid only for this call.
 */
using hal_ble_gatt_read_cb_t = hal_ble_gatt_status_t (*)(
    hal_ble_connection_id_t connectionId, std::uint16_t offset, std::uint8_t* output, std::size_t outputCapacity, std::size_t& outputLength, void* userData);

/**
 * @brief Synchronous GATT server write callback.
 *
 * The input data is owned by the BLE host and valid only for this call. The
 * callback must be bounded, non-blocking, and must not re-enter the driver.
 */
using hal_ble_gatt_write_cb_t =
    hal_ble_gatt_status_t (*)(hal_ble_connection_id_t connectionId, std::uint16_t offset, const std::uint8_t* data, std::size_t dataLength, void* userData);

struct hal_ble_gatt_descriptor_t
{
    hal_ble_uuid_t          uuid;
    std::uint16_t           permissions;
    std::uint16_t           maximumValueLength;
    hal_ble_gatt_read_cb_t  readCallback;
    hal_ble_gatt_write_cb_t writeCallback;
    void*                   userData;
};

struct hal_ble_gatt_characteristic_t
{
    hal_ble_uuid_t                   uuid;
    std::uint16_t                    properties;
    std::uint16_t                    permissions;
    std::uint16_t                    maximumValueLength;
    hal_ble_gatt_read_cb_t           readCallback;
    hal_ble_gatt_write_cb_t          writeCallback;
    void*                            userData;
    const hal_ble_gatt_descriptor_t* descriptors;
    std::size_t                      descriptorCount;
};

struct hal_ble_gatt_service_t
{
    hal_ble_uuid_t                       uuid;
    hal_ble_gatt_service_type_t          type;
    const hal_ble_gatt_characteristic_t* characteristics;
    std::size_t                          characteristicCount;
};

/**
 * @brief Connection and GATT event data.
 *
 * Only fields documented for the corresponding event type are valid. Any
 * payload pointer is valid only for the duration of the event callback.
 */
struct hal_ble_event_data_t
{
    hal_ble_connection_id_t   connectionId;
    hal_ble_role_t            role;
    hal_ble_address_t         address;
    hal_ble_uuid_t            uuid;
    hal_ble_request_id_t      requestId;
    hal_ble_event_operation_t operation;
    std::uint16_t             attributeHandle;
    std::uint16_t             properties;
    std::uint16_t             startHandle;
    std::uint16_t             endHandle;
    std::uint16_t             mtu;
    std::int32_t              disconnectReason;
    std::int8_t               rssi;
    std::uint8_t              advertisementType;
    std::uint8_t              advertisementFlags;
    bool                      encrypted;
    bool                      authenticated;
    bool                      bonded;
    bool                      notificationsEnabled;
    bool                      indicationsEnabled;
    bool                      isIndication;
    hal_ble_pairing_action_t  pairingAction;
    std::uint32_t             passkey;
    sys_error_t               status;
    hal_ble_gatt_status_t     gattStatus;
    const std::uint8_t*       data;
    std::size_t               dataLength;
};

/**
 * @brief BLE event callback.
 *
 * Invoked asynchronously from the platform BLE host context. Handlers must be
 * short and non-blocking; do not call the same HAL instance from this callback.
 */
using hal_ble_event_cb_t = void (*)(hal_ble_event_type_t event, const hal_ble_event_data_t& eventData, void* userData);

/** CLASSES *******************************************************************/

/**
 * @class IHal_Cpx_Ble
 * @brief Platform-independent Bluetooth Low Energy GAP and GATT interface.
 *
 * Provides central and peripheral operations without binding callers to a
 * vendor stack or application protocol. Implementations report unsupported
 * capabilities through getCapabilities() and return ERROR_NOT_SUPPORTED for
 * unsupported operations.
 *
 * @note Thread-Safety: Operational methods may be called from application
 *       tasks. Event and GATT access callbacks run in BLE host context and
 *       must not block or re-enter this interface.
 */
class IHal_Cpx_Ble : public IHAL_CPX
{
public:
    /**
     * @brief Configure BLE roles, limits, preferred MTU, and security policy.
     *
     * Must be called before init(). The configuration is copied by value.
     *
     * @param[in] config BLE configuration.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t setConfig(const hal_ble_config_t& config) = 0;

    /**
     * @brief Register the asynchronous event callback.
     *
     * @param[in] callback Callback to register; nullptr unregisters it.
     * @param[in] userData Opaque context passed to the callback.
     * @return ERROR_SUCCESS on success, otherwise an error code. If unregister
     *         returns an error, keep the callback context alive and retry.
     */
    virtual sys_error_t setEventCallback(hal_ble_event_cb_t callback, void* userData = nullptr) = 0;

    /**
     * @brief Get supported BLE capabilities and resource limits.
     *
     * @param[out] capabilities Supported capability flags and implementation limits.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t getCapabilities(hal_ble_capabilities_t& capabilities) const = 0;

    /**
     * @brief Start a GAP scan.
     *
     * Results and completion are delivered through the event callback.
     * @param[in] config Scan parameters.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t startScan(const hal_ble_scan_config_t& config) = 0;

    /**
     * @brief Stop an active GAP scan.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t stopScan() = 0;

    /**
     * @brief Start GAP advertising with caller-encoded AD structures.
     *
     * The implementation copies both payloads before returning. Payloads are
     * limited by the implementation's reported advertising capabilities.
     * @param[in] config Advertising parameters and payloads.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t startAdvertising(const hal_ble_advertising_config_t& config) = 0;

    /**
     * @brief Stop active GAP advertising.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t stopAdvertising() = 0;

    /**
     * @brief Connect as a central to a peer.
     *
     * @param[in] address Peer address.
     * @param[in] timeoutMs Connection timeout; zero selects the platform default.
     * @return ERROR_SUCCESS when the request is accepted, otherwise an error.
     */
    virtual sys_error_t connect(const hal_ble_address_t& address, std::uint32_t timeoutMs = 0U) = 0;

    /**
     * @brief Disconnect an active connection.
     * @param[in] connectionId Opaque connection identifier.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t disconnect(hal_ble_connection_id_t connectionId) = 0;

    /**
     * @brief Request link security for an active connection.
     * @param[in] connectionId Opaque connection identifier.
     * @return ERROR_SUCCESS when the request is accepted, otherwise an error.
     */
    virtual sys_error_t secureConnection(hal_ble_connection_id_t connectionId) = 0;

    /**
     * @brief Accept or reject an asynchronous pairing action.
     *
     * For INPUT_PASSKEY, passkey must be in the range 0..999999. For other
     * actions it is ignored. Call from application task context, not from the
     * BLE event callback.
     * @param[in] connectionId Connection awaiting the pairing response.
     * @param[in] accept Whether to accept the requested pairing action.
     * @param[in] passkey Passkey to submit for INPUT_PASSKEY.
     * @return ERROR_SUCCESS on success, otherwise an error status.
     */
    virtual sys_error_t respondToPairing(hal_ble_connection_id_t connectionId, bool accept, std::uint32_t passkey = 0U) = 0;

    /**
     * @brief Register GATT server services before start().
     *
     * Service, characteristic, descriptor, and callback user-data storage must
     * remain valid until deInit(). Implementations must reject invalid or
     * unsupported definitions.
     * @param[in] services Service definitions.
     * @param[in] serviceCount Number of definitions.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t registerGattServices(const hal_ble_gatt_service_t* services, std::size_t serviceCount) = 0;

    /**
     * @brief Retrieve the local value handle assigned to a registered
     *        characteristic.
     *
     * @param[in] serviceIndex Service index passed to registerGattServices().
     * @param[in] characteristicIndex Characteristic index within that service.
     * @param[out] valueHandle Assigned GATT value handle.
     * @return ERROR_SUCCESS when the host has registered the attribute,
     *         otherwise an error status.
     */
    virtual sys_error_t getLocalCharacteristicHandle(std::size_t serviceIndex, std::size_t characteristicIndex, std::uint16_t& valueHandle) const = 0;

    /**
     * @brief Discover services on a central connection.
     *
     * Pass nullptr to discover all services. Results and completion are
     * delivered through the event callback.
     * @param[in] connectionId Active central connection.
     * @param[out] requestId Identifier associated with discovery events.
     * @param[in] uuidFilter Optional service UUID filter.
     * @return ERROR_SUCCESS when accepted, otherwise an error status.
     */
    virtual sys_error_t discoverServices(hal_ble_connection_id_t connectionId, hal_ble_request_id_t& requestId, const hal_ble_uuid_t* uuidFilter = nullptr) = 0;

    /**
     * @brief Discover characteristics in a remote service handle range.
     *
     * @param[in] connectionId Active central connection.
     * @param[in] startHandle First handle in the service range.
     * @param[in] endHandle Last handle in the service range.
     * @param[out] requestId Identifier associated with discovery events.
     * @param[in] uuidFilter Optional characteristic UUID filter.
     * @return ERROR_SUCCESS when accepted, otherwise an error status.
     */
    virtual sys_error_t discoverCharacteristics(
        hal_ble_connection_id_t connectionId, std::uint16_t startHandle, std::uint16_t endHandle, hal_ble_request_id_t& requestId,
        const hal_ble_uuid_t* uuidFilter = nullptr) = 0;

    /**
     * @brief Discover descriptors in a remote GATT handle range.
     *
     * @param[in] connectionId Active central connection.
     * @param[in] startHandle First handle in the descriptor range.
     * @param[in] endHandle Last handle in the descriptor range.
     * @param[out] requestId Identifier associated with discovery events.
     * @return ERROR_SUCCESS when accepted, otherwise an error status.
     */
    virtual sys_error_t discoverDescriptors(hal_ble_connection_id_t connectionId, std::uint16_t startHandle, std::uint16_t endHandle, hal_ble_request_id_t& requestId) = 0;

    /**
     * @brief Read a remote GATT characteristic.
     *
     * Completion is delivered as GATT_CLIENT_READ_COMPLETE with requestId.
     * @param[in] connectionId Active central connection.
     * @param[in] valueHandle Remote characteristic value handle.
     * @param[out] requestId Identifier associated with the read event.
     * @return ERROR_SUCCESS when accepted, otherwise an error status.
     */
    virtual sys_error_t readCharacteristic(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, hal_ble_request_id_t& requestId) = 0;

    /**
     * @brief Write a remote GATT characteristic.
     *
     * The implementation copies the input before returning. Completion for a
     * write with response is delivered through the event callback.
     * @param[in] connectionId Active central connection.
     * @param[in] valueHandle Remote characteristic value handle.
     * @param[in] data Value bytes.
     * @param[in] dataLength Value length.
     * @param[in] writeType Write procedure type.
     * @param[out] requestId Identifier associated with the write event; zero for
     *             a write without response.
     * @return ERROR_SUCCESS when accepted, otherwise an error status.
     */
    virtual sys_error_t writeCharacteristic(
        hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength, hal_ble_gatt_write_type_t writeType,
        hal_ble_request_id_t& requestId) = 0;

    /**
     * @brief Configure notifications or indications for a remote characteristic.
     *
     * @param[in] connectionId Active central connection.
     * @param[in] valueHandle Remote characteristic value handle.
     * @param[in] cccdHandle Client Configuration Descriptor handle.
     * @param[in] notifications Enable notifications.
     * @param[in] indications Enable indications.
     * @param[out] requestId Identifier associated with the subscription event.
     * @return ERROR_SUCCESS when accepted, otherwise an error status.
     */
    virtual sys_error_t subscribe(
        hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, std::uint16_t cccdHandle, bool notifications, bool indications, hal_ble_request_id_t& requestId) = 0;

    /**
     * @brief Send a GATT notification from the local server.
     *
     * @param[in] connectionId Target connection.
     * @param[in] valueHandle Local characteristic value handle.
     * @param[in] data Value bytes.
     * @param[in] dataLength Value length.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t sendNotification(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength) = 0;

    /**
     * @brief Send a GATT indication from the local server.
     *
     * Completion is delivered as GATT_INDICATION_CONFIRMED.
     * @param[in] connectionId Target connection.
     * @param[in] valueHandle Local characteristic value handle.
     * @param[in] data Value bytes.
     * @param[in] dataLength Value length.
     * @return ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t sendIndication(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength) = 0;

    /**
     * @brief Check whether the implementation is initialized.
     *
     * @return true when initialized; otherwise false.
     */
    virtual bool isInitialized() const = 0;

    virtual ~IHal_Cpx_Ble() = default;
};
