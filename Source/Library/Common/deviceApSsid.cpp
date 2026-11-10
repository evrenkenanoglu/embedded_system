/** @file       deviceApSsid.cpp
 *  @brief      Platform-independent helper for composing a device-unique soft-AP SSID.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

/** INCLUDES ******************************************************************/
#include "deviceApSsid.hpp"

#include <cstdio>

/** FUNCTIONS *****************************************************************/
std::string buildDeviceApSsid(const char* productName, const uint8_t* baseMac)
{
    if (productName == nullptr || baseMac == nullptr)
    {
        return std::string();
    }

    char suffix[5] = {};
    std::snprintf(suffix, sizeof(suffix), "%02X%02X", baseMac[4], baseMac[5]);

    return std::string(productName) + "-" + suffix;
}
