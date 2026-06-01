#include <gtest/gtest.h>
#include "domain/DefaultNavigationStrategy.hpp"

// [변경] is_right_blocked now represents Right Scan blocked, not RightSensor polling.

class DefaultNavigationStrategyTest : public ::testing::Test {
protected:
    DefaultNavigationStrategy strategy;
};

TEST_F(DefaultNavigationStrategyTest, ForwardWhenAllClear) {
    SensorData data;
    EXPECT_EQ(strategy.navigate(data), Direction::FORWARD);
}

TEST_F(DefaultNavigationStrategyTest, LeftWhenFrontBlockedAndBothSidesOpen) {
    SensorData data;
    data.is_front_blocked = true;
    EXPECT_EQ(strategy.navigate(data), Direction::LEFT);
}

TEST_F(DefaultNavigationStrategyTest, RightWhenFrontAndLeftBlocked) {
    SensorData data;
    data.is_front_blocked = true;
    data.is_left_blocked  = true;
    EXPECT_EQ(strategy.navigate(data), Direction::RIGHT);
}

TEST_F(DefaultNavigationStrategyTest, LeftWhenFrontAndRightBlocked) {
    SensorData data;
    data.is_front_blocked = true;
    data.is_right_blocked = true;
    EXPECT_EQ(strategy.navigate(data), Direction::LEFT);
}

TEST_F(DefaultNavigationStrategyTest, BackwardWhenSurrounded) {
    SensorData data;
    data.is_front_blocked = true;
    data.is_left_blocked  = true;
    data.is_right_blocked = true;
    EXPECT_EQ(strategy.navigate(data), Direction::BACKWARD);
}

TEST_F(DefaultNavigationStrategyTest, DustAloneDoesNotAffectDirection) {
    SensorData data;
    data.has_dust = true;
    EXPECT_EQ(strategy.navigate(data), Direction::FORWARD);
}
