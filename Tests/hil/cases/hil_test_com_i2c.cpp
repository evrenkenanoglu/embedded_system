/** @file       hil_test_com_i2c.cpp
 *  @brief      Generic HIL contract test for an I2C bus (IHAL_COM).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_com_i2c.hpp"

/** CONSTANTS *****************************************************************/
static constexpr uint8_t kAbsentAddress = 0x7E;

// Register pointer written before each read; assumes the target exposes a readable register at 0x00.
static constexpr uint8_t kProbeRegister = 0x00;

/** FUNCTIONS *****************************************************************/
sys_error_t hil_test_com_i2c_contract(IHAL_COM& bus, uint8_t expectedTargetAddress)
{
    sys_error_t err = bus.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    uint8_t probeData = 0U;

    err = bus.writeRead(&kAbsentAddress, &kProbeRegister, 1U, &probeData, 1U);
    if (err == ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }

    err = bus.writeRead(&expectedTargetAddress, &kProbeRegister, 1U, &probeData, 1U);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    return ERROR_SUCCESS;
}
