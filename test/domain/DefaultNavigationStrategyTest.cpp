#include <gtest/gtest.h>
#include "domain/DefaultNavigationStrategy.hpp"

class DefaultNavigationStrategyTest : public ::testing::Test {
protected:
    DefaultNavigationStrategy strategy;
};

TEST_F(DefaultNavigationStrategyTest, ForwardWhenAllClear) {
    SensorData data;
    EXPECT_EQ(strategy.navigate(data), Direction::FORWARD);
}

TEST_F(DefaultNavigationStrategyTest, LeftWhenFrontBlockedLeftOpen) {
    SensorData data;
    data.is_front_blocked = true;
    EXPECT_EQ(strategy.navigate(data), Direction::LEFT);
}

// No right sensor: front + left blocked yields BACKWARD, which signals the
// controller to probe the right side (rotate + front sensor) before escaping.
TEST_F(DefaultNavigationStrategyTest, BackwardWhenFrontAndLeftBlocked) {
    SensorData data;
    data.is_front_blocked = true;
    data.is_left_blocked  = true;
    EXPECT_EQ(strategy.navigate(data), Direction::BACKWARD);
}

TEST_F(DefaultNavigationStrategyTest, DustAloneDoesNotAffectDirection) {
    SensorData data;
    data.has_dust = true;
    EXPECT_EQ(strategy.navigate(data), Direction::FORWARD);
}
