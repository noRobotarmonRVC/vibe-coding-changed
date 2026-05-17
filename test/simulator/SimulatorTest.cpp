#include <gtest/gtest.h>
#include <algorithm>
#include "simulator/Simulator.hpp"

TEST(SimulatorTest, StartMovesForwardAndCleansOn) {
    Simulator sim;
    sim.start();
    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
    EXPECT_EQ(sim.lastPower(), CleanPower::ON);
}

TEST(SimulatorTest, NormalTickIssuesNoNewCommands) {
    Simulator sim;
    sim.start();
    const auto motor_count   = sim.motorLog().size();
    const auto cleaner_count = sim.cleanerLog().size();

    sim.tick();

    EXPECT_EQ(sim.motorLog().size(),   motor_count);
    EXPECT_EQ(sim.cleanerLog().size(), cleaner_count);
}

TEST(SimulatorTest, DustPowersUpCleanerThenRestores) {
    Simulator sim;
    sim.start();

    sim.injectDust(true);
    sim.tick();
    sim.injectDust(false);
    EXPECT_EQ(sim.lastPower(), CleanPower::POWER_UP);

    for (int i = 0; i < RvcController::INTENSIFY_DURATION; ++i)
        sim.tick();

    EXPECT_EQ(sim.lastPower(), CleanPower::ON);
}

TEST(SimulatorTest, FrontObstacleAvoidanceResumesForward) {
    Simulator sim;
    sim.start();
    sim.injectLeft(false);
    sim.injectRight(false);

    sim.triggerFrontObstacle();

    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
}

TEST(SimulatorTest, SurroundedEscapeResumesForward) {
    Simulator sim;
    sim.start();
    sim.injectLeft(true);
    sim.injectRight(true);

    sim.triggerFrontObstacle();

    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);

    // verify full escape sequence: STOP → BACKWARD → LEFT → FORWARD
    const auto& log = sim.motorLog();
    auto it = std::find(log.begin(), log.end(), Direction::BACKWARD);
    ASSERT_NE(it, log.end());
    EXPECT_EQ(*(it + 1), Direction::LEFT);
    EXPECT_EQ(*(it + 2), Direction::FORWARD);
}

TEST(SimulatorTest, StopHaltsMotorAndCleaner) {
    Simulator sim;
    sim.start();
    sim.stop();
    EXPECT_EQ(sim.lastDirection(), Direction::STOP);
    EXPECT_EQ(sim.lastPower(), CleanPower::OFF);
}

TEST(SimulatorTest, TickIgnoredWhenIdle) {
    Simulator sim;
    sim.tick();
    EXPECT_TRUE(sim.motorLog().empty());
    EXPECT_TRUE(sim.cleanerLog().empty());
}
