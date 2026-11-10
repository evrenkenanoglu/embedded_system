/** @file       macAddress.hpp
 *  @brief      Platform-independent helpers for formatting MAC addresses.
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
 * @brief Formats a 6-byte MAC address as an uppercase colon-separated string.
 *
 * The caller guarantees that macAddress points to at least 6 readable bytes.
 * A nullptr input returns an empty string.
 *
 * @param[in] macAddress Pointer to the 6-byte MAC address.
 *
 * @return std::string The address in "AA:BB:CC:DD:EE:FF" form, or an empty string on null input.
 */
std::string macAddressToString(const uint8_t* macAddress);
