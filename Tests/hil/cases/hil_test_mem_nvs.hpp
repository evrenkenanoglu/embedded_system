/** @file       hil_test_mem_nvs.hpp
 *  @brief      Generic HIL contract test for a key-value memory device (IHAL_MEM).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal.h"

/**
 * @brief Verifies the IHAL_MEM key-value contract: write/read round-trip, erase, and read-after-erase failure.
 *
 * The test key is erased before returning, regardless of the outcome.
 *
 * @param mem Memory device instance; init is performed by the test.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_mem_nvs_contract(IHAL_MEM& mem);
