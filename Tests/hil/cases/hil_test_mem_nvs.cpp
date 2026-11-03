/** @file       hil_test_mem_nvs.cpp
 *  @brief      Generic HIL contract test for a key-value memory device (IHAL_MEM).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_mem_nvs.hpp"

#include <cstring>

/** CONSTANTS *****************************************************************/

// Key is a NUL-terminated string, as expected by key-value backends such as NVS.
static const char kProbeKey[] = "hil_probe";

static constexpr uint8_t kPattern[8] = {0xA5, 0x5A, 0x00, 0xFF, 0x12, 0x34, 0x56, 0x78};

/** FUNCTIONS *****************************************************************/
sys_error_t hil_test_mem_nvs_contract(IHAL_MEM& mem)
{
    sys_error_t err = mem.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    uint8_t readBack[sizeof(kPattern)] = {0};

    err = mem.writeData(kProbeKey, kPattern, sizeof(kPattern));
    if (err == ERROR_SUCCESS)
    {
        err = mem.readData(kProbeKey, readBack, sizeof(readBack));
        if (err == ERROR_SUCCESS && std::memcmp(kPattern, readBack, sizeof(kPattern)) != 0)
        {
            err = ERROR_DATA_CORRUPTED;
        }
    }

    const sys_error_t eraseErr = mem.erase(kProbeKey);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    if (eraseErr != ERROR_SUCCESS)
    {
        return eraseErr;
    }

    if (mem.readData(kProbeKey, readBack, sizeof(readBack)) == ERROR_SUCCESS)
    {
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}
