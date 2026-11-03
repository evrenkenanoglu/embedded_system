/** @file       hil_test_com_i2c.hpp
 *  @brief      Generic HIL contract test for an I2C bus (IHAL_COM).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal.h"

/**
 * @brief Verifies the I2C bus contract: a probe to an absent address must fail, and a probe to the expected device must succeed.
 *
 * @param bus                    Communication bus instance; init is performed by the test.
 * @param expectedTargetAddress  7-bit address of a device that must acknowledge on the bus.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_com_i2c_contract(IHAL_COM& bus, uint8_t expectedTargetAddress);
