/** @file       hil_test_mem_ota_slots.hpp
 *  @brief      Generic HIL contract test for the OTA application slots (ota_0 / ota_1) through IHal_Mem_Partition.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal_Mem_Partition.h"

/**
 * @brief Verifies both OTA application slots without writing flash.
 *
 * Checks that each slot resolves with a 64 KB aligned, non-zero size, that the slots do not overlap,
 * that at least one slot holds an ESP-IDF application image, and that the other slot is either erased
 * or holds an application image.
 *
 * @param partitions Partition device instance; init is performed by the test.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_mem_ota_slots_contract(IHal_Mem_Partition& partitions);
