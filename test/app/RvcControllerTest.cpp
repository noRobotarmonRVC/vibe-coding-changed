#include <gtest/gtest.h>
#include <vector>
#include "app/RvcController.hpp"
#include "domain/DefaultNavigationStrategy.hpp"

// [변경] Controller tests trace RightSensor removal and FrontSensor Right Scan behavior.

// ── hand-written mocks ────────────────────────────────────────────────────────

class MockSensor : public ISensor {
public:
    bool state = false;
    bool detect() override { return state; }
};

class MockMotor : public IMotorController {
public:
    std::vector<Direction> log;
    void move(Direction d) override { log.push_back(d); }
    [[nodiscard]] Direction last() const { return log.empty() ? Direction::STOP : log.back(); }
};

class MockCleaner : public ICleanerController {
public:
    std::vector<CleanPower> log;
    void setPower(CleanPower p) override { log.push_back(p); }
    [[nodiscard]] CleanPower last() const { return log.empty() ? CleanPower::OFF : log.back(); }
};

// ── fixture ───────────────────────────────────────────────────────────────────

class RvcControllerTest : public ::testing::Test {
protected:
    // [삭제] No right sensor mock; front is reused for Right Scan results.
    MockSensor front, left, dust;
    MockMotor motor;
    MockCleaner cleaner;
    DefaultNavigationStrategy nav;
    RvcController controller{&front, &left, &dust, &motor, &cleaner, &nav};
};

// ── lifecycle ─────────────────────────────────────────────────────────────────

TEST_F(RvcControllerTest, StartMovesForwardAndCleansOn) {
    controller.start();
    EXPECT_EQ(motor.last(), Direction::FORWARD);
    EXPECT_EQ(cleaner.last(), CleanPower::ON);
}

TEST_F(RvcControllerTest, StopHaltsMotorAndTurnsOffCleaner) {
    controller.start();
    controller.stop();
    EXPECT_EQ(motor.last(), Direction::STOP);
    EXPECT_EQ(cleaner.last(), CleanPower::OFF);
}

TEST_F(RvcControllerTest, TickDoesNothingWhenIdle) {
    controller.onTick();
    EXPECT_TRUE(motor.log.empty());
    EXPECT_TRUE(cleaner.log.empty());
}

// ── dust handling ─────────────────────────────────────────────────────────────

TEST_F(RvcControllerTest, DustDetectionPowersUpCleaner) {
    controller.start();
    cleaner.log.clear();

    dust.state = true;
    controller.onTick();

    EXPECT_EQ(cleaner.last(), CleanPower::POWER_UP);
}

TEST_F(RvcControllerTest, CleaningPowerRestoresAfterIntensifyDuration) {
    controller.start();
    dust.state = true;
    controller.onTick();
    dust.state = false;

    for (int i = 0; i < RvcController::INTENSIFY_DURATION; ++i) {
        controller.onTick();
    }

    EXPECT_EQ(cleaner.last(), CleanPower::ON);
}

// ── obstacle avoidance ────────────────────────────────────────────────────────

TEST_F(RvcControllerTest, FrontObstacleOpenSidesTurnsLeft) {
    controller.start();
    motor.log.clear();
    left.state  = false;
    front.state = false;  // [추가] Right Scan reports open.

    controller.onFrontObstacleDetected();

    // STOP -> RIGHT scan -> LEFT restore -> LEFT avoid -> FORWARD
    ASSERT_GE(motor.log.size(), 5U);
    EXPECT_EQ(motor.log[0], Direction::STOP);
    EXPECT_EQ(motor.log[1], Direction::RIGHT);
    EXPECT_EQ(motor.log[2], Direction::LEFT);
    EXPECT_EQ(motor.log[3], Direction::LEFT);
    EXPECT_EQ(motor.log[4], Direction::FORWARD);
}

TEST_F(RvcControllerTest, FrontObstacleLeftBlockedTurnsRight) {
    controller.start();
    motor.log.clear();
    left.state  = true;
    front.state = false;  // [추가] Right Scan reports open.

    controller.onFrontObstacleDetected();

    ASSERT_GE(motor.log.size(), 5U);
    EXPECT_EQ(motor.log[0], Direction::STOP);
    EXPECT_EQ(motor.log[1], Direction::RIGHT);
    EXPECT_EQ(motor.log[2], Direction::LEFT);
    EXPECT_EQ(motor.log[3], Direction::RIGHT);
    EXPECT_EQ(motor.log[4], Direction::FORWARD);
}

TEST_F(RvcControllerTest, SurroundedEscapesBackwardThenLeftThenForwardByTick) {
    controller.start();
    motor.log.clear();
    left.state  = true;
    front.state = true;  // [추가] Right Scan reports blocked.

    controller.onFrontObstacleDetected();

    // [변경] STOP -> RIGHT scan -> LEFT restore, then BACKWARD -> LEFT -> FORWARD by tick.
    ASSERT_GE(motor.log.size(), 3U);
    EXPECT_EQ(motor.log[0], Direction::STOP);
    EXPECT_EQ(motor.log[1], Direction::RIGHT);
    EXPECT_EQ(motor.log[2], Direction::LEFT);

    controller.onTick();
    ASSERT_GE(motor.log.size(), 4U);
    EXPECT_EQ(motor.log[3], Direction::BACKWARD);

    controller.onTick();
    ASSERT_GE(motor.log.size(), 5U);
    EXPECT_EQ(motor.log[4], Direction::LEFT);

    controller.onTick();
    ASSERT_GE(motor.log.size(), 6U);
    EXPECT_EQ(motor.log[5], Direction::FORWARD);
}

TEST_F(RvcControllerTest, FrontObstacleIgnoredWhenIdle) {
    controller.onFrontObstacleDetected();
    EXPECT_TRUE(motor.log.empty());
}
