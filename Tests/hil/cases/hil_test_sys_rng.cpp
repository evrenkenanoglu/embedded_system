/** @file       hil_test_sys_rng.cpp
 *  @brief      Generic HIL contract test for IHal_Sys_Rng.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_sys_rng.hpp"

/** FUNCTIONS *****************************************************************/
sys_error_t hil_test_sys_rng_contract(IHal_Sys_Rng& rng)
{
    sys_error_t err = rng.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    uint8_t buffer[16] = {0};

    err = rng.getRandomBytes(nullptr, sizeof(buffer));
    if (err != ERROR_NULL_POINTER)
    {
        return ERROR_FAIL;
    }

    err = rng.getRandomBytes(buffer, 0U);
    if (err != ERROR_INVALID_ARG)
    {
        return ERROR_FAIL;
    }

    err = rng.getRandomBytes(buffer, sizeof(buffer));
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    bool anyNonZero = false;
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        anyNonZero = anyNonZero || (buffer[i] != 0U);
    }
    if (!anyNonZero)
    {
        return ERROR_DATA_CORRUPTED;
    }

    const uint32_t sampleA = rng.getRandomU32();
    const uint32_t sampleB = rng.getRandomU32();
    if (sampleA == sampleB)
    {
        return ERROR_DATA_CORRUPTED;
    }

    for (uint32_t i = 0; i < 32U; i++)
    {
        const uint32_t val = rng.getRandomRange(10U, 20U);
        if (val < 10U || val > 20U)
        {
            return ERROR_DATA_CORRUPTED;
        }
    }

    if (rng.getRandomRange(42U, 42U) != 42U)
    {
        return ERROR_DATA_CORRUPTED;
    }

    return ERROR_SUCCESS;
}
