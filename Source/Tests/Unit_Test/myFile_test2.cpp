#include "gtest/gtest.h"

extern "C"
{
#include "myCFile.h"
}

TEST(AddTest2, PositiveNumbers)
{
    // Test adding positive numbers
    int result = myCadd(2, 3);
    ASSERT_EQ(result, 5);
}

TEST(AddTest2, NegativeNumbers)
{
    // Test adding negative numbers
    int result = myCadd(-2, -3);
    ASSERT_EQ(result, -5);
}

TEST(AddTest2, MixedNumbers)
{
    // Test adding mixed positive and negative numbers
    int result = myCadd(2, -3);
    ASSERT_EQ(result, -1);
}
