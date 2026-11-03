/** @file       hil_test_sys_rtc.hpp
 *  @brief      Generic HIL contract test for IHal_Sys_Rtc.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Rtc.h"

/**
 * @brief Verifies the IHal_Sys_Rtc contract (wall-clock set/get, monotonic progress, timezone round-trip).
 *
 * Sets the wall clock to a fixed known epoch and restores the original timezone before returning.
 *
 * @param rtc RTC instance; init is performed by the test.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_sys_rtc_contract(IHal_Sys_Rtc& rtc);
