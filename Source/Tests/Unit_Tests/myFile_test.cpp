#include "myFile.h"
#include "gtest/gtest.h"

TEST(AddTest, PositiveNumbers)
{
    // Test adding positive numbers
    int result = add(2, 3);
    ASSERT_EQ(result, 5);
}

TEST(AddTest, NegativeNumbers)
{
    // Test adding negative numbers
    int result = add(-2, -3);
    ASSERT_EQ(result, -5);
}

TEST(AddTest, MixedNumbers)
{
    // Test adding mixed positive and negative numbers
    int result = add(2, -3);
    ASSERT_EQ(result, -1);
}
