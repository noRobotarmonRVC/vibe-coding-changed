#include <gtest/gtest.h>
#include <vector>
#include "app/RvcController.hpp"
#include "domain/DefaultNavigationStrategy.hpp"

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
// No right sensor: the controller reuses the front sensor to probe the right
// side after rotating right.

class RvcControllerTest : public ::testing::Test {
protected:
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

// ── obstacle avoidance (multi-tick) ───────────────────────────────────────────
// [변경] Right probing and escape are verified across ticks, not in one event.

TEST_F(RvcControllerTest, FrontObstacleOnlyStopsThenWaits) {
    controller.start();
    motor.log.clear();

    EXPECT_TRUE(controller.onFrontObstacleDetected());  // interrupt: STOP only, enter AVOIDING

    ASSERT_EQ(motor.log.size(), 1U);
    EXPECT_EQ(motor.log[0], Direction::STOP);
}

TEST_F(RvcControllerTest, LeftOpenTurnsLeftThenResumesForward) {
    controller.start();
    motor.log.clear();
    left.state = false;  // left open

    controller.onFrontObstacleDetected();  // STOP, AVOIDING
    controller.onTick();                   // left open -> LEFT, CLEANING
    EXPECT_EQ(motor.last(), Direction::LEFT);

    controller.onTick();                   // CLEANING -> FORWARD
    EXPECT_EQ(motor.last(), Direction::FORWARD);
}

TEST_F(RvcControllerTest, LeftBlockedRightOpenProbesRightThenResumes) {
    controller.start();
    motor.log.clear();
    left.state = true;   // left blocked -> must probe right

    controller.onFrontObstacleDetected();  // STOP, AVOIDING
    controller.onTick();                   // left blocked -> RIGHT (probe), CHECKING_RIGHT
    EXPECT_EQ(motor.last(), Direction::RIGHT);

    front.state = false;                   // right side (now front) is open
    controller.onTick();                   // CHECKING_RIGHT -> CLEANING (already facing right)
    controller.onTick();                   // CLEANING -> FORWARD
    EXPECT_EQ(motor.last(), Direction::FORWARD);
}

TEST_F(RvcControllerTest, SurroundedProbesRightThenBacksUpOneCell) {
    controller.start();
    motor.log.clear();
    left.state = true;

    controller.onFrontObstacleDetected();  // STOP, AVOIDING
    controller.onTick();                   // RIGHT (probe), CHECKING_RIGHT
    front.state = true;                    // right side also blocked -> surrounded
    controller.onTick();                   // LEFT (face back), ESCAPING
    controller.onTick();                   // BACKWARD (one cell), back to AVOIDING

    // STOP -> RIGHT -> LEFT -> BACKWARD, with exactly one BACKWARD per escape
    ASSERT_EQ(motor.log.size(), 4U);
    EXPECT_EQ(motor.log[0], Direction::STOP);
    EXPECT_EQ(motor.log[1], Direction::RIGHT);
    EXPECT_EQ(motor.log[2], Direction::LEFT);
    EXPECT_EQ(motor.log[3], Direction::BACKWARD);
}

TEST_F(RvcControllerTest, FrontObstacleIgnoredWhenIdle) {
    EXPECT_FALSE(controller.onFrontObstacleDetected());
    EXPECT_TRUE(motor.log.empty());
}

// A front obstacle during INTENSIFYING still stops and enters avoidance.
TEST_F(RvcControllerTest, IntensifyingInterruptedByFrontObstacle) {
    controller.start();
    dust.state = true;
    controller.onTick();                   // CLEANING -> INTENSIFYING (POWER_UP)
    ASSERT_EQ(cleaner.last(), CleanPower::POWER_UP);
    motor.log.clear();

    controller.onFrontObstacleDetected();  // interrupt while intensifying
    EXPECT_EQ(motor.last(), Direction::STOP);

    left.state = false;
    controller.onTick();                   // AVOIDING -> left open -> turn left
    EXPECT_EQ(motor.last(), Direction::LEFT);
}

// [추가] Front interrupts are accepted only while cruising. During the avoidance
// sequence, the front sensor is reused for Right Scan and false rising edges must
// not reset the state machine.
TEST_F(RvcControllerTest, FrontInterruptAcceptedWhileCruising) {
    controller.start();
    EXPECT_TRUE(controller.onFrontObstacleDetected());
    EXPECT_EQ(motor.last(), Direction::STOP);
}

TEST_F(RvcControllerTest, FrontInterruptIgnoredDuringAvoidance) {
    controller.start();
    ASSERT_TRUE(controller.onFrontObstacleDetected());
    const auto cmds_before = motor.log.size();

    EXPECT_FALSE(controller.onFrontObstacleDetected());
    EXPECT_EQ(motor.log.size(), cmds_before);
}

TEST_F(RvcControllerTest, FrontInterruptIgnoredWhileEscaping) {
    controller.start();
    left.state = true;
    ASSERT_TRUE(controller.onFrontObstacleDetected());  // STOP, AVOIDING
    controller.onTick();                               // RIGHT, CHECKING_RIGHT
    front.state = true;
    controller.onTick();                               // LEFT, ESCAPING
    const auto cmds_before = motor.log.size();

    EXPECT_FALSE(controller.onFrontObstacleDetected());
    EXPECT_EQ(motor.log.size(), cmds_before);
}
