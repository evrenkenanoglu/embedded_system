/** @file       IHal_Sys_DeviceInfo.h
 *  @brief      Specialized interface for reading silicon-provisioned device identity.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

#pragma once

#include "IHal.h"

#include <cstdint>

/**
 * @class IHal_Sys_DeviceInfo
 * @brief Specialization of IHAL_SYS exposing factory-provisioned device identity.
 *
 * Consumed by the application layer (device identification, telemetry, and
 * provisioning) to obtain the hardware identity burned into the silicon,
 * without relying on any hardcoded fallback value.
 */
class IHal_Sys_DeviceInfo : public IHAL_SYS
{
public:
    /**
     * @brief Reads the factory base MAC address of the device.
     *
     * The returned address is the factory base address of the Wi-Fi station
     * interface, as provisioned in eFuse.
     *
     * @param[out] macAddress Pointer to a buffer of at least 6 bytes receiving the MAC address.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_ARG when
     *         macAddress is nullptr, otherwise the translated platform error.
     */
    virtual sys_error_t getBaseMacAddress(uint8_t* macAddress) = 0;

    virtual ~IHal_Sys_DeviceInfo() = default;
};
