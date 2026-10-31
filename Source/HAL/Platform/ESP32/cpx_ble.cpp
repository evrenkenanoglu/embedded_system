/** @file       cpx_ble.cpp
 *  @brief      ESP32 NimBLE implementation of the generic BLE HAL.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

/** INCLUDES ******************************************************************/

#include "cpx_ble.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

#include <esp_err.h>
#include <esp_nimble_hci.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <host/ble_att.h>
#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_sm.h>
#include <host/ble_store.h>
#include <host/ble_uuid.h>
#include <host/util/util.h>
#include <nimble/ble.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <os/os_mbuf.h>
#include <sdkconfig.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <store/config/ble_store_config.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

/** CONSTANTS *****************************************************************/

namespace
{
    constexpr std::size_t   kMaxGattServices               = 8U;
    constexpr std::size_t   kMaxGattCharacteristics        = 32U;
    constexpr std::size_t   kMaxGattDescriptors            = 32U;
    constexpr std::size_t   kMaxPendingClientOperations    = 8U;
    constexpr std::size_t   kMaxSecurityConnections        = 8U;
    constexpr std::size_t   kMaxLegacyAdvertisingData      = HAL_BLE_LEGACY_ADV_DATA_MAX_LENGTH;
    constexpr std::size_t   kMaxAttributeLength            = BLE_ATT_ATTR_MAX_LEN;
    constexpr std::uint32_t kDefaultConnectTimeoutMs       = 30000U;
    constexpr std::uint16_t kDefaultMtu                    = 23U;
    constexpr std::uint8_t  kInvalidOwnAddressType         = 0xFFU;
    constexpr std::uint8_t  kEmptyPayload                  = 0U;
    constexpr std::uint16_t kValidCharacteristicProperties = HAL_BLE_GATT_PROPERTY_READ | HAL_BLE_GATT_PROPERTY_WRITE | HAL_BLE_GATT_PROPERTY_WRITE_WITHOUT_RSP |
                                                             HAL_BLE_GATT_PROPERTY_NOTIFY | HAL_BLE_GATT_PROPERTY_INDICATE | HAL_BLE_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE;
    constexpr std::uint16_t kValidGattPermissions = HAL_BLE_GATT_PERMISSION_READ | HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED | HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED |
                                                    HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED | HAL_BLE_GATT_PERMISSION_WRITE | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                                    HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED;

    /** TYPEDEFS ******************************************************************/

    enum class client_operation_type_t : std::uint8_t
    {
        SERVICE_DISCOVERY = 0,
        CHARACTERISTIC_DISCOVERY,
        DESCRIPTOR_DISCOVERY,
        READ,
        WRITE,
        SUBSCRIBE
    };

    union nimble_uuid_storage_t
    {
        ble_uuid16_t  uuid16;
        ble_uuid32_t  uuid32;
        ble_uuid128_t uuid128;
    };

    struct gatt_attribute_binding_t
    {
        const hal_ble_gatt_characteristic_t* characteristic;
        const hal_ble_gatt_descriptor_t*     descriptor;
    };

    struct gatt_storage_t
    {
        std::array<ble_gatt_svc_def, kMaxGattServices + 1U>                                                 services{};
        std::array<ble_gatt_chr_def, kMaxGattCharacteristics + kMaxGattServices>                            characteristics{};
        std::array<ble_gatt_dsc_def, kMaxGattDescriptors + kMaxGattCharacteristics>                         descriptors{};
        std::array<std::uint16_t, kMaxGattCharacteristics>                                                  valueHandles{};
        std::array<gatt_attribute_binding_t, kMaxGattCharacteristics + kMaxGattDescriptors>                 bindings{};
        std::array<nimble_uuid_storage_t, kMaxGattServices + kMaxGattCharacteristics + kMaxGattDescriptors> uuids{};
        std::size_t                                                                                         uuidCount{0U};
        std::size_t                                                                                         characteristicCount{0U};
        std::size_t                                                                                         descriptorCount{0U};
        std::size_t                                                                                         bindingCount{0U};
        std::size_t                                                                                         serviceCount{0U};
        std::array<std::size_t, kMaxGattServices>                                                           serviceCharacteristicOffsets{};
        std::array<std::size_t, kMaxGattServices>                                                           serviceCharacteristicCounts{};
    };

    struct client_operation_t
    {
        bool                    active{false};
        cpx_ble*                owner{nullptr};
        client_operation_type_t type{client_operation_type_t::SERVICE_DISCOVERY};
        hal_ble_connection_id_t connectionId{HAL_BLE_INVALID_CONNECTION_ID};
        hal_ble_request_id_t    requestId{0U};
        bool                    hasUuidFilter{false};
        hal_ble_uuid_type_t     uuidFilterType{hal_ble_uuid_type_t::UUID16};
        nimble_uuid_storage_t   uuidFilter{};
    };

    struct pending_pairing_t
    {
        bool                     active{false};
        hal_ble_connection_id_t  connectionId{HAL_BLE_INVALID_CONNECTION_ID};
        hal_ble_pairing_action_t action{hal_ble_pairing_action_t::NONE};
    };

    /** VARIABLES *****************************************************************/

    gatt_storage_t                                               s_gattStorage;
    std::array<client_operation_t, kMaxPendingClientOperations>  s_clientOperations;
    std::array<pending_pairing_t, kMaxSecurityConnections>       s_pendingPairings;
    std::array<hal_ble_connection_id_t, kMaxSecurityConnections> s_activeConnections{
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID,
        HAL_BLE_INVALID_CONNECTION_ID};
    portMUX_TYPE              s_bleStateMux           = portMUX_INITIALIZER_UNLOCKED;
    cpx_ble*                  s_instance              = nullptr;
    bool                      s_hciInitialized        = false;
    bool                      s_nimblePortInitialized = false;
    bool                      s_nimbleLifecycleUsed   = false;
    std::atomic<std::uint8_t> s_ownAddressType{kInvalidOwnAddressType};

    /** LOCAL FUNCTIONS ***********************************************************/

    /**
     * @brief Translate a NimBLE host status into the repository error model.
     */
    sys_error_t translateNimbleError(int status)
    {
        if (status == 0)
        {
            return ERROR_SUCCESS;
        }

        switch (status)
        {
            case BLE_HS_EINVAL:
                return ERROR_INVALID_ARG;
            case BLE_HS_EALREADY:
            case BLE_HS_EBUSY:
                return ERROR_DEVICE_BUSY;
            case BLE_HS_ENOMEM:
                return ERROR_OUT_OF_MEMORY;
            case BLE_HS_ETIMEOUT:
                return ERROR_TIMEOUT;
            case BLE_HS_EDONE:
                return ERROR_SUCCESS;
            case BLE_HS_ENOTCONN:
                return ERROR_CONNECTION_CLOSED;
            case BLE_HS_EAGAIN:
                return ERROR_BUSY;
            default:
                return ERROR_FAIL;
        }
    }

    std::uint16_t millisecondsTo625MicrosecondUnits(std::uint16_t milliseconds)
    {
        return static_cast<std::uint16_t>((static_cast<std::uint32_t>(milliseconds) * 8U + 4U) / 5U);
    }

    std::uint16_t millisecondsTo1250MicrosecondUnits(std::uint16_t milliseconds)
    {
        return static_cast<std::uint16_t>((static_cast<std::uint32_t>(milliseconds) * 4U + 4U) / 5U);
    }

    std::uint16_t millisecondsTo10MillisecondUnits(std::uint16_t milliseconds)
    {
        return static_cast<std::uint16_t>((static_cast<std::uint32_t>(milliseconds) + 9U) / 10U);
    }

    bool isValidUuidType(hal_ble_uuid_type_t type)
    {
        return type == hal_ble_uuid_type_t::UUID16 || type == hal_ble_uuid_type_t::UUID32 || type == hal_ble_uuid_type_t::UUID128;
    }

    hal_ble_address_type_t convertAddressType(std::uint8_t addressType)
    {
        switch (addressType)
        {
            case BLE_ADDR_PUBLIC:
                return hal_ble_address_type_t::PUBLIC;
            case BLE_ADDR_RANDOM:
                return hal_ble_address_type_t::RANDOM;
            case BLE_ADDR_PUBLIC_ID:
                return hal_ble_address_type_t::PUBLIC_IDENTITY;
            case BLE_ADDR_RANDOM_ID:
                return hal_ble_address_type_t::RANDOM_IDENTITY;
            default:
                return hal_ble_address_type_t::UNKNOWN;
        }
    }

    const ble_uuid_t* convertUuid(nimble_uuid_storage_t& storage, const hal_ble_uuid_t& uuid)
    {
        switch (uuid.type)
        {
            case hal_ble_uuid_type_t::UUID16:
                storage.uuid16.u.type = BLE_UUID_TYPE_16;
                storage.uuid16.value  = uuid.value.uuid16;
                return &storage.uuid16.u;
            case hal_ble_uuid_type_t::UUID32:
                storage.uuid32.u.type = BLE_UUID_TYPE_32;
                storage.uuid32.value  = uuid.value.uuid32;
                return &storage.uuid32.u;
            case hal_ble_uuid_type_t::UUID128:
                storage.uuid128.u.type = BLE_UUID_TYPE_128;
                std::memcpy(storage.uuid128.value, uuid.value.uuid128, sizeof(storage.uuid128.value));
                return &storage.uuid128.u;
            default:
                return nullptr;
        }
    }

    const ble_uuid_t* storedUuid(const nimble_uuid_storage_t& storage, hal_ble_uuid_type_t type)
    {
        switch (type)
        {
            case hal_ble_uuid_type_t::UUID16:
                return &storage.uuid16.u;
            case hal_ble_uuid_type_t::UUID32:
                return &storage.uuid32.u;
            case hal_ble_uuid_type_t::UUID128:
                return &storage.uuid128.u;
            default:
                return nullptr;
        }
    }

    void convertUuid(const ble_uuid_any_t& source, hal_ble_uuid_t& destination)
    {
        switch (source.u.type)
        {
            case BLE_UUID_TYPE_16:
                destination.type         = hal_ble_uuid_type_t::UUID16;
                destination.value.uuid16 = source.u16.value;
                break;
            case BLE_UUID_TYPE_32:
                destination.type         = hal_ble_uuid_type_t::UUID32;
                destination.value.uuid32 = source.u32.value;
                break;
            case BLE_UUID_TYPE_128:
                destination.type = hal_ble_uuid_type_t::UUID128;
                std::memcpy(destination.value.uuid128, source.u128.value, sizeof(destination.value.uuid128));
                break;
            default:
                destination.type = static_cast<hal_ble_uuid_type_t>(0xFFU);
                break;
        }
    }

    nimble_uuid_storage_t* allocateUuid(const hal_ble_uuid_t& uuid)
    {
        if (s_gattStorage.uuidCount >= s_gattStorage.uuids.size())
        {
            return nullptr;
        }

        nimble_uuid_storage_t* const storage = &s_gattStorage.uuids[s_gattStorage.uuidCount++];
        if (convertUuid(*storage, uuid) == nullptr)
        {
            --s_gattStorage.uuidCount;
            return nullptr;
        }
        return storage;
    }

    std::uint16_t characteristicFlags(const hal_ble_gatt_characteristic_t& characteristic)
    {
        std::uint16_t       flags       = 0U;
        const std::uint16_t properties  = characteristic.properties;
        const std::uint16_t permissions = characteristic.permissions;

        if ((properties & HAL_BLE_GATT_PROPERTY_READ) != 0U)
        {
            flags |= BLE_GATT_CHR_F_READ;
        }
        if ((properties & HAL_BLE_GATT_PROPERTY_WRITE) != 0U)
        {
            flags |= BLE_GATT_CHR_F_WRITE;
        }
        if ((properties & HAL_BLE_GATT_PROPERTY_WRITE_WITHOUT_RSP) != 0U)
        {
            flags |= BLE_GATT_CHR_F_WRITE_NO_RSP;
        }
        if ((properties & HAL_BLE_GATT_PROPERTY_NOTIFY) != 0U)
        {
            flags |= BLE_GATT_CHR_F_NOTIFY;
        }
        if ((properties & HAL_BLE_GATT_PROPERTY_INDICATE) != 0U)
        {
            flags |= BLE_GATT_CHR_F_INDICATE;
        }
        if ((properties & HAL_BLE_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE) != 0U)
        {
            flags |= BLE_GATT_CHR_F_AUTH_SIGN_WRITE;
        }
        if ((permissions & HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED) != 0U)
        {
            flags |= BLE_GATT_CHR_F_READ_ENC;
        }
        if ((permissions & HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED) != 0U)
        {
            flags |= BLE_GATT_CHR_F_READ_AUTHEN;
        }
        if ((permissions & HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED) != 0U)
        {
            flags |= BLE_GATT_CHR_F_READ_AUTHOR;
        }
        if ((permissions & HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED) != 0U)
        {
            flags |= BLE_GATT_CHR_F_WRITE_ENC;
        }
        if ((permissions & HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED) != 0U)
        {
            flags |= BLE_GATT_CHR_F_WRITE_AUTHEN;
        }
        if ((permissions & HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED) != 0U)
        {
            flags |= BLE_GATT_CHR_F_WRITE_AUTHOR;
        }

        return flags;
    }

    std::uint8_t descriptorFlags(const hal_ble_gatt_descriptor_t& descriptor)
    {
        std::uint8_t flags = 0U;
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_READ) != 0U)
        {
            flags |= BLE_ATT_F_READ;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED) != 0U)
        {
            flags |= BLE_ATT_F_READ_ENC;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED) != 0U)
        {
            flags |= BLE_ATT_F_READ_AUTHEN;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED) != 0U)
        {
            flags |= BLE_ATT_F_READ_AUTHOR;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_WRITE) != 0U)
        {
            flags |= BLE_ATT_F_WRITE;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED) != 0U)
        {
            flags |= BLE_ATT_F_WRITE_ENC;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED) != 0U)
        {
            flags |= BLE_ATT_F_WRITE_AUTHEN;
        }
        if ((descriptor.permissions & HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED) != 0U)
        {
            flags |= BLE_ATT_F_WRITE_AUTHOR;
        }
        return flags;
    }

    client_operation_t* allocateClientOperation(cpx_ble* owner, client_operation_type_t type, hal_ble_connection_id_t connectionId, hal_ble_request_id_t requestId)
    {
        portENTER_CRITICAL(&s_bleStateMux);
        for (client_operation_t& operation : s_clientOperations)
        {
            if (!operation.active)
            {
                operation.active         = true;
                operation.owner          = owner;
                operation.type           = type;
                operation.connectionId   = connectionId;
                operation.requestId      = requestId;
                operation.hasUuidFilter  = false;
                operation.uuidFilterType = hal_ble_uuid_type_t::UUID16;
                portEXIT_CRITICAL(&s_bleStateMux);
                return &operation;
            }
        }
        portEXIT_CRITICAL(&s_bleStateMux);
        return nullptr;
    }

    void releaseClientOperation(client_operation_t* operation)
    {
        if (operation == nullptr)
        {
            return;
        }

        portENTER_CRITICAL(&s_bleStateMux);
        operation->active         = false;
        operation->owner          = nullptr;
        operation->connectionId   = HAL_BLE_INVALID_CONNECTION_ID;
        operation->requestId      = 0U;
        operation->hasUuidFilter  = false;
        operation->uuidFilterType = hal_ble_uuid_type_t::UUID16;
        portEXIT_CRITICAL(&s_bleStateMux);
    }

    hal_ble_gatt_status_t translateGattStatus(int status)
    {
        if (status == 0)
        {
            return hal_ble_gatt_status_t::SUCCESS;
        }

        const int attStatus = status - BLE_HS_ERR_ATT_BASE;
        if (attStatus >= static_cast<int>(hal_ble_gatt_status_t::INVALID_HANDLE) && attStatus <= static_cast<int>(hal_ble_gatt_status_t::INSUFFICIENT_RESOURCES))
        {
            return static_cast<hal_ble_gatt_status_t>(attStatus);
        }
        return hal_ble_gatt_status_t::APPLICATION_ERROR;
    }

    void fillEventStatus(hal_ble_event_data_t& eventData, int status)
    {
        eventData.status     = translateNimbleError(status);
        eventData.gattStatus = translateGattStatus(status);
    }

    void addPendingPairing(hal_ble_connection_id_t connectionId, hal_ble_pairing_action_t action)
    {
        portENTER_CRITICAL(&s_bleStateMux);
        for (pending_pairing_t& pairing : s_pendingPairings)
        {
            if (!pairing.active || pairing.connectionId == connectionId)
            {
                pairing.active       = true;
                pairing.connectionId = connectionId;
                pairing.action       = action;
                break;
            }
        }
        portEXIT_CRITICAL(&s_bleStateMux);
    }

    bool getPendingPairing(hal_ble_connection_id_t connectionId, hal_ble_pairing_action_t& action)
    {
        bool found = false;
        portENTER_CRITICAL(&s_bleStateMux);
        for (const pending_pairing_t& pairing : s_pendingPairings)
        {
            if (pairing.active && pairing.connectionId == connectionId)
            {
                action = pairing.action;
                found  = true;
                break;
            }
        }
        portEXIT_CRITICAL(&s_bleStateMux);
        return found;
    }

    void clearPendingPairing(hal_ble_connection_id_t connectionId)
    {
        portENTER_CRITICAL(&s_bleStateMux);
        for (pending_pairing_t& pairing : s_pendingPairings)
        {
            if (pairing.active && pairing.connectionId == connectionId)
            {
                pairing.active       = false;
                pairing.connectionId = HAL_BLE_INVALID_CONNECTION_ID;
                pairing.action       = hal_ble_pairing_action_t::NONE;
            }
        }
        portEXIT_CRITICAL(&s_bleStateMux);
    }

    bool trackConnection(hal_ble_connection_id_t connectionId, std::uint8_t maxConnections)
    {
        bool        tracked     = false;
        std::size_t activeCount = 0U;
        portENTER_CRITICAL(&s_bleStateMux);
        for (const hal_ble_connection_id_t activeConnection : s_activeConnections)
        {
            if (activeConnection != HAL_BLE_INVALID_CONNECTION_ID)
            {
                ++activeCount;
            }
        }
        for (hal_ble_connection_id_t& activeConnection : s_activeConnections)
        {
            if (activeConnection == connectionId)
            {
                tracked = true;
                break;
            }
            if (activeConnection == HAL_BLE_INVALID_CONNECTION_ID && activeCount < maxConnections)
            {
                activeConnection = connectionId;
                tracked          = true;
                break;
            }
        }
        portEXIT_CRITICAL(&s_bleStateMux);
        return tracked;
    }

    void removeConnection(hal_ble_connection_id_t connectionId)
    {
        portENTER_CRITICAL(&s_bleStateMux);
        for (hal_ble_connection_id_t& activeConnection : s_activeConnections)
        {
            if (activeConnection == connectionId)
            {
                activeConnection = HAL_BLE_INVALID_CONNECTION_ID;
            }
        }
        portEXIT_CRITICAL(&s_bleStateMux);
    }
} // namespace

/** FUNCTIONS *****************************************************************/

cpx_ble::cpx_ble()
    : _config{}
    , _eventCallback(nullptr)
    , _eventUserData(nullptr)
    , _initialized(false)
    , _hostStarted(false)
    , _hostSynced(false)
    , _activeEventCallbacks(0U)
    , _scanState(procedure_state_t::IDLE)
    , _advertisingState(procedure_state_t::IDLE)
    , _servicesRegistered(false)
    , _nextRequestId(1U)
{
    std::memcpy(_config.deviceName, "BLE-Device", sizeof("BLE-Device"));
    _config.roleMask                              = static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL) | static_cast<std::uint8_t>(hal_ble_role_t::PERIPHERAL);
    _config.maxConnections                        = 3U;
    _config.preferredMtu                          = 247U;
    _config.connectionParams.minIntervalMs        = 30U;
    _config.connectionParams.maxIntervalMs        = 50U;
    _config.connectionParams.latency              = 0U;
    _config.connectionParams.supervisionTimeoutMs = 4000U;
    _config.security.enabled                      = false;
    _config.security.bonding                      = false;
    _config.security.mitmProtection               = false;
    _config.security.secureConnections            = false;
    _config.security.ioCapability                 = hal_ble_io_capability_t::NONE;
    _config.security.minimumKeySize               = 0U;
}

cpx_ble::~cpx_ble()
{
    const sys_error_t status = deInit();
    if (status != ERROR_SUCCESS)
    {
        SYS_LOG_E("cpx_ble: deInit failed during destruction (%d)", static_cast<int>(status));
    }
}

sys_error_t cpx_ble::setConfig(const hal_ble_config_t& config)
{
    RETURN_IF_ERROR((_initialized.load() || s_nimblePortInitialized || s_hciInitialized), ERROR_INVALID_STATE);
    RETURN_ON_ERROR(_validateConfig(config));
    _config = config;
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::setEventCallback(hal_ble_event_cb_t callback, void* userData)
{
    /// Publish callback state atomically and wait for in-flight callbacks when unregistering.
    RETURN_IF_ERROR((!_initialized.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((callback != nullptr && _hostStarted.load()), ERROR_INVALID_STATE);

    portENTER_CRITICAL(&s_bleStateMux);
    _eventCallback = callback;
    _eventUserData = userData;
    portEXIT_CRITICAL(&s_bleStateMux);

    if (callback == nullptr)
    {
        const TickType_t waitTicks   = (pdMS_TO_TICKS(100U) == 0U) ? 1U : pdMS_TO_TICKS(100U);
        const TickType_t waitStarted = xTaskGetTickCount();
        while (_activeEventCallbacks.load() != 0U)
        {
            RETURN_IF_ERROR(((xTaskGetTickCount() - waitStarted) >= waitTicks), ERROR_TIMEOUT);
            vTaskDelay(1U);
        }
    }
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::init(void* params)
{
    /// Validate parameters, configuration, and exclusive NimBLE ownership before acquiring resources.
#if defined(CONFIG_ENABLE_CHIPOBLE) && CONFIG_ENABLE_CHIPOBLE
    (void)params;
    SYS_LOG_E("cpx_ble: standalone host cannot be started while CHIPoBLE owns the NimBLE host");
    return ERROR_NOT_SUPPORTED;
#endif

    RETURN_IF_ERROR((params != nullptr), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((_initialized.load()), ERROR_INVALID_STATE);
    RETURN_IF_ERROR((_config.roleMask == 0U), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((s_nimbleLifecycleUsed), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((s_instance != nullptr && s_instance != this), ERROR_DEVICE_BUSY);

    hal_ble_capabilities_t capabilities{};
    RETURN_ON_ERROR(getCapabilities(capabilities));
    RETURN_IF_ERROR(((_config.roleMask & ~capabilities.flags) != 0U), ERROR_NOT_SUPPORTED);
    const bool securityRequested = _config.security.enabled || _config.security.bonding || _config.security.mitmProtection || _config.security.secureConnections;
    RETURN_IF_ERROR((securityRequested && (capabilities.flags & HAL_BLE_CAP_BONDING) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((_config.security.secureConnections && (capabilities.flags & HAL_BLE_CAP_SECURE_CONNECTIONS) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((_config.maxConnections > capabilities.maxConnections), ERROR_NOT_SUPPORTED);

    /// Initialize the controller transport and NimBLE host, releasing partial resources on failure.
    s_instance = this;
    if (!s_hciInitialized)
    {
        const esp_err_t hciStatus = esp_nimble_hci_init();
        RETURN_IF_ERROR((hciStatus != ESP_OK),      // Expression
                        TRANSLATE_ERROR(hciStatus), // Error Code
                        s_instance = nullptr;
                        SYS_LOG_E("cpx_ble: esp_nimble_hci_init failed (%d)", static_cast<int>(hciStatus)) // Cleanup
        );
        s_hciInitialized = true;
    }

    const esp_err_t portStatus = nimble_port_init();
    RETURN_IF_ERROR((portStatus != ESP_OK),      // Expression
                    TRANSLATE_ERROR(portStatus), // Error Code
                    const sys_error_t rollbackStatus = deInit();
                    if (rollbackStatus != ERROR_SUCCESS) SYS_LOG_E("cpx_ble: HCI rollback failed after NimBLE initialization error (%d)", static_cast<int>(rollbackStatus));
                    SYS_LOG_E("cpx_ble: nimble_port_init failed (%d)", static_cast<int>(portStatus)) // Cleanup
    );
    s_nimblePortInitialized = true;
    s_nimbleLifecycleUsed   = true;

    /// Configure host callbacks, security policy, MTU, and the standard GAP/GATT services.
    ble_hs_cfg.reset_cb          = &_onHostReset;
    ble_hs_cfg.sync_cb           = &_onHostSync;
    ble_hs_cfg.store_status_cb   = ble_store_util_status_rr;
    ble_hs_cfg.sm_bonding        = _config.security.enabled && _config.security.bonding;
    ble_hs_cfg.sm_mitm           = _config.security.enabled && _config.security.mitmProtection;
    ble_hs_cfg.sm_sc             = _config.security.enabled && _config.security.secureConnections;
    ble_hs_cfg.sm_io_cap         = static_cast<std::uint8_t>(_config.security.ioCapability);
    ble_hs_cfg.sm_our_key_dist   = ble_hs_cfg.sm_bonding ? (BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID) : 0U;
    ble_hs_cfg.sm_their_key_dist = ble_hs_cfg.sm_bonding ? (BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID) : 0U;
    const int mtuStatus          = ble_att_set_preferred_mtu(_config.preferredMtu);
    RETURN_IF_ERROR((mtuStatus != 0),                // Expression
                    translateNimbleError(mtuStatus), // Error Code
                    const sys_error_t rollbackStatus = deInit();
                    if (rollbackStatus != ERROR_SUCCESS) SYS_LOG_E("cpx_ble: rollback failed after preferred MTU setup (%d)", static_cast<int>(rollbackStatus)) // Cleanup
    );

    if (ble_hs_cfg.sm_bonding)
    {
        ble_store_config_init();
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();
    const int deviceNameStatus = ble_svc_gap_device_name_set(_config.deviceName);
    RETURN_IF_ERROR((deviceNameStatus != 0),                // Expression
                    translateNimbleError(deviceNameStatus), // Error Code
                    const sys_error_t rollbackStatus = deInit();
                    if (rollbackStatus != ERROR_SUCCESS) SYS_LOG_E("cpx_ble: rollback failed after GAP name setup (%d)", static_cast<int>(rollbackStatus)) // Cleanup
    );

    /// Publish the initialized state only after all host configuration succeeds.
    _initialized.store(true);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::start()
{
    /// Start the NimBLE host task after initialization has completed.
    RETURN_IF_ERROR((!_initialized.load()), ERROR_NOT_INITIALIZED);
    bool expectedNotStarted = false;
    RETURN_IF_ERROR((!_hostStarted.compare_exchange_strong(expectedNotStarted, true)), ERROR_INVALID_STATE);

    nimble_port_freertos_init(&_hostTask);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::get(void* data)
{
    (void)data;
    return ERROR_NOT_SUPPORTED;
}

sys_error_t cpx_ble::set(void* data)
{
    (void)data;
    return ERROR_NOT_SUPPORTED;
}

sys_error_t cpx_ble::stop()
{
    /// Cancel active GAP procedures and request termination of every tracked connection.
    RETURN_IF_ERROR((!_initialized.load()), ERROR_NOT_INITIALIZED);
    if (_scanState.load() != procedure_state_t::IDLE)
    {
        RETURN_ON_ERROR(stopScan());
    }

    if (_advertisingState.load() != procedure_state_t::IDLE)
    {
        RETURN_ON_ERROR(stopAdvertising());
    }

    std::array<hal_ble_connection_id_t, kMaxSecurityConnections> connections{};
    portENTER_CRITICAL(&s_bleStateMux);
    connections = s_activeConnections;
    portEXIT_CRITICAL(&s_bleStateMux);
    for (const hal_ble_connection_id_t connectionId : connections)
    {
        if (connectionId != HAL_BLE_INVALID_CONNECTION_ID)
        {
            const int status = ble_gap_terminate(connectionId, BLE_ERR_REM_USER_CONN_TERM);
            RETURN_IF_ERROR((status != 0 && status != BLE_HS_ENOTCONN), translateNimbleError(status));
        }
    }

    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::deInit()
{
    /// Drain active procedures and release host/controller resources in reverse acquisition order.
    if (!_initialized.load() && !_hostStarted.load() && !s_nimblePortInitialized && !s_hciInitialized)
    {
        return ERROR_SUCCESS;
    }

    if (_initialized.load())
    {
        RETURN_ON_ERROR(stop());
    }

    if (_hostStarted.load())
    {
        const int stopStatus = nimble_port_stop();
        RETURN_IF_ERROR((stopStatus != 0), translateNimbleError(stopStatus));
        _hostStarted.store(false);
    }

    if (s_nimblePortInitialized)
    {
        const esp_err_t portStatus = nimble_port_deinit();
        RETURN_IF_ERROR((portStatus != ESP_OK), TRANSLATE_ERROR(portStatus));
        s_nimblePortInitialized = false;
    }

    _hostSynced.store(false);
    _initialized.store(false);
    _servicesRegistered.store(false);
    _scanState.store(procedure_state_t::IDLE);
    _advertisingState.store(procedure_state_t::IDLE);
    _eventCallback = nullptr;
    _eventUserData = nullptr;

    if (s_hciInitialized)
    {
        const esp_err_t hciStatus = esp_nimble_hci_deinit();
        RETURN_IF_ERROR((hciStatus != ESP_OK), TRANSLATE_ERROR(hciStatus));
        s_hciInitialized = false;
    }

    if (s_instance == this && !s_nimblePortInitialized && !s_hciInitialized)
    {
        s_instance = nullptr;
    }

    SYS_LOG_I("cpx_ble: deinitialized; this process must not reinitialize NimBLE");
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::getCapabilities(hal_ble_capabilities_t& capabilities) const
{
    /// Report compile-time NimBLE roles and statically bounded driver resources.
    capabilities = {};
#if defined(CONFIG_BT_NIMBLE_SECURITY_ENABLE) && CONFIG_BT_NIMBLE_SECURITY_ENABLE
    capabilities.flags |= HAL_BLE_CAP_BONDING;
#endif
#if defined(CONFIG_BT_NIMBLE_SM_SC) && CONFIG_BT_NIMBLE_SM_SC
    capabilities.flags |= HAL_BLE_CAP_SECURE_CONNECTIONS;
#endif
#if defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL) && CONFIG_BT_NIMBLE_ROLE_CENTRAL
    capabilities.flags |= HAL_BLE_CAP_CENTRAL | HAL_BLE_CAP_GATT_CLIENT;
#endif
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL) && CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
    capabilities.flags |= HAL_BLE_CAP_PERIPHERAL | HAL_BLE_CAP_GATT_SERVER;
#endif
    capabilities.maxConnections = static_cast<std::uint8_t>(kMaxSecurityConnections);
#if defined(CONFIG_BT_NIMBLE_MAX_CONNECTIONS)
    capabilities.maxConnections = std::min<std::uint8_t>(capabilities.maxConnections, static_cast<std::uint8_t>(CONFIG_BT_NIMBLE_MAX_CONNECTIONS));
#endif
    if (capabilities.maxConnections > 1U)
    {
        capabilities.flags |= HAL_BLE_CAP_MULTIPLE_CONNECTIONS;
    }
    capabilities.maxServices              = static_cast<std::uint8_t>(kMaxGattServices);
    capabilities.maxCharacteristics       = static_cast<std::uint8_t>(kMaxGattCharacteristics);
    capabilities.maxDescriptors           = static_cast<std::uint8_t>(kMaxGattDescriptors);
    capabilities.maxMtu                   = 527U;
    capabilities.maxAdvertisingDataLength = static_cast<std::uint16_t>(kMaxLegacyAdvertisingData);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::startScan(const hal_ble_scan_config_t& config)
{
    /// Validate scan timing and reserve the single GAP scan procedure.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((config.intervalMs == 0U), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((config.windowMs == 0U || config.windowMs > config.intervalMs), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((config.intervalMs > 40959U), ERROR_NOT_SUPPORTED);

    const std::uint8_t ownAddressType = s_ownAddressType.load();
    RETURN_IF_ERROR((ownAddressType == kInvalidOwnAddressType), ERROR_INVALID_STATE);
    procedure_state_t expectedState = procedure_state_t::IDLE;
    RETURN_IF_ERROR((!_scanState.compare_exchange_strong(expectedState, procedure_state_t::STARTING)), ERROR_INVALID_STATE);

    ble_gap_disc_params scanParams{};
    scanParams.itvl             = millisecondsTo625MicrosecondUnits(config.intervalMs);
    scanParams.window           = millisecondsTo625MicrosecondUnits(config.windowMs);
    scanParams.filter_policy    = BLE_HCI_SCAN_FILT_NO_WL;
    scanParams.limited          = 0U;
    scanParams.passive          = config.active ? 0U : 1U;
    scanParams.filterDuplicates = config.filterDuplicates ? 1U : 0U;

    const std::int32_t durationMs =
        (config.durationMs == 0U)
            ? BLE_HS_FOREVER
            : static_cast<std::int32_t>(std::min<std::uint32_t>(config.durationMs, static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())));
    /// Submit the scan and roll back the reserved state if NimBLE rejects it.
    const int status = ble_gap_disc(ownAddressType, durationMs, &scanParams, &_gapEventHandler, this);
    RETURN_IF_ERROR((status != 0),                // Expression
                    translateNimbleError(status), // Error Code
                    expectedState = procedure_state_t::STARTING;
                    (void)_scanState.compare_exchange_strong(expectedState, procedure_state_t::IDLE) // Cleanup
    );

    expectedState = procedure_state_t::STARTING;
    (void)_scanState.compare_exchange_strong(expectedState, procedure_state_t::ACTIVE);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::stopScan()
{
    /// Cancel an active scan and restore the idle state if cancellation fails.
    RETURN_IF_ERROR((!_initialized.load()), ERROR_NOT_INITIALIZED);
    procedure_state_t expectedState = procedure_state_t::ACTIVE;
    if (_scanState.compare_exchange_strong(expectedState, procedure_state_t::STOPPING))
    {
        const int status = ble_gap_disc_cancel();
        RETURN_IF_ERROR((status != 0 && status != BLE_HS_EALREADY), // Expression
                        translateNimbleError(status),               // Error Code
                        expectedState = procedure_state_t::STOPPING;
                        (void)_scanState.compare_exchange_strong(expectedState, procedure_state_t::ACTIVE) // Cleanup
        );
        _scanState.store(procedure_state_t::IDLE);
        return ERROR_SUCCESS;
    }

    if (expectedState == procedure_state_t::IDLE)
    {
        return ERROR_SUCCESS;
    }
    return ERROR_DEVICE_BUSY;
}

sys_error_t cpx_ble::startAdvertising(const hal_ble_advertising_config_t& config)
{
    /// Validate legacy advertising constraints and reserve the advertising procedure.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::PERIPHERAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((config.advertisingDataLength > kMaxLegacyAdvertisingData), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((config.scanResponseDataLength > kMaxLegacyAdvertisingData), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((config.connectable != config.scannable), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((!config.scannable && config.scanResponseDataLength != 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((config.advertisingDataLength != 0U && config.advertisingData == nullptr), ERROR_NULL_POINTER);
    RETURN_IF_ERROR((config.scanResponseDataLength != 0U && config.scanResponseData == nullptr), ERROR_NULL_POINTER);
    RETURN_IF_ERROR((config.intervalMinMs < 20U || config.intervalMaxMs > 10240U || config.intervalMaxMs < config.intervalMinMs), ERROR_INVALID_ARG);
    const std::uint8_t ownAddressType = s_ownAddressType.load();
    RETURN_IF_ERROR((ownAddressType == kInvalidOwnAddressType), ERROR_INVALID_STATE);
    procedure_state_t expectedState = procedure_state_t::IDLE;
    RETURN_IF_ERROR((!_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::STARTING)), ERROR_INVALID_STATE);

    std::array<std::uint8_t, kMaxLegacyAdvertisingData> advertisingData{};
    std::array<std::uint8_t, kMaxLegacyAdvertisingData> scanResponseData{};
    if (config.advertisingDataLength > 0U)
    {
        std::memcpy(advertisingData.data(), config.advertisingData, config.advertisingDataLength);
    }
    if (config.scanResponseDataLength > 0U)
    {
        std::memcpy(scanResponseData.data(), config.scanResponseData, config.scanResponseDataLength);
    }

    /// Copy the caller's payloads into NimBLE before starting advertising.
    int status = ble_gap_adv_set_data(advertisingData.data(), static_cast<int>(config.advertisingDataLength));
    RETURN_IF_ERROR((status != 0),                // Expression
                    translateNimbleError(status), // Error Code
                    expectedState = procedure_state_t::STARTING;
                    (void)_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::IDLE) // Cleanup
    );
    status = ble_gap_adv_rsp_set_data(scanResponseData.data(), static_cast<int>(config.scanResponseDataLength));
    RETURN_IF_ERROR((status != 0),                // Expression
                    translateNimbleError(status), // Error Code
                    expectedState = procedure_state_t::STARTING;
                    (void)_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::IDLE) // Cleanup
    );

    ble_gap_adv_params advertisingParams{};
    advertisingParams.conn_mode     = config.connectable ? BLE_GAP_CONN_MODE_UND : BLE_GAP_CONN_MODE_NON;
    advertisingParams.disc_mode     = BLE_GAP_DISC_MODE_GEN;
    advertisingParams.itvl_min      = millisecondsTo625MicrosecondUnits(config.intervalMinMs);
    advertisingParams.itvl_max      = millisecondsTo625MicrosecondUnits(config.intervalMaxMs);
    advertisingParams.channel_map   = BLE_GAP_ADV_DFLT_CHANNEL_MAP;
    advertisingParams.filter_policy = BLE_HCI_ADV_FILT_NONE;

    const std::int32_t durationMs =
        (config.durationMs == 0U)
            ? BLE_HS_FOREVER
            : static_cast<std::int32_t>(std::min<std::uint32_t>(config.durationMs, static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())));
    status = ble_gap_adv_start(ownAddressType, nullptr, durationMs, &advertisingParams, &_gapEventHandler, this);
    RETURN_IF_ERROR((status != 0),                // Expression
                    translateNimbleError(status), // Error Code
                    expectedState = procedure_state_t::STARTING;
                    (void)_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::IDLE) // Cleanup
    );

    expectedState = procedure_state_t::STARTING;
    (void)_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::ACTIVE);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::stopAdvertising()
{
    /// Stop active advertising and restore the idle state if NimBLE rejects the request.
    RETURN_IF_ERROR((!_initialized.load()), ERROR_NOT_INITIALIZED);
    procedure_state_t expectedState = procedure_state_t::ACTIVE;
    if (_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::STOPPING))
    {
        const int status = ble_gap_adv_stop();
        RETURN_IF_ERROR((status != 0 && status != BLE_HS_EALREADY), // Expression
                        translateNimbleError(status),               // Error Code
                        expectedState = procedure_state_t::STOPPING;
                        (void)_advertisingState.compare_exchange_strong(expectedState, procedure_state_t::ACTIVE) // Cleanup
        );
        _advertisingState.store(procedure_state_t::IDLE);
        return ERROR_SUCCESS;
    }

    if (expectedState == procedure_state_t::IDLE)
    {
        return ERROR_SUCCESS;
    }
    return ERROR_DEVICE_BUSY;
}

sys_error_t cpx_ble::connect(const hal_ble_address_t& address, std::uint32_t timeoutMs)
{
    /// Validate the peer, translate configured connection parameters, and submit the connection request.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((_scanState.load() != procedure_state_t::IDLE), ERROR_DEVICE_BUSY);
    const bool validAddressType = address.type == hal_ble_address_type_t::PUBLIC || address.type == hal_ble_address_type_t::RANDOM ||
                                  address.type == hal_ble_address_type_t::PUBLIC_IDENTITY || address.type == hal_ble_address_type_t::RANDOM_IDENTITY;
    RETURN_IF_ERROR((!validAddressType), ERROR_INVALID_ARG);
    const std::uint8_t ownAddressType = s_ownAddressType.load();
    RETURN_IF_ERROR((ownAddressType == kInvalidOwnAddressType), ERROR_INVALID_STATE);

    ble_addr_t peerAddress{};
    switch (address.type)
    {
        case hal_ble_address_type_t::PUBLIC:
            peerAddress.type = BLE_ADDR_PUBLIC;
            break;
        case hal_ble_address_type_t::RANDOM:
            peerAddress.type = BLE_ADDR_RANDOM;
            break;
        case hal_ble_address_type_t::PUBLIC_IDENTITY:
            peerAddress.type = BLE_ADDR_PUBLIC_ID;
            break;
        case hal_ble_address_type_t::RANDOM_IDENTITY:
            peerAddress.type = BLE_ADDR_RANDOM_ID;
            break;
        default:
            return ERROR_INVALID_ARG;
    }
    std::memcpy(peerAddress.val, address.bytes, sizeof(peerAddress.val));

    ble_gap_conn_params connectionParams{};
    connectionParams.scan_itvl           = 16U;
    connectionParams.scan_window         = 16U;
    connectionParams.itvl_min            = millisecondsTo1250MicrosecondUnits(_config.connectionParams.minIntervalMs);
    connectionParams.itvl_max            = millisecondsTo1250MicrosecondUnits(_config.connectionParams.maxIntervalMs);
    connectionParams.latency             = _config.connectionParams.latency;
    connectionParams.supervision_timeout = millisecondsTo10MillisecondUnits(_config.connectionParams.supervisionTimeoutMs);
    connectionParams.min_ce_len          = 0U;
    connectionParams.max_ce_len          = 0U;

    const std::int32_t timeout = (timeoutMs == 0U)
                                     ? static_cast<std::int32_t>(kDefaultConnectTimeoutMs)
                                     : static_cast<std::int32_t>(std::min<std::uint32_t>(timeoutMs, static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())));
    const int          status  = ble_gap_connect(ownAddressType, &peerAddress, timeout, &connectionParams, &_gapEventHandler, this);
    return translateNimbleError(status);
}

sys_error_t cpx_ble::disconnect(hal_ble_connection_id_t connectionId)
{
    /// Request termination of the selected connection.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);

    const int status = ble_gap_terminate(connectionId, BLE_ERR_REM_USER_CONN_TERM);
    return translateNimbleError(status);
}

sys_error_t cpx_ble::secureConnection(hal_ble_connection_id_t connectionId)
{
    /// Initiate link security when the configured security policy permits it.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((!_config.security.enabled), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    return translateNimbleError(ble_gap_security_initiate(connectionId));
}

sys_error_t cpx_ble::respondToPairing(hal_ble_connection_id_t connectionId, bool accept, std::uint32_t passkey)
{
    /// Resolve the pending pairing action and inject the user's decision into NimBLE.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);

    hal_ble_pairing_action_t action = hal_ble_pairing_action_t::NONE;
    RETURN_IF_ERROR((!getPendingPairing(connectionId, action)), ERROR_INVALID_STATE);
    if (!accept)
    {
        clearPendingPairing(connectionId);
        return translateNimbleError(ble_gap_terminate(connectionId, BLE_ERR_AUTH_FAIL));
    }

    if (action == hal_ble_pairing_action_t::INPUT_PASSKEY || action == hal_ble_pairing_action_t::DISPLAY_PASSKEY)
    {
        RETURN_IF_ERROR((passkey > 999999U), ERROR_INVALID_ARG);
    }
    else if (action != hal_ble_pairing_action_t::NUMERIC_COMPARISON)
    {
        return ERROR_NOT_SUPPORTED;
    }

    ble_sm_io securityInput{};
    switch (action)
    {
        case hal_ble_pairing_action_t::INPUT_PASSKEY:
        case hal_ble_pairing_action_t::DISPLAY_PASSKEY:
            securityInput.action  = (action == hal_ble_pairing_action_t::INPUT_PASSKEY) ? BLE_SM_IOACT_INPUT : BLE_SM_IOACT_DISP;
            securityInput.passkey = passkey;
            break;
        case hal_ble_pairing_action_t::NUMERIC_COMPARISON:
            securityInput.action        = BLE_SM_IOACT_NUMCMP;
            securityInput.numcmp_accept = 1U;
            break;
        default:
            return ERROR_NOT_SUPPORTED;
    }

    const int status = ble_sm_inject_io(connectionId, &securityInput);
    if (status == 0)
    {
        clearPendingPairing(connectionId);
    }
    return translateNimbleError(status);
}

sys_error_t cpx_ble::registerGattServices(const hal_ble_gatt_service_t* services, std::size_t serviceCount)
{
    /// Validate the complete profile before translating it into static NimBLE definitions.
    RETURN_IF_ERROR((!_initialized.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((_hostStarted.load()), ERROR_INVALID_STATE);
    RETURN_IF_ERROR((_servicesRegistered.load()), ERROR_INVALID_STATE);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::PERIPHERAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((serviceCount > 0U && services == nullptr), ERROR_NULL_POINTER);
    RETURN_IF_ERROR((serviceCount > kMaxGattServices), ERROR_NOT_SUPPORTED);

    if (serviceCount == 0U)
    {
        return ERROR_SUCCESS;
    }

    std::size_t totalCharacteristics = 0U;
    std::size_t totalDescriptors     = 0U;
    for (std::size_t serviceIndex = 0U; serviceIndex < serviceCount; ++serviceIndex)
    {
        const hal_ble_gatt_service_t& service = services[serviceIndex];
        RETURN_IF_ERROR((!isValidUuidType(service.uuid.type)), ERROR_INVALID_CONFIG);
        RETURN_IF_ERROR((service.type != hal_ble_gatt_service_type_t::PRIMARY && service.type != hal_ble_gatt_service_type_t::SECONDARY), ERROR_INVALID_CONFIG);
        RETURN_IF_ERROR((service.characteristics == nullptr || service.characteristicCount == 0U), ERROR_INVALID_CONFIG);
        RETURN_IF_ERROR((service.characteristicCount > kMaxGattCharacteristics - totalCharacteristics), ERROR_NOT_SUPPORTED);
        totalCharacteristics += service.characteristicCount;
        for (std::size_t characteristicIndex = 0U; characteristicIndex < service.characteristicCount; ++characteristicIndex)
        {
            const hal_ble_gatt_characteristic_t& characteristic = service.characteristics[characteristicIndex];
            RETURN_IF_ERROR((characteristic.descriptorCount != 0U && characteristic.descriptors == nullptr), ERROR_NULL_POINTER);
            RETURN_IF_ERROR((characteristic.descriptorCount > kMaxGattDescriptors - totalDescriptors), ERROR_NOT_SUPPORTED);
            totalDescriptors += characteristic.descriptorCount;
        }
    }
    RETURN_IF_ERROR((totalCharacteristics > kMaxGattCharacteristics), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((totalDescriptors > kMaxGattDescriptors), ERROR_NOT_SUPPORTED);

    s_gattStorage                    = {};
    std::size_t characteristicOffset = 0U;
    std::size_t descriptorOffset     = 0U;
    std::size_t valueHandleIndex     = 0U;
    s_gattStorage.serviceCount       = serviceCount;

    for (std::size_t serviceIndex = 0U; serviceIndex < serviceCount; ++serviceIndex)
    {
        const hal_ble_gatt_service_t& service                    = services[serviceIndex];
        s_gattStorage.serviceCharacteristicOffsets[serviceIndex] = valueHandleIndex;
        s_gattStorage.serviceCharacteristicCounts[serviceIndex]  = service.characteristicCount;
        nimble_uuid_storage_t* const serviceUuid                 = allocateUuid(service.uuid);
        RETURN_IF_ERROR((serviceUuid == nullptr), ERROR_OUT_OF_MEMORY);

        ble_gatt_svc_def& nimbleService = s_gattStorage.services[serviceIndex];
        nimbleService.type              = (service.type == hal_ble_gatt_service_type_t::PRIMARY) ? BLE_GATT_SVC_TYPE_PRIMARY : BLE_GATT_SVC_TYPE_SECONDARY;
        nimbleService.uuid              = convertUuid(*serviceUuid, service.uuid);
        nimbleService.characteristics   = &s_gattStorage.characteristics[characteristicOffset];

        for (std::size_t charIndex = 0U; charIndex < service.characteristicCount; ++charIndex)
        {
            const hal_ble_gatt_characteristic_t& characteristic = service.characteristics[charIndex];
            RETURN_IF_ERROR((!isValidUuidType(characteristic.uuid.type)), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((characteristic.maximumValueLength == 0U), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((characteristic.maximumValueLength > BLE_ATT_ATTR_MAX_LEN), ERROR_NOT_SUPPORTED);
            RETURN_IF_ERROR(((characteristic.properties & ~kValidCharacteristicProperties) != 0U), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR(((characteristic.permissions & ~kValidGattPermissions) != 0U), ERROR_INVALID_CONFIG);
            const bool canRead            = (characteristic.properties & HAL_BLE_GATT_PROPERTY_READ) != 0U;
            const bool canWrite           = (characteristic.properties & (HAL_BLE_GATT_PROPERTY_WRITE | HAL_BLE_GATT_PROPERTY_WRITE_WITHOUT_RSP)) != 0U;
            const bool signedWrite        = (characteristic.properties & HAL_BLE_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE) != 0U;
            const bool hasReadPermission  = (characteristic.permissions & (HAL_BLE_GATT_PERMISSION_READ | HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED)) != 0U;
            const bool hasWritePermission = (characteristic.permissions & (HAL_BLE_GATT_PERMISSION_WRITE | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED)) != 0U;
            RETURN_IF_ERROR((characteristic.properties == 0U), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((canRead != (characteristic.readCallback != nullptr)), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR(((canWrite || signedWrite) != (characteristic.writeCallback != nullptr)), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((canRead != hasReadPermission), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((canWrite != hasWritePermission), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((!signedWrite || (_config.security.enabled && _config.security.bonding)), ERROR_INVALID_CONFIG);
            const bool requiresEncryption = (characteristic.permissions & (HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED | HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED |
                                                                           HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED)) != 0U;
            RETURN_IF_ERROR((requiresEncryption && !_config.security.enabled), ERROR_INVALID_CONFIG);
            const bool requiresMitm = (characteristic.permissions & (HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED)) != 0U;
            RETURN_IF_ERROR((requiresMitm && !_config.security.mitmProtection), ERROR_INVALID_CONFIG);

            nimble_uuid_storage_t* const characteristicUuid = allocateUuid(characteristic.uuid);
            RETURN_IF_ERROR((characteristicUuid == nullptr), ERROR_OUT_OF_MEMORY);

            ble_gatt_chr_def& nimbleCharacteristic = s_gattStorage.characteristics[characteristicOffset];
            nimbleCharacteristic.uuid              = convertUuid(*characteristicUuid, characteristic.uuid);
            nimbleCharacteristic.access_cb         = &_gattAccessHandler;
            nimbleCharacteristic.arg               = &s_gattStorage.bindings[s_gattStorage.bindingCount];
            nimbleCharacteristic.flags             = characteristicFlags(characteristic);
            nimbleCharacteristic.min_key_size      = _config.security.minimumKeySize;
            nimbleCharacteristic.val_handle        = &s_gattStorage.valueHandles[valueHandleIndex++];

            gatt_attribute_binding_t& characteristicBinding = s_gattStorage.bindings[s_gattStorage.bindingCount++];
            characteristicBinding.characteristic            = &characteristic;
            characteristicBinding.descriptor                = nullptr;

            if (characteristic.descriptorCount > 0U)
            {
                nimbleCharacteristic.descriptors = &s_gattStorage.descriptors[descriptorOffset];
            }

            for (std::size_t descriptorIndex = 0U; descriptorIndex < characteristic.descriptorCount; ++descriptorIndex)
            {
                const hal_ble_gatt_descriptor_t& descriptor = characteristic.descriptors[descriptorIndex];
                RETURN_IF_ERROR((!isValidUuidType(descriptor.uuid.type)), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptor.maximumValueLength == 0U), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptor.maximumValueLength > BLE_ATT_ATTR_MAX_LEN), ERROR_NOT_SUPPORTED);
                RETURN_IF_ERROR(((descriptor.permissions & ~kValidGattPermissions) != 0U), ERROR_INVALID_CONFIG);
                const bool descriptorCanRead  = (descriptor.permissions & (HAL_BLE_GATT_PERMISSION_READ | HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED)) != 0U;
                const bool descriptorCanWrite = (descriptor.permissions & (HAL_BLE_GATT_PERMISSION_WRITE | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED)) != 0U;
                RETURN_IF_ERROR((descriptor.permissions == 0U), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptorCanRead != (descriptor.readCallback != nullptr)), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptorCanWrite != (descriptor.writeCallback != nullptr)), ERROR_INVALID_CONFIG);
                const bool descriptorRequiresEncryption =
                    (descriptor.permissions & (HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED | HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                               HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED)) != 0U;
                RETURN_IF_ERROR((descriptorRequiresEncryption && !_config.security.enabled), ERROR_INVALID_CONFIG);
                const bool descriptorRequiresMitm =
                    (descriptor.permissions & (HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED)) != 0U;
                RETURN_IF_ERROR((descriptorRequiresMitm && !_config.security.mitmProtection), ERROR_INVALID_CONFIG);

                nimble_uuid_storage_t* const descriptorUuid = allocateUuid(descriptor.uuid);
                RETURN_IF_ERROR((descriptorUuid == nullptr), ERROR_OUT_OF_MEMORY);

                ble_gatt_dsc_def& nimbleDescriptor = s_gattStorage.descriptors[descriptorOffset];
                nimbleDescriptor.uuid              = convertUuid(*descriptorUuid, descriptor.uuid);
                nimbleDescriptor.att_flags         = descriptorFlags(descriptor);
                nimbleDescriptor.min_key_size      = _config.security.minimumKeySize;
                nimbleDescriptor.access_cb         = &_gattAccessHandler;
                nimbleDescriptor.arg               = &s_gattStorage.bindings[s_gattStorage.bindingCount];

                gatt_attribute_binding_t& descriptorBinding = s_gattStorage.bindings[s_gattStorage.bindingCount++];
                descriptorBinding.characteristic            = nullptr;
                descriptorBinding.descriptor                = &descriptor;
                ++descriptorOffset;
            }

            if (characteristic.descriptorCount > 0U)
            {
                ++descriptorOffset;
            }
            ++characteristicOffset;
        }

        ++characteristicOffset;
    }

    /// Register the validated services before the host starts.
    const int countStatus = ble_gatts_count_cfg(s_gattStorage.services.data());
    RETURN_IF_ERROR((countStatus != 0), translateNimbleError(countStatus));
    const int addStatus = ble_gatts_add_svcs(s_gattStorage.services.data());
    RETURN_IF_ERROR((addStatus != 0), translateNimbleError(addStatus));

    _servicesRegistered.store(true);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::getLocalCharacteristicHandle(std::size_t serviceIndex, std::size_t characteristicIndex, std::uint16_t& valueHandle) const
{
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((!_servicesRegistered.load()), ERROR_INVALID_STATE);
    RETURN_IF_ERROR((serviceIndex >= s_gattStorage.serviceCount), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((characteristicIndex >= s_gattStorage.serviceCharacteristicCounts[serviceIndex]), ERROR_INVALID_ARG);

    const std::size_t handleIndex = s_gattStorage.serviceCharacteristicOffsets[serviceIndex] + characteristicIndex;
    valueHandle                   = s_gattStorage.valueHandles[handleIndex];
    RETURN_IF_ERROR((valueHandle == 0U), ERROR_INVALID_STATE);
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::discoverServices(hal_ble_connection_id_t connectionId, hal_ble_request_id_t& requestId, const hal_ble_uuid_t* uuidFilter)
{
    /// Reserve a bounded operation record and start filtered or unfiltered discovery.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    if (uuidFilter != nullptr)
    {
        RETURN_IF_ERROR((!isValidUuidType(uuidFilter->type)), ERROR_INVALID_ARG);
    }

    requestId = _nextRequestId.fetch_add(1U);
    if (requestId == 0U)
    {
        requestId = _nextRequestId.fetch_add(1U);
    }
    client_operation_t* const operation = allocateClientOperation(this, client_operation_type_t::SERVICE_DISCOVERY, connectionId, requestId);
    RETURN_IF_ERROR((operation == nullptr), ERROR_DEVICE_BUSY);

    if (uuidFilter != nullptr)
    {
        operation->hasUuidFilter  = true;
        operation->uuidFilterType = uuidFilter->type;
        RETURN_IF_ERROR(
            (convertUuid(operation->uuidFilter, *uuidFilter) == nullptr), // Expression
            ERROR_INVALID_ARG,                                            // Error Code
            releaseClientOperation(operation)                             // Cleanup
        );
    }

    const int status = (uuidFilter == nullptr)
                           ? ble_gattc_disc_all_svcs(connectionId, &_serviceDiscoveryCallback, operation)
                           : ble_gattc_disc_svc_by_uuid(connectionId, convertUuid(operation->uuidFilter, *uuidFilter), &_serviceDiscoveryCallback, operation);
    RETURN_IF_ERROR(
        (status != 0),                    // Expression
        translateNimbleError(status),     // Error Code
        releaseClientOperation(operation) // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::discoverCharacteristics(
    hal_ble_connection_id_t connectionId, std::uint16_t startHandle, std::uint16_t endHandle, hal_ble_request_id_t& requestId, const hal_ble_uuid_t* uuidFilter)
{
    /// Reserve a bounded operation record and start characteristic discovery.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((startHandle == 0U || endHandle < startHandle), ERROR_INVALID_ARG);
    if (uuidFilter != nullptr)
    {
        RETURN_IF_ERROR((!isValidUuidType(uuidFilter->type)), ERROR_INVALID_ARG);
    }

    requestId = _nextRequestId.fetch_add(1U);
    if (requestId == 0U)
    {
        requestId = _nextRequestId.fetch_add(1U);
    }
    client_operation_t* const operation = allocateClientOperation(this, client_operation_type_t::CHARACTERISTIC_DISCOVERY, connectionId, requestId);
    RETURN_IF_ERROR((operation == nullptr), ERROR_DEVICE_BUSY);

    if (uuidFilter != nullptr)
    {
        operation->hasUuidFilter  = true;
        operation->uuidFilterType = uuidFilter->type;
        RETURN_IF_ERROR(
            (convertUuid(operation->uuidFilter, *uuidFilter) == nullptr), // Expression
            ERROR_INVALID_ARG,                                            // Error Code
            releaseClientOperation(operation)                             // Cleanup
        );
    }

    const int status = ble_gattc_disc_all_chrs(connectionId, startHandle, endHandle, &_characteristicDiscoveryCallback, operation);
    RETURN_IF_ERROR(
        (status != 0),                    // Expression
        translateNimbleError(status),     // Error Code
        releaseClientOperation(operation) // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::discoverDescriptors(hal_ble_connection_id_t connectionId, std::uint16_t startHandle, std::uint16_t endHandle, hal_ble_request_id_t& requestId)
{
    /// Reserve a bounded operation record and start descriptor discovery.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((startHandle == 0U || endHandle < startHandle), ERROR_INVALID_ARG);

    requestId = _nextRequestId.fetch_add(1U);
    if (requestId == 0U)
    {
        requestId = _nextRequestId.fetch_add(1U);
    }
    client_operation_t* const operation = allocateClientOperation(this, client_operation_type_t::DESCRIPTOR_DISCOVERY, connectionId, requestId);
    RETURN_IF_ERROR((operation == nullptr), ERROR_DEVICE_BUSY);

    const int status = ble_gattc_disc_all_dscs(connectionId, startHandle, endHandle, &_descriptorDiscoveryCallback, operation);
    RETURN_IF_ERROR(
        (status != 0),                    // Expression
        translateNimbleError(status),     // Error Code
        releaseClientOperation(operation) // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::readCharacteristic(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, hal_ble_request_id_t& requestId)
{
    /// Reserve a bounded operation record and submit the remote read.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((valueHandle == 0U), ERROR_INVALID_ARG);

    requestId = _nextRequestId.fetch_add(1U);
    if (requestId == 0U)
    {
        requestId = _nextRequestId.fetch_add(1U);
    }
    client_operation_t* const operation = allocateClientOperation(this, client_operation_type_t::READ, connectionId, requestId);
    RETURN_IF_ERROR((operation == nullptr), ERROR_DEVICE_BUSY);

    const int status = ble_gattc_read(connectionId, valueHandle, &_readCallback, operation);
    RETURN_IF_ERROR(
        (status != 0),                    // Expression
        translateNimbleError(status),     // Error Code
        releaseClientOperation(operation) // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::writeCharacteristic(
    hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength, hal_ble_gatt_write_type_t writeType,
    hal_ble_request_id_t& requestId)
{
    /// Validate payload ownership and submit the selected write procedure.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((valueHandle == 0U), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((dataLength > BLE_ATT_ATTR_MAX_LEN), ERROR_MESSAGE_TOO_LARGE);
    RETURN_IF_ERROR((dataLength != 0U && data == nullptr), ERROR_NULL_POINTER);
    RETURN_IF_ERROR((writeType != hal_ble_gatt_write_type_t::WITH_RESPONSE && writeType != hal_ble_gatt_write_type_t::WITHOUT_RESPONSE), ERROR_INVALID_ARG);

    if (writeType == hal_ble_gatt_write_type_t::WITHOUT_RESPONSE)
    {
        requestId = 0U;
        return translateNimbleError(
            ble_gattc_write_no_rsp_flat(connectionId, valueHandle, (dataLength == 0U) ? &kEmptyPayload : data, static_cast<std::uint16_t>(dataLength)));
    }

    requestId = _nextRequestId.fetch_add(1U);
    if (requestId == 0U)
    {
        requestId = _nextRequestId.fetch_add(1U);
    }
    client_operation_t* const operation = allocateClientOperation(this, client_operation_type_t::WRITE, connectionId, requestId);
    RETURN_IF_ERROR((operation == nullptr), ERROR_DEVICE_BUSY);

    const int status =
        ble_gattc_write_flat(connectionId, valueHandle, (dataLength == 0U) ? &kEmptyPayload : data, static_cast<std::uint16_t>(dataLength), &_writeCallback, operation);
    RETURN_IF_ERROR(
        (status != 0),                    // Expression
        translateNimbleError(status),     // Error Code
        releaseClientOperation(operation) // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::subscribe(
    hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, std::uint16_t cccdHandle, bool notifications, bool indications, hal_ble_request_id_t& requestId)
{
    /// Encode the requested CCCD bits and submit the subscription write.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((valueHandle == 0U || cccdHandle == 0U), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((!notifications && !indications), ERROR_INVALID_ARG);

    requestId = _nextRequestId.fetch_add(1U);
    if (requestId == 0U)
    {
        requestId = _nextRequestId.fetch_add(1U);
    }
    client_operation_t* const operation = allocateClientOperation(this, client_operation_type_t::SUBSCRIBE, connectionId, requestId);
    RETURN_IF_ERROR((operation == nullptr), ERROR_DEVICE_BUSY);

    const std::uint16_t subscriptionBits = static_cast<std::uint16_t>((notifications ? 0x0001U : 0U) | (indications ? 0x0002U : 0U));
    const std::uint8_t  cccdValue[2]     = {static_cast<std::uint8_t>(subscriptionBits & 0x00FFU), static_cast<std::uint8_t>((subscriptionBits >> 8U) & 0x00FFU)};
    const int           status           = ble_gattc_write_flat(connectionId, cccdHandle, cccdValue, sizeof(cccdValue), &_writeCallback, operation);
    RETURN_IF_ERROR(
        (status != 0),                    // Expression
        translateNimbleError(status),     // Error Code
        releaseClientOperation(operation) // Cleanup
    );
    return ERROR_SUCCESS;
}

sys_error_t cpx_ble::sendNotification(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength)
{
    /// Copy the value into a NimBLE-owned mbuf and transmit it without acknowledgment.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::PERIPHERAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((valueHandle == 0U), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((dataLength > BLE_ATT_ATTR_MAX_LEN), ERROR_MESSAGE_TOO_LARGE);
    RETURN_IF_ERROR((dataLength != 0U && data == nullptr), ERROR_NULL_POINTER);

    os_mbuf* const message = ble_hs_mbuf_from_flat((dataLength == 0U) ? &kEmptyPayload : data, static_cast<std::uint16_t>(dataLength));
    RETURN_IF_ERROR((message == nullptr), ERROR_OUT_OF_MEMORY);
    return translateNimbleError(ble_gatts_notify_custom(connectionId, valueHandle, message));
}

sys_error_t cpx_ble::sendIndication(hal_ble_connection_id_t connectionId, std::uint16_t valueHandle, const std::uint8_t* data, std::size_t dataLength)
{
    /// Copy the value into a NimBLE-owned mbuf and transmit it with acknowledgment.
    RETURN_IF_ERROR((!_hostSynced.load()), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(((_config.roleMask & static_cast<std::uint8_t>(hal_ble_role_t::PERIPHERAL)) == 0U), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((connectionId == HAL_BLE_INVALID_CONNECTION_ID), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((valueHandle == 0U), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((dataLength > BLE_ATT_ATTR_MAX_LEN), ERROR_MESSAGE_TOO_LARGE);
    RETURN_IF_ERROR((dataLength != 0U && data == nullptr), ERROR_NULL_POINTER);

    os_mbuf* const message = ble_hs_mbuf_from_flat((dataLength == 0U) ? &kEmptyPayload : data, static_cast<std::uint16_t>(dataLength));
    RETURN_IF_ERROR((message == nullptr), ERROR_OUT_OF_MEMORY);
    return translateNimbleError(ble_gatts_indicate_custom(connectionId, valueHandle, message));
}

bool cpx_ble::isInitialized() const
{
    return _initialized.load();
}

sys_error_t cpx_ble::_validateConfig(const hal_ble_config_t& config) const
{
    /// Enforce platform bounds and Bluetooth interval/security constraints.
    constexpr std::uint8_t kValidRoleMask = static_cast<std::uint8_t>(hal_ble_role_t::CENTRAL) | static_cast<std::uint8_t>(hal_ble_role_t::PERIPHERAL);

    RETURN_IF_ERROR((config.roleMask == 0U || (config.roleMask & ~kValidRoleMask) != 0U), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.deviceName[0] == '\0'), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((std::memchr(config.deviceName, '\0', sizeof(config.deviceName)) == nullptr), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.maxConnections == 0U), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.maxConnections > kMaxSecurityConnections), ERROR_NOT_SUPPORTED);
    RETURN_IF_ERROR((config.preferredMtu < kDefaultMtu || config.preferredMtu > 527U), ERROR_INVALID_CONFIG);
    const bool invalidMinimumKeySize = config.security.minimumKeySize != 0U && (config.security.minimumKeySize < 7U || config.security.minimumKeySize > 16U);
    RETURN_IF_ERROR((invalidMinimumKeySize), ERROR_INVALID_CONFIG);
    const bool securityFeaturesDisabled =
        !config.security.enabled && (config.security.bonding || config.security.mitmProtection || config.security.secureConnections || config.security.minimumKeySize != 0U);
    RETURN_IF_ERROR((securityFeaturesDisabled), ERROR_INVALID_CONFIG);
    const bool validIoCapability = config.security.ioCapability == hal_ble_io_capability_t::DISPLAY_ONLY ||
                                   config.security.ioCapability == hal_ble_io_capability_t::DISPLAY_YES_NO ||
                                   config.security.ioCapability == hal_ble_io_capability_t::KEYBOARD_ONLY || config.security.ioCapability == hal_ble_io_capability_t::NONE ||
                                   config.security.ioCapability == hal_ble_io_capability_t::KEYBOARD_DISPLAY;
    RETURN_IF_ERROR((!validIoCapability), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.connectionParams.minIntervalMs < 8U), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.connectionParams.maxIntervalMs > 4000U), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.connectionParams.maxIntervalMs < config.connectionParams.minIntervalMs), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.connectionParams.latency > 499U), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.connectionParams.supervisionTimeoutMs < 100U || config.connectionParams.supervisionTimeoutMs > 32000U), ERROR_INVALID_CONFIG);
    const bool supervisionTimeoutTooShort = static_cast<std::uint32_t>(config.connectionParams.supervisionTimeoutMs) <=
                                            (1U + config.connectionParams.latency) * static_cast<std::uint32_t>(config.connectionParams.maxIntervalMs) * 2U;
    RETURN_IF_ERROR((supervisionTimeoutTooShort), ERROR_INVALID_CONFIG);
    RETURN_IF_ERROR((config.security.mitmProtection && config.security.ioCapability == hal_ble_io_capability_t::NONE), ERROR_INVALID_CONFIG);
    return ERROR_SUCCESS;
}

void cpx_ble::_emitEvent(hal_ble_event_type_t event, const hal_ble_event_data_t& eventData)
{
    /// Snapshot callback state under the critical section and track active invocations for safe detachment.
    hal_ble_event_cb_t callback = nullptr;
    void*              userData = nullptr;
    portENTER_CRITICAL(&s_bleStateMux);
    callback = _eventCallback;
    userData = _eventUserData;
    if (callback != nullptr)
    {
        _activeEventCallbacks.fetch_add(1U);
    }
    portEXIT_CRITICAL(&s_bleStateMux);

    if (callback != nullptr)
    {
        callback(event, eventData, userData);
        _activeEventCallbacks.fetch_sub(1U);
    }
}

void cpx_ble::_hostTask(void* arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void cpx_ble::_onHostSync()
{
    /// Resolve the local identity address and publish host readiness to registered listeners.
    if (s_instance == nullptr)
    {
        return;
    }

    std::uint8_t ownAddressType = kInvalidOwnAddressType;
    int          status         = ble_hs_id_infer_auto(0, &ownAddressType);
    if (status != 0)
    {
        hal_ble_event_data_t eventData{};
        fillEventStatus(eventData, status);
        s_instance->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        return;
    }
    s_ownAddressType.store(ownAddressType);

    s_instance->_hostSynced.store(true);
    hal_ble_event_data_t eventData{};
    eventData.status     = ERROR_SUCCESS;
    eventData.gattStatus = hal_ble_gatt_status_t::SUCCESS;
    s_instance->_emitEvent(hal_ble_event_type_t::HOST_READY, eventData);
}

void cpx_ble::_onHostReset(int reason)
{
    /// Clear all host-owned state and notify listeners that NimBLE reset.
    if (s_instance != nullptr)
    {
        s_instance->_hostSynced.store(false);
        s_instance->_scanState.store(procedure_state_t::IDLE);
        s_instance->_advertisingState.store(procedure_state_t::IDLE);
        s_ownAddressType.store(kInvalidOwnAddressType);
        portENTER_CRITICAL(&s_bleStateMux);
        s_activeConnections.fill(HAL_BLE_INVALID_CONNECTION_ID);
        for (pending_pairing_t& pairing : s_pendingPairings)
        {
            pairing.active       = false;
            pairing.connectionId = HAL_BLE_INVALID_CONNECTION_ID;
            pairing.action       = hal_ble_pairing_action_t::NONE;
        }
        for (client_operation_t& operation : s_clientOperations)
        {
            operation.active        = false;
            operation.owner         = nullptr;
            operation.connectionId  = HAL_BLE_INVALID_CONNECTION_ID;
            operation.requestId     = 0U;
            operation.hasUuidFilter = false;
        }
        portEXIT_CRITICAL(&s_bleStateMux);
        hal_ble_event_data_t eventData{};
        eventData.status           = ERROR_FAIL;
        eventData.gattStatus       = hal_ble_gatt_status_t::UNLIKELY_ERROR;
        eventData.disconnectReason = reason;
        s_instance->_emitEvent(hal_ble_event_type_t::HOST_RESET, eventData);
    }
}

int cpx_ble::_gapEventHandler(struct ble_gap_event* event, void* arg)
{
    /// Translate GAP events to portable events while updating driver procedure and connection state.
    cpx_ble* const driver = static_cast<cpx_ble*>(arg);
    if (driver == nullptr || event == nullptr)
    {
        return BLE_HS_EINVAL;
    }

    hal_ble_event_data_t eventData{};
    eventData.status     = ERROR_SUCCESS;
    eventData.gattStatus = hal_ble_gatt_status_t::SUCCESS;

    switch (event->type)
    {
        case BLE_GAP_EVENT_DISC:
            eventData.address.type = convertAddressType(event->disc.addr.type);
            std::memcpy(eventData.address.bytes, event->disc.addr.val, sizeof(eventData.address.bytes));
            eventData.rssi              = event->disc.rssi;
            eventData.advertisementType = event->disc.event_type;
            eventData.data              = event->disc.data;
            eventData.dataLength        = event->disc.length_data;
            driver->_emitEvent(hal_ble_event_type_t::SCAN_RESULT, eventData);
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            driver->_scanState.store(procedure_state_t::IDLE);
            eventData.operation = hal_ble_event_operation_t::SCAN;
            eventData.status    = event->disc_complete.reason == 0 ? ERROR_SUCCESS : ERROR_FAIL;
            driver->_emitEvent(hal_ble_event_type_t::SCAN_COMPLETE, eventData);
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            driver->_advertisingState.store(procedure_state_t::IDLE);
            eventData.operation = hal_ble_event_operation_t::ADVERTISING;
            eventData.status    = event->adv_complete.reason == 0 ? ERROR_SUCCESS : ERROR_CONNECTION_LOST;
            driver->_emitEvent(hal_ble_event_type_t::ADVERTISING_STOPPED, eventData);
            return 0;

        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0)
            {
                eventData.connectionId = event->connect.conn_handle;
                ble_gap_conn_desc connectionDescription{};
                const bool        hasConnectionDescription = ble_gap_conn_find(eventData.connectionId, &connectionDescription) == 0;
                if (hasConnectionDescription)
                {
                    eventData.role         = (connectionDescription.role == BLE_GAP_ROLE_MASTER) ? hal_ble_role_t::CENTRAL : hal_ble_role_t::PERIPHERAL;
                    eventData.address.type = convertAddressType(connectionDescription.peer_ota_addr.type);
                    std::memcpy(eventData.address.bytes, connectionDescription.peer_ota_addr.val, sizeof(eventData.address.bytes));
                }
                if (!trackConnection(eventData.connectionId, driver->_config.maxConnections))
                {
                    (void)ble_gap_terminate(eventData.connectionId, BLE_ERR_REM_USER_CONN_TERM);
                    eventData.status    = ERROR_NETWORK_LIMIT;
                    eventData.operation = hal_ble_event_operation_t::CONNECT;
                    driver->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
                    return 0;
                }
                if (hasConnectionDescription && connectionDescription.role == BLE_GAP_ROLE_MASTER)
                {
                    (void)ble_gattc_exchange_mtu(event->connect.conn_handle, nullptr, nullptr);
                }
                driver->_emitEvent(hal_ble_event_type_t::CONNECTED, eventData);
                if (driver->_advertisingState.exchange(procedure_state_t::IDLE) != procedure_state_t::IDLE)
                {
                    driver->_emitEvent(hal_ble_event_type_t::ADVERTISING_STOPPED, eventData);
                }
            }
            else
            {
                eventData.operation = hal_ble_event_operation_t::CONNECT;
                fillEventStatus(eventData, event->connect.status);
                driver->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            eventData.connectionId     = event->disconnect.conn.conn_handle;
            eventData.status           = ERROR_SUCCESS;
            eventData.disconnectReason = event->disconnect.reason;
            eventData.address.type     = convertAddressType(event->disconnect.conn.peer_ota_addr.type);
            eventData.role             = (event->disconnect.conn.role == BLE_GAP_ROLE_MASTER) ? hal_ble_role_t::CENTRAL : hal_ble_role_t::PERIPHERAL;
            std::memcpy(eventData.address.bytes, event->disconnect.conn.peer_ota_addr.val, sizeof(eventData.address.bytes));
            removeConnection(eventData.connectionId);
            clearPendingPairing(eventData.connectionId);
            driver->_emitEvent(hal_ble_event_type_t::DISCONNECTED, eventData);
            return 0;

        case BLE_GAP_EVENT_MTU:
            eventData.connectionId = event->mtu.conn_handle;
            eventData.mtu          = event->mtu.value;
            driver->_emitEvent(hal_ble_event_type_t::MTU_CHANGED, eventData);
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            eventData.connectionId         = event->subscribe.conn_handle;
            eventData.attributeHandle      = event->subscribe.attr_handle;
            eventData.notificationsEnabled = event->subscribe.cur_notify != 0U;
            eventData.indicationsEnabled   = event->subscribe.cur_indicate != 0U;
            driver->_emitEvent(hal_ble_event_type_t::GATT_SERVER_SUBSCRIPTION_CHANGED, eventData);
            return 0;

        case BLE_GAP_EVENT_NOTIFY_TX:
            eventData.connectionId    = event->notify_tx.conn_handle;
            eventData.attributeHandle = event->notify_tx.attr_handle;
            fillEventStatus(eventData, event->notify_tx.status);
            if (event->notify_tx.indication)
            {
                driver->_emitEvent(hal_ble_event_type_t::GATT_INDICATION_CONFIRMED, eventData);
            }
            else
            {
                driver->_emitEvent(hal_ble_event_type_t::GATT_NOTIFICATION_SENT, eventData);
            }
            return 0;

        case BLE_GAP_EVENT_NOTIFY_RX:
        {
            if (event->notify_rx.om == nullptr)
            {
                return BLE_HS_EINVAL;
            }
            std::array<std::uint8_t, kMaxAttributeLength> payload{};
            const std::size_t                             payloadLength = OS_MBUF_PKTLEN(event->notify_rx.om);
            if (payloadLength > payload.size() || os_mbuf_copydata(event->notify_rx.om, 0, static_cast<int>(payloadLength), payload.data()) != 0)
            {
                eventData.connectionId    = event->notify_rx.conn_handle;
                eventData.attributeHandle = event->notify_rx.attr_handle;
                eventData.status          = ERROR_BUFFER_OVERFLOW;
                driver->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
                return BLE_HS_EMSGSIZE;
            }
            eventData.connectionId    = event->notify_rx.conn_handle;
            eventData.attributeHandle = event->notify_rx.attr_handle;
            eventData.isIndication    = event->notify_rx.indication != 0U;
            eventData.data            = payload.data();
            eventData.dataLength      = payloadLength;
            driver->_emitEvent(eventData.isIndication ? hal_ble_event_type_t::GATT_INDICATION_RECEIVED : hal_ble_event_type_t::GATT_NOTIFICATION_RECEIVED, eventData);
            return 0;
        }

        case BLE_GAP_EVENT_ENC_CHANGE:
            eventData.connectionId = event->enc_change.conn_handle;
            fillEventStatus(eventData, event->enc_change.status);
            clearPendingPairing(eventData.connectionId);
            if (event->enc_change.status == 0)
            {
                ble_gap_conn_desc connectionDescription{};
                if (ble_gap_conn_find(eventData.connectionId, &connectionDescription) == 0)
                {
                    eventData.encrypted     = connectionDescription.sec_state.encrypted;
                    eventData.authenticated = connectionDescription.sec_state.authenticated;
                    eventData.bonded        = connectionDescription.sec_state.bonded;
                }
            }
            driver->_emitEvent(hal_ble_event_type_t::SECURITY_CHANGED, eventData);
            driver->_emitEvent(hal_ble_event_type_t::PAIRING_COMPLETE, eventData);
            return 0;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
        {
            eventData.connectionId = event->passkey.conn_handle;
            switch (event->passkey.params.action)
            {
                case BLE_SM_IOACT_DISP:
                    eventData.pairingAction = hal_ble_pairing_action_t::DISPLAY_PASSKEY;
                    eventData.passkey       = event->passkey.params.passkey;
                    break;
                case BLE_SM_IOACT_INPUT:
                    eventData.pairingAction = hal_ble_pairing_action_t::INPUT_PASSKEY;
                    break;
                case BLE_SM_IOACT_NUMCMP:
                    eventData.pairingAction = hal_ble_pairing_action_t::NUMERIC_COMPARISON;
                    eventData.passkey       = event->passkey.params.numcmp;
                    break;
                case BLE_SM_IOACT_OOB:
                    eventData.pairingAction = hal_ble_pairing_action_t::OOB_DATA;
                    break;
                default:
                    eventData.pairingAction = hal_ble_pairing_action_t::NONE;
                    break;
            }
            addPendingPairing(eventData.connectionId, eventData.pairingAction);
            driver->_emitEvent(hal_ble_event_type_t::PASSKEY_ACTION, eventData);
            return 0;
        }

        default:
            return 0;
    }
}

int cpx_ble::_gattAccessHandler(std::uint16_t connectionHandle, std::uint16_t attributeHandle, struct ble_gatt_access_ctxt* context, void* arg)
{
    /// Validate the attribute binding and relay bounded reads or writes to the registered callback.
    (void)attributeHandle;
    const gatt_attribute_binding_t* const binding = static_cast<const gatt_attribute_binding_t*>(arg);
    if (binding == nullptr || context == nullptr)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    const hal_ble_gatt_read_cb_t  readCallback  = (binding->characteristic != nullptr) ? binding->characteristic->readCallback : binding->descriptor->readCallback;
    const hal_ble_gatt_write_cb_t writeCallback = (binding->characteristic != nullptr) ? binding->characteristic->writeCallback : binding->descriptor->writeCallback;
    const std::uint16_t maximumValueLength      = (binding->characteristic != nullptr) ? binding->characteristic->maximumValueLength : binding->descriptor->maximumValueLength;
    void* const         userData                = (binding->characteristic != nullptr) ? binding->characteristic->userData : binding->descriptor->userData;

    std::array<std::uint8_t, kMaxAttributeLength> value{};
    const hal_ble_connection_id_t                 connectionId = connectionHandle;

    if (context->op == BLE_GATT_ACCESS_OP_READ_CHR || context->op == BLE_GATT_ACCESS_OP_READ_DSC)
    {
        if (readCallback == nullptr)
        {
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }

        std::size_t                 valueLength = 0U;
        const hal_ble_gatt_status_t status =
            readCallback(connectionId, context->offset, value.data(), std::min<std::size_t>(value.size(), maximumValueLength), valueLength, userData);
        if (status != hal_ble_gatt_status_t::SUCCESS)
        {
            return static_cast<int>(status);
        }
        if (valueLength > maximumValueLength || valueLength > value.size())
        {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (os_mbuf_append(context->om, value.data(), static_cast<std::uint16_t>(valueLength)) != 0)
        {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return 0;
    }

    if (context->op == BLE_GATT_ACCESS_OP_WRITE_CHR || context->op == BLE_GATT_ACCESS_OP_WRITE_DSC)
    {
        if (writeCallback == nullptr)
        {
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
        }

        const std::size_t valueLength = OS_MBUF_PKTLEN(context->om);
        if (valueLength > maximumValueLength || valueLength > value.size())
        {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (os_mbuf_copydata(context->om, 0, static_cast<int>(valueLength), value.data()) != 0)
        {
            return BLE_ATT_ERR_UNLIKELY;
        }
        return static_cast<int>(writeCallback(connectionId, context->offset, value.data(), valueLength, userData));
    }

    return BLE_ATT_ERR_UNLIKELY;
}

int cpx_ble::_serviceDiscoveryCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, const struct ble_gatt_svc* service, void* arg)
{
    /// Correlate service results with the pending request and release its slot on completion.
    client_operation_t* const operation = static_cast<client_operation_t*>(arg);
    if (operation == nullptr || !operation->active || operation->owner == nullptr || error == nullptr)
    {
        return BLE_HS_EINVAL;
    }

    hal_ble_event_data_t eventData{};
    eventData.connectionId = connectionHandle;
    eventData.requestId    = operation->requestId;
    eventData.operation    = hal_ble_event_operation_t::SERVICE_DISCOVERY;

    if (error->status == BLE_HS_EDONE)
    {
        operation->owner->_emitEvent(hal_ble_event_type_t::GATT_SERVICE_DISCOVERY_COMPLETE, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (error->status != 0)
    {
        fillEventStatus(eventData, error->status);
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (service == nullptr)
    {
        eventData.status = ERROR_PROTOCOL;
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (operation->hasUuidFilter && ble_uuid_cmp(&service->uuid.u, storedUuid(operation->uuidFilter, operation->uuidFilterType)) != 0)
    {
        return 0;
    }

    eventData.startHandle = service->startHandle;
    eventData.endHandle   = service->endHandle;
    convertUuid(service->uuid, eventData.uuid);
    operation->owner->_emitEvent(hal_ble_event_type_t::GATT_SERVICE_DISCOVERED, eventData);
    return 0;
}

int cpx_ble::_characteristicDiscoveryCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, const struct ble_gatt_chr* characteristic, void* arg)
{
    /// Correlate characteristic results with the pending request and release its slot on completion.
    client_operation_t* const operation = static_cast<client_operation_t*>(arg);
    if (operation == nullptr || !operation->active || operation->owner == nullptr || error == nullptr)
    {
        return BLE_HS_EINVAL;
    }

    hal_ble_event_data_t eventData{};
    eventData.connectionId = connectionHandle;
    eventData.requestId    = operation->requestId;
    eventData.operation    = hal_ble_event_operation_t::CHARACTERISTIC_DISCOVERY;
    if (error->status == BLE_HS_EDONE)
    {
        operation->owner->_emitEvent(hal_ble_event_type_t::GATT_CHARACTERISTIC_DISCOVERY_COMPLETE, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (error->status != 0)
    {
        fillEventStatus(eventData, error->status);
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (characteristic == nullptr)
    {
        eventData.status = ERROR_PROTOCOL;
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }

    if (operation->hasUuidFilter)
    {
        if (ble_uuid_cmp(&characteristic->uuid.u, storedUuid(operation->uuidFilter, operation->uuidFilterType)) != 0)
        {
            return 0;
        }
    }
    eventData.attributeHandle = characteristic->val_handle;
    eventData.startHandle     = characteristic->def_handle;
    eventData.endHandle       = characteristic->val_handle;
    eventData.properties      = characteristic->properties;
    convertUuid(characteristic->uuid, eventData.uuid);
    operation->owner->_emitEvent(hal_ble_event_type_t::GATT_CHARACTERISTIC_DISCOVERED, eventData);
    return 0;
}

int cpx_ble::_descriptorDiscoveryCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, const struct ble_gatt_dsc* descriptor, void* arg)
{
    /// Correlate descriptor results with the pending request and release its slot on completion.
    client_operation_t* const operation = static_cast<client_operation_t*>(arg);
    if (operation == nullptr || !operation->active || operation->owner == nullptr || error == nullptr)
    {
        return BLE_HS_EINVAL;
    }

    hal_ble_event_data_t eventData{};
    eventData.connectionId = connectionHandle;
    eventData.requestId    = operation->requestId;
    eventData.operation    = hal_ble_event_operation_t::DESCRIPTOR_DISCOVERY;
    if (error->status == BLE_HS_EDONE)
    {
        operation->owner->_emitEvent(hal_ble_event_type_t::GATT_DESCRIPTOR_DISCOVERY_COMPLETE, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (error->status != 0)
    {
        fillEventStatus(eventData, error->status);
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (descriptor == nullptr)
    {
        eventData.status = ERROR_PROTOCOL;
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }

    eventData.attributeHandle = descriptor->handle;
    convertUuid(descriptor->uuid, eventData.uuid);
    operation->owner->_emitEvent(hal_ble_event_type_t::GATT_DESCRIPTOR_DISCOVERED, eventData);
    return 0;
}

int cpx_ble::_readCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, struct ble_gatt_attr* attribute, void* arg)
{
    /// Copy the read result into callback-scoped storage and complete its request.
    client_operation_t* const operation = static_cast<client_operation_t*>(arg);
    if (operation == nullptr || !operation->active || operation->owner == nullptr || error == nullptr)
    {
        return BLE_HS_EINVAL;
    }

    hal_ble_event_data_t eventData{};
    eventData.connectionId = connectionHandle;
    eventData.requestId    = operation->requestId;
    eventData.operation    = hal_ble_event_operation_t::READ;
    if (error->status != 0)
    {
        fillEventStatus(eventData, error->status);
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }
    if (attribute == nullptr || attribute->om == nullptr)
    {
        eventData.status = ERROR_PROTOCOL;
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }

    std::array<std::uint8_t, kMaxAttributeLength> value{};
    const std::size_t                             valueLength = OS_MBUF_PKTLEN(attribute->om);
    if (valueLength > value.size() || os_mbuf_copydata(attribute->om, 0, static_cast<int>(valueLength), value.data()) != 0)
    {
        eventData.status = ERROR_BUFFER_OVERFLOW;
        operation->owner->_emitEvent(hal_ble_event_type_t::OPERATION_FAILED, eventData);
        releaseClientOperation(operation);
        return 0;
    }

    eventData.attributeHandle = attribute->handle;
    eventData.data            = value.data();
    eventData.dataLength      = valueLength;
    operation->owner->_emitEvent(hal_ble_event_type_t::GATT_CLIENT_READ_COMPLETE, eventData);
    releaseClientOperation(operation);
    return 0;
}

int cpx_ble::_writeCallback(std::uint16_t connectionHandle, const struct ble_gatt_error* error, struct ble_gatt_attr* attribute, void* arg)
{
    /// Report the write or subscription result and release its pending operation slot.
    client_operation_t* const operation = static_cast<client_operation_t*>(arg);
    if (operation == nullptr || !operation->active || operation->owner == nullptr || error == nullptr)
    {
        return BLE_HS_EINVAL;
    }

    hal_ble_event_data_t eventData{};
    eventData.connectionId    = connectionHandle;
    eventData.requestId       = operation->requestId;
    eventData.operation       = (operation->type == client_operation_type_t::SUBSCRIBE) ? hal_ble_event_operation_t::SUBSCRIBE : hal_ble_event_operation_t::WRITE;
    eventData.attributeHandle = (attribute == nullptr) ? 0U : attribute->handle;
    fillEventStatus(eventData, error->status);
    const hal_ble_event_type_t event =
        (operation->type == client_operation_type_t::SUBSCRIBE) ? hal_ble_event_type_t::GATT_CLIENT_SUBSCRIPTION_COMPLETE : hal_ble_event_type_t::GATT_CLIENT_WRITE_COMPLETE;
    operation->owner->_emitEvent(event, eventData);
    releaseClientOperation(operation);
    return 0;
}
