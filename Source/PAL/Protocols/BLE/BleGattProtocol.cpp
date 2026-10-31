/** @file       BleGattProtocol.cpp
 *  @brief      Generic GATT profile registration over the BLE HAL.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       30/10/2026
 */

/** INCLUDES ******************************************************************/

#include "BleGattProtocol.hpp"

#include "System/LogHandler.h"
#include "System/error_macros.h"

/** CONSTANTS *****************************************************************/

namespace
{
    constexpr std::uint16_t kValidCharacteristicProperties = HAL_BLE_GATT_PROPERTY_READ | HAL_BLE_GATT_PROPERTY_WRITE | HAL_BLE_GATT_PROPERTY_WRITE_WITHOUT_RSP |
                                                             HAL_BLE_GATT_PROPERTY_NOTIFY | HAL_BLE_GATT_PROPERTY_INDICATE | HAL_BLE_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE;
    constexpr std::uint16_t kValidPermissions = HAL_BLE_GATT_PERMISSION_READ | HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED | HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED |
                                                HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED | HAL_BLE_GATT_PERMISSION_WRITE | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                                HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED;
} // namespace

BleGattProtocol::BleGattProtocol(IHal_Cpx_Ble& bleHal, const hal_ble_gatt_service_t* services, std::size_t serviceCount)
    : _bleHal(bleHal)
    , _services(services)
    , _serviceCount(serviceCount)
    , _eventCallback(nullptr)
    , _eventUserData(nullptr)
    , _initialized(false)
    , _profilesRegistered(false)
{
}

BleGattProtocol::~BleGattProtocol()
{
    const sys_error_t status = deInit();
    if (status != ERROR_SUCCESS)
    {
        SYS_LOG_E("BleGattProtocol: failed to detach BLE event callback (%d)", static_cast<int>(status));
    }
}

/** FUNCTIONS *****************************************************************/

sys_error_t BleGattProtocol::init()
{
    /// Validate the profile and attach the consumer callback before registering the services.
    RETURN_IF_ERROR((_initialized), ERROR_INVALID_STATE);
    RETURN_IF_ERROR((!_bleHal.isInitialized()), ERROR_NOT_INITIALIZED);
    RETURN_ON_ERROR(_validateProfiles());

    RETURN_ON_ERROR(_bleHal.setEventCallback(&_halEventThunk, this));

    if (!_profilesRegistered && _serviceCount > 0U)
    {
        /// Register the validated profiles while the HAL is initialized but before it starts.
        RETURN_ON_ERROR(
            _bleHal.registerGattServices(_services, _serviceCount), // Expression
            _rollbackEventCallback()                                // Cleanup
        );
    }

    /// Commit initialized state only after callback attachment and profile registration succeed.
    _profilesRegistered = true;
    _initialized        = true;
    return ERROR_SUCCESS;
}

void BleGattProtocol::_rollbackEventCallback()
{
    const sys_error_t detachStatus = _bleHal.setEventCallback(nullptr);
    if (detachStatus != ERROR_SUCCESS)
    {
        _initialized = true;
        SYS_LOG_E("BleGattProtocol: failed to roll back event callback (%d)", static_cast<int>(detachStatus));
    }
}

sys_error_t BleGattProtocol::deInit()
{
    /// Detach the callback without taking ownership of the injected HAL lifecycle.
    if (!_initialized)
    {
        return ERROR_SUCCESS;
    }

    if (_bleHal.isInitialized())
    {
        RETURN_ON_ERROR(_bleHal.setEventCallback(nullptr));
    }
    _initialized = false;
    return ERROR_SUCCESS;
}

sys_error_t BleGattProtocol::setEventCallback(hal_ble_event_cb_t callback, void* userData)
{
    RETURN_IF_ERROR((_initialized), ERROR_INVALID_STATE);

    _eventCallback = callback;
    _eventUserData = userData;
    return ERROR_SUCCESS;
}

void BleGattProtocol::_halEventThunk(hal_ble_event_type_t event, const hal_ble_event_data_t& eventData, void* userData)
{
    BleGattProtocol* const protocol = static_cast<BleGattProtocol*>(userData);
    if (protocol != nullptr && protocol->_eventCallback != nullptr)
    {
        protocol->_eventCallback(event, eventData, protocol->_eventUserData);
    }
}

bool BleGattProtocol::_isUuidValid(const hal_ble_uuid_t& uuid)
{
    switch (uuid.type)
    {
        case hal_ble_uuid_type_t::UUID16:
        case hal_ble_uuid_type_t::UUID32:
        case hal_ble_uuid_type_t::UUID128:
            return true;
        default:
            return false;
    }
}

sys_error_t BleGattProtocol::_validateProfiles() const
{
    /// Validate each service, characteristic, and descriptor before passing the profile to the HAL.
    RETURN_IF_ERROR((_serviceCount > 0U && _services == nullptr), ERROR_NULL_POINTER);

    for (std::size_t serviceIndex = 0U; serviceIndex < _serviceCount; ++serviceIndex)
    {
        const hal_ble_gatt_service_t& service = _services[serviceIndex];
        RETURN_IF_ERROR((!_isUuidValid(service.uuid)), ERROR_INVALID_CONFIG);
        RETURN_IF_ERROR((service.type != hal_ble_gatt_service_type_t::PRIMARY && service.type != hal_ble_gatt_service_type_t::SECONDARY), ERROR_INVALID_CONFIG);
        RETURN_IF_ERROR((service.characteristicCount == 0U), ERROR_INVALID_CONFIG);
        RETURN_IF_ERROR((service.characteristics == nullptr), ERROR_NULL_POINTER);

        for (std::size_t characteristicIndex = 0U; characteristicIndex < service.characteristicCount; ++characteristicIndex)
        {
            const hal_ble_gatt_characteristic_t& characteristic = service.characteristics[characteristicIndex];
            RETURN_IF_ERROR((!_isUuidValid(characteristic.uuid)), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((characteristic.maximumValueLength == 0U), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR(((characteristic.properties & ~kValidCharacteristicProperties) != 0U), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR(((characteristic.permissions & ~kValidPermissions) != 0U), ERROR_INVALID_CONFIG);

            const bool canRead            = (characteristic.properties & HAL_BLE_GATT_PROPERTY_READ) != 0U;
            const bool canWrite           = (characteristic.properties & (HAL_BLE_GATT_PROPERTY_WRITE | HAL_BLE_GATT_PROPERTY_WRITE_WITHOUT_RSP)) != 0U;
            const bool canSignedWrite     = (characteristic.properties & HAL_BLE_GATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE) != 0U;
            const bool hasReadPermission  = (characteristic.permissions & (HAL_BLE_GATT_PERMISSION_READ | HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED)) != 0U;
            const bool hasWritePermission = (characteristic.permissions & (HAL_BLE_GATT_PERMISSION_WRITE | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED)) != 0U;

            RETURN_IF_ERROR((characteristic.properties == 0U), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((canRead != (characteristic.readCallback != nullptr)), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR(((canWrite || canSignedWrite) != (characteristic.writeCallback != nullptr)), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((canRead != hasReadPermission), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((canWrite != hasWritePermission), ERROR_INVALID_CONFIG);
            RETURN_IF_ERROR((characteristic.descriptorCount != 0U && characteristic.descriptors == nullptr), ERROR_NULL_POINTER);

            for (std::size_t descriptorIndex = 0U; descriptorIndex < characteristic.descriptorCount; ++descriptorIndex)
            {
                const hal_ble_gatt_descriptor_t& descriptor = characteristic.descriptors[descriptorIndex];
                RETURN_IF_ERROR((!_isUuidValid(descriptor.uuid)), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptor.maximumValueLength == 0U), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR(((descriptor.permissions & ~kValidPermissions) != 0U), ERROR_INVALID_CONFIG);

                const bool descriptorCanRead  = (descriptor.permissions & (HAL_BLE_GATT_PERMISSION_READ | HAL_BLE_GATT_PERMISSION_READ_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_READ_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_READ_AUTHORIZED)) != 0U;
                const bool descriptorCanWrite = (descriptor.permissions & (HAL_BLE_GATT_PERMISSION_WRITE | HAL_BLE_GATT_PERMISSION_WRITE_ENCRYPTED |
                                                                           HAL_BLE_GATT_PERMISSION_WRITE_AUTHENTICATED | HAL_BLE_GATT_PERMISSION_WRITE_AUTHORIZED)) != 0U;

                RETURN_IF_ERROR((descriptor.permissions == 0U), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptorCanRead != (descriptor.readCallback != nullptr)), ERROR_INVALID_CONFIG);
                RETURN_IF_ERROR((descriptorCanWrite != (descriptor.writeCallback != nullptr)), ERROR_INVALID_CONFIG);
            }
        }
    }

    return ERROR_SUCCESS;
}
