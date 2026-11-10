/** @file       macAddress.cpp
 *  @brief      Platform-independent helpers for formatting MAC addresses.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

/** INCLUDES ******************************************************************/
#include "macAddress.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr std::size_t kMacBufferSize = 18U;
    constexpr int         kMacTextLength = 17;
} // namespace

/** FUNCTIONS *****************************************************************/
std::string macAddressToString(const uint8_t* macAddress)
{
    if (macAddress == nullptr)
    {
        return std::string();
    }

    char      buffer[kMacBufferSize];
    const int written =
        std::snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    if (written != kMacTextLength)
    {
        return std::string();
    }

    return std::string(buffer);
}
