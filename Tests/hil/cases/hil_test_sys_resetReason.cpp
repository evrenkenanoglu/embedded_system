/** @file       hil_test_sys_resetReason.cpp
 *  @brief      Generic HIL contract test for IHal_Sys_ResetReason.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_sys_resetReason.hpp"

/** FUNCTIONS *****************************************************************/

/**
 * @brief Abnormal resets per IHal_Sys_ResetReason::isAbnormalReset contract.
 */
static bool isAbnormalByContract(hal_reset_reason_t reason)
{
    switch (reason)
    {
        case hal_reset_reason_t::PANIC_CRASH:
        case hal_reset_reason_t::TASK_WATCHDOG:
        case hal_reset_reason_t::INTERRUPT_WATCHDOG:
        case hal_reset_reason_t::BROWNOUT:
        case hal_reset_reason_t::CPU_LOCKUP:
            return true;
        default:
            return false;
    }
}

sys_error_t hil_test_sys_resetReason_contract(IHal_Sys_ResetReason& resetReason)
{
    sys_error_t err = resetReason.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    const hal_reset_reason_t reason = resetReason.getResetReason();
    if (static_cast<uint8_t>(reason) > static_cast<uint8_t>(hal_reset_reason_t::CPU_LOCKUP))
    {
        return ERROR_DATA_CORRUPTED;
    }

    const char* text = resetReason.getResetReasonString();
    if (text == nullptr || text[0] == '\0')
    {
        return ERROR_FAIL;
    }

    if (resetReason.isAbnormalReset() != isAbnormalByContract(reason))
    {
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}
