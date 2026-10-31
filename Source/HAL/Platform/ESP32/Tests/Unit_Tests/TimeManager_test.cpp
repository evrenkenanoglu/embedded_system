/** @file       TimeManager_test.cpp
 *  @brief      Unit tests for TimeManager (sync gating, window wrap-around, certificate validity).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#include "gtest/gtest.h"

#include "App/Time/TimeManager.hpp"
#include "fake_sys_rtc.hpp"

#include <cstdlib>
#include <ctime>

/*** CONSTANTS ***/
namespace
{
    /// @brief 2026-01-01T00:00:00Z
    constexpr time_t kNewYear2026 = 1767225600;

    time_t atUtcHour(uint32_t hour)
    {
        return kNewYear2026 + static_cast<time_t>(hour) * 3600;
    }
} // namespace
/*** CONSTANTS ***/

/*** STUBS ***/
class StubSntpClient : public ISntpClient
{
public:
    sys_error_t init(const sntp_client_config_t& config) override
    {
        (void)config;
        return ERROR_SUCCESS;
    }
    sys_error_t deInit() override
    {
        return ERROR_SUCCESS;
    }
    sys_error_t start() override
    {
        return ERROR_SUCCESS;
    }
    sys_error_t stop() override
    {
        return ERROR_SUCCESS;
    }
    sys_error_t triggerSync() override
    {
        return ERROR_SUCCESS;
    }
    sntp_sync_status_t getSyncStatus() override
    {
        return status;
    }
    sys_error_t setSyncCallback(sntp_sync_cb_t callback, void* userData) override
    {
        (void)callback;
        (void)userData;
        return ERROR_SUCCESS;
    }

    sntp_sync_status_t status = sntp_sync_status_t::IN_PROGRESS;
};
/*** STUBS ***/

/*** TESTS ***/
class TimeManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Pin the local zone to UTC so hour-of-day assertions are deterministic. */
        setenv("TZ", "UTC0", 1);
        tzset();
        ASSERT_EQ(rtc_.init(), ERROR_SUCCESS);
        ASSERT_EQ(rtc_.setTime(atUtcHour(0)), ERROR_SUCCESS);
        sntp_.status = sntp_sync_status_t::COMPLETED;
    }

    fake_sys_rtc   rtc_;
    StubSntpClient sntp_;
};

TEST_F(TimeManagerTest, NotSynchronizedWhenSntpHasNotCompleted)
{
    TimeManager tm(rtc_, sntp_);
    sntp_.status = sntp_sync_status_t::IN_PROGRESS;

    EXPECT_FALSE(tm.isTimeSynchronized());
}

TEST_F(TimeManagerTest, NotSynchronizedWhenRtcEpochIsImplausible)
{
    TimeManager tm(rtc_, sntp_);
    ASSERT_EQ(rtc_.setTime(0), ERROR_SUCCESS);

    EXPECT_FALSE(tm.isTimeSynchronized());
}

TEST_F(TimeManagerTest, SynchronizedWhenSntpCompletedAndRtcPlausible)
{
    TimeManager tm(rtc_, sntp_);

    EXPECT_TRUE(tm.isTimeSynchronized());
}

TEST_F(TimeManagerTest, CertificateInvalidWhenUnsynchronizedEvenInsideRange)
{
    TimeManager tm(rtc_, sntp_);
    sntp_.status = sntp_sync_status_t::IN_PROGRESS;

    EXPECT_FALSE(tm.isCertificateValid(kNewYear2026 - 100, kNewYear2026 + 100));
}

TEST_F(TimeManagerTest, CertificateValidityIsInclusiveAtBoundaries)
{
    TimeManager  tm(rtc_, sntp_);
    const time_t notBefore = kNewYear2026;
    const time_t notAfter  = kNewYear2026 + 86400;

    ASSERT_EQ(rtc_.setTime(notBefore - 1), ERROR_SUCCESS);
    EXPECT_FALSE(tm.isCertificateValid(notBefore, notAfter));

    ASSERT_EQ(rtc_.setTime(notBefore), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isCertificateValid(notBefore, notAfter));

    ASSERT_EQ(rtc_.setTime(notAfter), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isCertificateValid(notBefore, notAfter));

    ASSERT_EQ(rtc_.setTime(notAfter + 1), ERROR_SUCCESS);
    EXPECT_FALSE(tm.isCertificateValid(notBefore, notAfter));
}

TEST_F(TimeManagerTest, CertificateRejectsInvertedRange)
{
    TimeManager tm(rtc_, sntp_);

    EXPECT_FALSE(tm.isCertificateValid(kNewYear2026 + 10, kNewYear2026));
}

TEST_F(TimeManagerTest, MaintenanceWindowWrapsAcrossMidnight)
{
    TimeManager tm(rtc_, sntp_);

    ASSERT_EQ(rtc_.setTime(atUtcHour(23)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(23, 3));

    ASSERT_EQ(rtc_.setTime(atUtcHour(0)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(23, 3));

    ASSERT_EQ(rtc_.setTime(atUtcHour(2)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(23, 3));

    ASSERT_EQ(rtc_.setTime(atUtcHour(3)), ERROR_SUCCESS);
    EXPECT_FALSE(tm.isWithinMaintenanceWindow(23, 3));

    ASSERT_EQ(rtc_.setTime(atUtcHour(12)), ERROR_SUCCESS);
    EXPECT_FALSE(tm.isWithinMaintenanceWindow(23, 3));
}

TEST_F(TimeManagerTest, MaintenanceWindowPlainRangeExcludesEndHour)
{
    TimeManager tm(rtc_, sntp_);

    ASSERT_EQ(rtc_.setTime(atUtcHour(9)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(9, 17));

    ASSERT_EQ(rtc_.setTime(atUtcHour(16)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(9, 17));

    ASSERT_EQ(rtc_.setTime(atUtcHour(17)), ERROR_SUCCESS);
    EXPECT_FALSE(tm.isWithinMaintenanceWindow(9, 17));

    ASSERT_EQ(rtc_.setTime(atUtcHour(8)), ERROR_SUCCESS);
    EXPECT_FALSE(tm.isWithinMaintenanceWindow(9, 17));
}

TEST_F(TimeManagerTest, MaintenanceWindowEqualBoundsCoversFullDay)
{
    TimeManager tm(rtc_, sntp_);

    ASSERT_EQ(rtc_.setTime(atUtcHour(0)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(5, 5));

    ASSERT_EQ(rtc_.setTime(atUtcHour(23)), ERROR_SUCCESS);
    EXPECT_TRUE(tm.isWithinMaintenanceWindow(5, 5));
}

TEST_F(TimeManagerTest, MaintenanceWindowRejectsOutOfRangeHour)
{
    TimeManager tm(rtc_, sntp_);

    EXPECT_FALSE(tm.isWithinMaintenanceWindow(24, 3));
    EXPECT_FALSE(tm.isWithinMaintenanceWindow(23, 24));
}

TEST_F(TimeManagerTest, MaintenanceWindowFailsClosedWhenUnsynchronized)
{
    TimeManager tm(rtc_, sntp_);
    sntp_.status = sntp_sync_status_t::IN_PROGRESS;

    EXPECT_FALSE(tm.isWithinMaintenanceWindow(23, 3));
}

TEST_F(TimeManagerTest, Iso8601FormatsSynchronizedTime)
{
    TimeManager tm(rtc_, sntp_);
    ASSERT_EQ(rtc_.setTime(atUtcHour(13) + 45 * 60 + 30), ERROR_SUCCESS);

    char buffer[20] = {};
    ASSERT_EQ(tm.getIso8601String(buffer, sizeof(buffer)), ERROR_SUCCESS);
    EXPECT_STREQ(buffer, "2026-01-01T13:45:30");
}

TEST_F(TimeManagerTest, Iso8601RejectsUnsynchronizedAndShortBuffer)
{
    TimeManager tm(rtc_, sntp_);
    char        buffer[32] = {};

    EXPECT_NE(tm.getIso8601String(buffer, 10), ERROR_SUCCESS);

    sntp_.status = sntp_sync_status_t::IN_PROGRESS;
    EXPECT_EQ(tm.getIso8601String(buffer, sizeof(buffer)), ERROR_INVALID_STATE);
}

TEST_F(TimeManagerTest, LocalHourReportsSynchronizedHourAndFailsClosed)
{
    TimeManager tm(rtc_, sntp_);
    ASSERT_EQ(rtc_.setTime(atUtcHour(13) + 45 * 60), ERROR_SUCCESS);

    uint8_t hour = 0xFF;
    ASSERT_EQ(tm.getLocalHour(hour), ERROR_SUCCESS);
    EXPECT_EQ(hour, 13U);

    sntp_.status = sntp_sync_status_t::IN_PROGRESS;
    EXPECT_EQ(tm.getLocalHour(hour), ERROR_INVALID_STATE);
}
/*** TESTS ***/
