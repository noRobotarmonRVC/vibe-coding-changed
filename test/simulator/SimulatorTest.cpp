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

// [추가] The console simulation clock should advance only during an active run.
TEST(SimulatorTest, RunningStateChangesOnlyByStartAndStop) {
    Simulator sim;
    EXPECT_FALSE(sim.isRunning());

    sim.start();
    EXPECT_TRUE(sim.isRunning());

    sim.stop();
    EXPECT_FALSE(sim.isRunning());
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

// [추가] Dead-end corridor regressions. False front interrupts raised during
// Right Scan must not break the multi-tick backward escape chain.
TEST(SimulatorTest, EscapesDeadEndCorridorThroughLeftGap) {
    Simulator sim(3, 2, {2, 0}, Heading::WEST);
    sim.placeObstacle(0, 1);
    sim.placeObstacle(1, 1);
    sim.start();

    bool escaped = false;
    for (int i = 0; i < 20; ++i) {
        sim.tick();
        if (sim.pos().y == 1) {
            escaped = true;
            break;
        }
    }

    EXPECT_TRUE(escaped);
    EXPECT_EQ(sim.pos().x, 2);
    EXPECT_EQ(sim.pos().y, 1);
}

TEST(SimulatorTest, EscapesDeadEndCorridorThroughRightGap) {
    Simulator sim(4, 3, {3, 1}, Heading::WEST);
    sim.placeObstacle(0, 0);
    sim.placeObstacle(0, 2);
    sim.placeObstacle(1, 2);
    sim.placeObstacle(2, 2);
    sim.placeObstacle(3, 2);
    sim.start();

    bool escaped = false;
    for (int i = 0; i < 20; ++i) {
        sim.tick();
        if (sim.pos().y == 0) {
            escaped = true;
            break;
        }
    }

    EXPECT_TRUE(escaped);
}

TEST(SimulatorTest, BacksUpFullCorridorThenEscapes) {
    Simulator sim(5, 2, {4, 0}, Heading::WEST);
    for (int x = 0; x < 4; ++x) {
        sim.placeObstacle(x, 1);
    }
    sim.start();

    bool escaped = false;
    for (int i = 0; i < 30; ++i) {
        sim.tick();
        if (sim.pos().y == 1) {
            escaped = true;
            break;
        }
    }

    EXPECT_TRUE(escaped);
    const auto& log = sim.motorLog();
    EXPECT_GE(std::count(log.begin(), log.end(), Direction::BACKWARD), 3);
}

TEST(SimulatorTest, AvoidanceInterruptDoesNotBreakBackupChain) {
    Simulator sim(5, 2, {4, 0}, Heading::WEST);
    for (int x = 0; x < 5; ++x) {
        sim.placeObstacle(x, 1);
    }
    sim.start();

    bool hit_left_end = false;
    bool backed_to_right_end = false;
    for (int i = 0; i < 30; ++i) {
        sim.tick();
        if (sim.pos().x == 0) {
            hit_left_end = true;
        }
        if (hit_left_end && sim.pos().x == 4) {
            backed_to_right_end = true;
        }
    }

    EXPECT_TRUE(backed_to_right_end);
    const auto& log = sim.motorLog();
    EXPECT_GE(std::count(log.begin(), log.end(), Direction::BACKWARD), 3);
}
