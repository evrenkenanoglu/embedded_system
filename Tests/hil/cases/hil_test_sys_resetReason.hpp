/** @file       hil_test_sys_resetReason.hpp
 *  @brief      Generic HIL contract test for IHal_Sys_ResetReason.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_ResetReason.h"

/**
 * @brief Verifies the IHal_Sys_ResetReason contract (valid enum, non-empty string, abnormal-reset consistency).
 *
 * @param resetReason Reset reason instance; init is performed by the test.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_sys_resetReason_contract(IHal_Sys_ResetReason& resetReason);
