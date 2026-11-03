/** @file       hil_test_sys_rtc.cpp
 *  @brief      Generic HIL contract test for IHal_Sys_Rtc.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 */

/** INCLUDES ******************************************************************/
#include "hil_test_sys_rtc.hpp"

#include <cstring>

/** CONSTANTS *****************************************************************/
static constexpr time_t kKnownEpoch     = 1767225600; // 2026-01-01T00:00:00Z
static constexpr time_t kEpochTolerance = 5;
static constexpr char   kTestTimezone[] = "UTC0";
static constexpr size_t kTzBufferSize   = 64U;

/** FUNCTIONS *****************************************************************/

/**
 * @brief Sets the test timezone, reads it back, and checks nullptr rejection.
 */
static sys_error_t checkTimezoneRoundTrip(IHal_Sys_Rtc& rtc)
{
    sys_error_t err = rtc.setTimezone(kTestTimezone);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    char readBack[kTzBufferSize] = {0};
    err                          = rtc.getTimezone(readBack, sizeof(readBack));
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    if (std::strcmp(readBack, kTestTimezone) != 0)
    {
        return ERROR_DATA_CORRUPTED;
    }

    if (rtc.setTimezone(nullptr) != ERROR_INVALID_ARG)
    {
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t hil_test_sys_rtc_contract(IHal_Sys_Rtc& rtc)
{
    sys_error_t err = rtc.init();
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    const uint64_t monoStart = rtc.getMonotonicTimeUs();

    err = rtc.setTime(kKnownEpoch, 0U);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    if (!rtc.isTimeSet())
    {
        return ERROR_FAIL;
    }

    time_t now = 0;
    err        = rtc.getTime(now);
    if (err != ERROR_SUCCESS)
    {
        return err;
    }
    if (now < kKnownEpoch || now > kKnownEpoch + kEpochTolerance)
    {
        return ERROR_DATA_CORRUPTED;
    }

    if (rtc.getMonotonicTimeUs() < monoStart)
    {
        return ERROR_DATA_CORRUPTED;
    }

    char originalTz[kTzBufferSize] = {0};
    err                            = rtc.getTimezone(originalTz, sizeof(originalTz));
    if (err != ERROR_SUCCESS)
    {
        return err;
    }

    const sys_error_t tzErr      = checkTimezoneRoundTrip(rtc);
    const sys_error_t restoreErr = rtc.setTimezone(originalTz);
    if (tzErr != ERROR_SUCCESS)
    {
        return tzErr;
    }

    return restoreErr;
}
