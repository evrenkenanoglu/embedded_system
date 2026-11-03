/** @file       hil_test_io_gpio.hpp
 *  @brief      Generic HIL loopback test for two GPIO instances (drive and sense pins wired together).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

#pragma once

#include "HAL/IHAL/IHal_Io_Gpio.h"

/**
 * @brief Verifies GPIO output levels and toggling by sensing the driven pin on a physically wired second pin.
 *
 * The drive pin is forced LOW before returning, regardless of the outcome.
 *
 * @param driveGpio GPIO instance configured as the output driver.
 * @param senseGpio GPIO instance wired to driveGpio and read as input.
 * @return sys_error_t ERROR_SUCCESS on pass, otherwise the failing error code.
 */
sys_error_t hil_test_io_gpio_contract(IHAL_IO_GPIO& driveGpio, IHAL_IO_GPIO& senseGpio);
