/** @file       deviceApSsid.hpp
 *  @brief      Platform-independent helper for composing a device-unique soft-AP SSID.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Composes "<productName>-<last 2 MAC bytes as four uppercase hex characters>".
 *
 * For example, "SmartPlug-1A2B" for a base MAC ending in 0x1A 0x2B. The caller is responsible for
 * checking the result against the platform's SSID length limit.
 *
 * @param[in] productName Product name, NUL-terminated.
 * @param[in] baseMac     Pointer to the 6-byte base MAC address.
 *
 * @return std::string The SSID, or an empty string if productName or baseMac is nullptr.
 */
std::string buildDeviceApSsid(const char* productName, const uint8_t* baseMac);
