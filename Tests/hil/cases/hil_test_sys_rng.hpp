/** @file       hil_test_sys_rng.hpp
 *  @brief      Generic HIL contract test for IHal_Sys_Rng.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Rng.h"

/**
 * @brief Verifies the IHal_Sys_Rng contract (argument validation, output sanity, range bounds).
 *
 * @param rng Initialized-on-entry random number generator instance.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_sys_rng_contract(IHal_Sys_Rng& rng);
