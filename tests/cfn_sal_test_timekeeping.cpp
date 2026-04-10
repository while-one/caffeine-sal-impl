#include <gtest/gtest.h>
#include "utilities/cfn_sal_timekeeping.h"
#include <thread>
#include <chrono>

using namespace testing;

class TimekeepingTest : public Test
{
  protected:
    void SetUp() override
    {
        cfn_sal_timekeeping_construct(&timekeeping, NULL, NULL, NULL, NULL, NULL);
    }

    void TearDown() override
    {
        cfn_sal_timekeeping_destruct(&timekeeping);
    }

    cfn_sal_timekeeping_t timekeeping;
};

TEST_F(TimekeepingTest, GetMsIncreases)
{
    uint64_t ms1 = 0;
    uint64_t ms2 = 0;

    EXPECT_EQ(cfn_sal_timekeeping_get_ms(&timekeeping, &ms1), CFN_HAL_ERROR_OK);

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(cfn_sal_timekeeping_get_ms(&timekeeping, &ms2), CFN_HAL_ERROR_OK);
    EXPECT_GT(ms2, ms1);
}

TEST_F(TimekeepingTest, GetTimeReturnsValid)
{
    time_t now = 0;
    EXPECT_EQ(cfn_sal_timekeeping_get_time(&timekeeping, &now), CFN_HAL_ERROR_OK);
    EXPECT_GT(now, 1700000000); // Should be a recent-ish unix timestamp
}
