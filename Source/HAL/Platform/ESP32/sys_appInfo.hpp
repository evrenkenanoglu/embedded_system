/** @file       sys_appInfo.hpp
 *  @brief      ESP32 platform implementation of IHal_Sys_AppInfo (running image descriptor).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_AppInfo.h"
#include "System/system.h"

/**
 * @class sys_appInfo
 * @brief ESP32 implementation of IHal_Sys_AppInfo.
 *
 * Stateless: the descriptor is embedded in the running image and read on demand.
 */
class sys_appInfo : public IHal_Sys_AppInfo
{
public:
    sys_appInfo();
    ~sys_appInfo() override;

    sys_appInfo(const sys_appInfo&)            = delete;
    sys_appInfo& operator=(const sys_appInfo&) = delete;
    sys_appInfo(sys_appInfo&&)                 = delete;
    sys_appInfo& operator=(sys_appInfo&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t getRunningAppInfo(hal_app_info_t* info) override;

private:
    uint8_t _initialized;
};
