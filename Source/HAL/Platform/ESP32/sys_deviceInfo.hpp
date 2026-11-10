/** @file       sys_deviceInfo.hpp
 *  @brief      ESP32 platform implementation of IHal_Sys_DeviceInfo (eFuse-provisioned identity).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_DeviceInfo.h"
#include "System/system.h"

class sys_deviceInfo : public IHal_Sys_DeviceInfo
{
public:
    sys_deviceInfo()           = default;
    ~sys_deviceInfo() override = default;

    sys_deviceInfo(const sys_deviceInfo&)            = delete;
    sys_deviceInfo& operator=(const sys_deviceInfo&) = delete;
    sys_deviceInfo(sys_deviceInfo&&)                 = delete;
    sys_deviceInfo& operator=(sys_deviceInfo&&)      = delete;

    // Stateless driver: lifecycle hooks have no hardware state to manage.
    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t getBaseMacAddress(uint8_t* macAddress) override;
};
