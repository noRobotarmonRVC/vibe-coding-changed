#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include "simulator/Simulator.hpp"

// [추가] Integrated Claude design coverage for edge-triggered probe and escape.

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

// Front obstacle with the left side open: rotate left and resume forward.
TEST(SimulatorTest, FrontObstacleAvoidanceResumesForward) {
    Simulator sim(20, 12, {5, 5}, Heading::EAST);
    sim.placeObstacle(6, 5);  // directly ahead (EAST)
    sim.start();

    for (int i = 0; i < 4; ++i) { sim.tick(); }

    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
}

// Front + left + right(probed) all blocked: rotate right to probe, face back,
// then back up and eventually resume forward.
TEST(SimulatorTest, SurroundedEscapeBacksUpThenResumesForward) {
    Simulator sim(20, 12, {5, 5}, Heading::EAST);
    sim.placeObstacle(6, 5);  // front  (EAST)
    sim.placeObstacle(5, 4);  // left   (NORTH)
    sim.placeObstacle(5, 6);  // right  (SOUTH, found via the probe)
    sim.start();

    for (int i = 0; i < 8; ++i) { sim.tick(); }

    const auto& log = sim.motorLog();
    EXPECT_NE(std::find(log.begin(), log.end(), Direction::BACKWARD), log.end());
    EXPECT_EQ(sim.lastDirection(), Direction::FORWARD);
}

// Regression: backing up must never move more than one cell in a single tick.
TEST(SimulatorTest, NeverMovesMoreThanOneCellPerTick) {
    Simulator sim(20, 12, {5, 5}, Heading::EAST);
    sim.placeObstacle(6, 5);
    sim.placeObstacle(5, 4);
    sim.placeObstacle(5, 6);
    sim.start();

    Position prev = sim.pos();
    for (int i = 0; i < 12; ++i) {
        sim.tick();
        Position now = sim.pos();
        int manhattan = std::abs(now.x - prev.x) + std::abs(now.y - prev.y);
        EXPECT_LE(manhattan, 1);
        prev = now;
    }
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

// Edge-triggered interrupt: STOP is issued once even while the front stays blocked.
TEST(SimulatorTest, FrontInterruptFiresOnceWhileStuck) {
    Simulator sim(20, 12, {5, 5}, Heading::EAST);
    // fully boxed in: front, left, right (probed), and behind all blocked
    sim.placeObstacle(6, 5);  // front  (EAST)
    sim.placeObstacle(5, 4);  // left   (NORTH)
    sim.placeObstacle(5, 6);  // right  (SOUTH)
    sim.placeObstacle(4, 5);  // behind (WEST)
    sim.start();

    for (int i = 0; i < 10; ++i) { sim.tick(); }

    const auto& log = sim.motorLog();
    EXPECT_EQ(std::count(log.begin(), log.end(), Direction::STOP), 1);
}

// Dead-end corridor: backs up over multiple ticks (one cell each) to escape.
TEST(SimulatorTest, DeadEndBacksUpMultipleTimes) {
    Simulator sim(20, 12, {5, 5}, Heading::EAST);
    sim.placeObstacle(6, 5);                                    // front
    sim.placeObstacle(4, 4); sim.placeObstacle(5, 4); sim.placeObstacle(6, 4);  // north wall
    sim.placeObstacle(4, 6); sim.placeObstacle(5, 6); sim.placeObstacle(6, 6);  // south wall
    sim.start();

    for (int i = 0; i < 12; ++i) { sim.tick(); }

    const auto& log = sim.motorLog();
    EXPECT_GE(std::count(log.begin(), log.end(), Direction::BACKWARD), 2);
}

// After probing right and finding it blocked, the RVC restores its original
// heading before backing up — so it reverses opposite to the start heading.
TEST(SimulatorTest, BacksUpAlongOriginalHeadingAfterProbe) {
    Simulator sim(20, 12, {5, 5}, Heading::EAST);
    sim.placeObstacle(6, 5);  // front
    sim.placeObstacle(5, 4);  // left
    sim.placeObstacle(5, 6);  // right (probed)
    sim.start();

    Position start = sim.pos();
    for (int i = 0; i < 5; ++i) { sim.tick(); }

    // reversed westward (opposite of original EAST heading)
    EXPECT_LT(sim.pos().x, start.x);
    EXPECT_EQ(sim.pos().y, start.y);
}
