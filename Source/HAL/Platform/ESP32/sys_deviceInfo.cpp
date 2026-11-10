/** @file       sys_deviceInfo.cpp
 *  @brief      ESP32 platform implementation of IHal_Sys_DeviceInfo (eFuse-provisioned identity).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

/** INCLUDES ******************************************************************/
#include "sys_deviceInfo.hpp"

#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_mac.h>

#include <cstdint>

/** FUNCTIONS *****************************************************************/
sys_error_t sys_deviceInfo::init(void* params)
{
    (void)params;
    return ERROR_SUCCESS;
}

sys_error_t sys_deviceInfo::feed()
{
    return ERROR_SUCCESS;
}

sys_error_t sys_deviceInfo::deInit()
{
    return ERROR_SUCCESS;
}

sys_error_t sys_deviceInfo::getBaseMacAddress(uint8_t* macAddress)
{
    /// Validate the caller-provided 6-byte output buffer.
    RETURN_IF_ERROR((macAddress == nullptr), ERROR_INVALID_ARG);

    /// Read the factory base MAC from eFuse via the Wi-Fi station interface.
    return TRANSLATE_ERROR(esp_read_mac(macAddress, ESP_MAC_WIFI_STA));
}
