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

// [추가] 막다른 통로 탈출 — 맵 기반 회귀 테스트.
// front interrupt가 Right Scan(CHECKING_RIGHT)을 가로채면 후진 연쇄가 끊겨
// 출구가 있어도 통로 안에서 진동했다. interrupt를 정상 주행 중에만 발화하도록
// 고친 뒤, 아래 맵들에서 로봇이 실제로 통로를 빠져나가야 한다.

// 통로 끝에서 좌측(들어온 길 옆)이 트인 경우: 후진해 통로를 되짚어
// 나오다 좌측 출구로 빠진다.
//   .. <      (로봇 (2,0), WEST)
//   X X .      (2,1)만 열린 출구
TEST(SimulatorTest, EscapesDeadEndCorridorThroughLeftGap) {
    Simulator sim(3, 2, {2, 0}, Heading::WEST);
    sim.placeObstacle(0, 1);
    sim.placeObstacle(1, 1);
    // (2,1) is the only gap
    sim.start();

    bool escaped = false;
    for (int i = 0; i < 20; ++i) {
        sim.tick();
        if (sim.pos().y == 1) { escaped = true; break; }  // left the corridor row
    }
    EXPECT_TRUE(escaped);
    EXPECT_EQ(sim.pos().x, 2);
    EXPECT_EQ(sim.pos().y, 1);
}

// 통로 끝에서 우측이 트인(L자) 경우: 후진하다 우측 출구를 probe로 찾아 빠진다.
//   X . . .
//   . . . <   (로봇 (3,1), WEST)
//   X X X X
TEST(SimulatorTest, EscapesDeadEndCorridorThroughRightGap) {
    Simulator sim(4, 3, {3, 1}, Heading::WEST);
    sim.placeObstacle(0, 0);
    sim.placeObstacle(0, 2); sim.placeObstacle(1, 2);
    sim.placeObstacle(2, 2); sim.placeObstacle(3, 2);
    sim.start();

    bool escaped = false;
    for (int i = 0; i < 20; ++i) {
        sim.tick();
        if (sim.pos().y == 0) { escaped = true; break; }  // climbed out to the top row
    }
    EXPECT_TRUE(escaped);
}

// 긴 통로: 왼쪽 끝까지 갔다가 통로 전체를 되짚어 후진한 뒤 맨 끝 출구로 탈출.
//   . . . . <    (로봇 (4,0), WEST)
//   X X X X .    (4,1)만 열린 출구
TEST(SimulatorTest, BacksUpFullCorridorThenEscapes) {
    Simulator sim(5, 2, {4, 0}, Heading::WEST);
    for (int x = 0; x < 4; ++x) { sim.placeObstacle(x, 1); }  // (4,1) left open
    sim.start();

    bool escaped = false;
    for (int i = 0; i < 30; ++i) {
        sim.tick();
        if (sim.pos().y == 1) { escaped = true; break; }
    }
    EXPECT_TRUE(escaped);

    // 통로를 되짚어 나오는 동안 여러 칸을 연속 후진했어야 한다.
    const auto& log = sim.motorLog();
    EXPECT_GE(std::count(log.begin(), log.end(), Direction::BACKWARD), 3);
}

// 회귀: 회피 시퀀스 중 회전이 front interrupt를 발화시켜도 후진 연쇄가
// 끊기지 않아야 한다. interrupt가 Right Scan을 가로채던 버그에서는 후진이
// 한두 번 만에 끊겼다 — 통로를 따라 여러 칸 후진이 이어져야 한다.
TEST(SimulatorTest, AvoidanceInterruptDoesNotBreakBackupChain) {
    Simulator sim(5, 2, {4, 0}, Heading::WEST);
    for (int x = 0; x < 5; ++x) { sim.placeObstacle(x, 1); }  // fully closed corridor
    sim.start();

    // 왼쪽 끝(x==0)에서 막힌 뒤 후진으로 오른쪽 끝(x==4)까지 되짚어 나왔는지 추적.
    bool hit_left_end = false;
    bool backed_to_right_end = false;
    for (int i = 0; i < 30; ++i) {
        sim.tick();
        if (sim.pos().x == 0) { hit_left_end = true; }
        if (hit_left_end && sim.pos().x == 4) { backed_to_right_end = true; }
    }

    EXPECT_TRUE(backed_to_right_end);  // 통로를 끝까지 되짚어 후진
    const auto& log = sim.motorLog();
    EXPECT_GE(std::count(log.begin(), log.end(), Direction::BACKWARD), 3);
}
