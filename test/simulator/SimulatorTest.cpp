#include <gtest/gtest.h>
#include "simulator/Simulator.hpp"

// [변경] Simulator tests trace Right Scan injection and one-cell-per-tick escape.

TEST(SimulatorTest, StartMovesForwardAndCleansOn) {
    Simulator sim;
    sim.start();
    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
    EXPECT_EQ(sim.lastPower(), CleanPower::ON);
}

TEST(SimulatorTest, NormalTickReissuesForward) {
    Simulator sim;
    sim.start();
    const auto motor_count   = sim.motorLog().size();
    const auto cleaner_count = sim.cleanerLog().size();

    sim.tick();

    EXPECT_EQ(sim.motorLog().size(),   motor_count + 1);
    EXPECT_EQ(sim.cleanerLog().size(), cleaner_count);
}

TEST(SimulatorTest, DustPowersUpCleanerThenRestores) {
    Simulator sim;
    sim.start();

    sim.placeDust(sim.pos().x, sim.pos().y);
    sim.tick();
    EXPECT_EQ(sim.lastPower(), CleanPower::POWER_UP);

    for (int i = 0; i < RvcController::INTENSIFY_DURATION; ++i) {
        sim.tick();
    }

    EXPECT_EQ(sim.lastPower(), CleanPower::ON);
}

TEST(SimulatorTest, FrontObstacleAvoidanceResumesForward) {
    Simulator sim;
    sim.start();
    sim.injectLeft(false);
    sim.injectFront(false);  // [추가] Manual Right Scan result uses front injection.

    sim.triggerFrontObstacle();

    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
}

TEST(SimulatorTest, SurroundedEscapeMovesOneCellPerTick) {
    Simulator sim;
    sim.start();
    sim.injectLeft(true);
    sim.injectFront(true);  // [추가] Manual Right Scan result uses front injection.
    const Position start = sim.pos();

    sim.triggerFrontObstacle();

    EXPECT_EQ(sim.pos().x, start.x);
    EXPECT_EQ(sim.pos().y, start.y);

    // [변경] Escape advances one movement command per tick.
    sim.tick();
    EXPECT_EQ(sim.lastDirection(), Direction::BACKWARD);
    EXPECT_EQ(sim.pos().x, start.x - 1);
    EXPECT_EQ(sim.pos().y, start.y);

    const Position after_backward = sim.pos();
    sim.tick();
    EXPECT_EQ(sim.lastDirection(), Direction::LEFT);
    EXPECT_EQ(sim.pos().x, after_backward.x);
    EXPECT_EQ(sim.pos().y, after_backward.y);

    sim.tick();
    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
    EXPECT_EQ(sim.pos().x, after_backward.x);
    EXPECT_EQ(sim.pos().y, after_backward.y - 1);
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
