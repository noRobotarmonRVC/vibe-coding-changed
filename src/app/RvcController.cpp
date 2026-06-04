#include "app/RvcController.hpp"
#include "domain/Direction.hpp"
#include "domain/CleanPower.hpp"
#include "domain/SensorData.hpp"

RvcController::RvcController(ISensor* front_sensor, ISensor* left_sensor,
                              ISensor* dust_sensor,
                              IMotorController* motor, ICleanerController* cleaner,
                              INavigationStrategy* nav_strategy)
    : _front_sensor(front_sensor)
    , _left_sensor(left_sensor)
    , _dust_sensor(dust_sensor)
    , _motor(motor)
    , _cleaner(cleaner)
    , _nav_strategy(nav_strategy) {}

void RvcController::start() {
    _state = RvcState::CLEANING;
    _cleaner->setPower(CleanPower::ON);
    _motor->move(Direction::FORWARD);
}

void RvcController::stop() {
    _state = RvcState::IDLE;
    _motor->move(Direction::STOP);
    _cleaner->setPower(CleanPower::OFF);
}

void RvcController::onTick() {
    switch (_state) {
        case RvcState::IDLE:
            return;

        case RvcState::INTENSIFYING:
            if (--_intensify_ticks <= 0) {
                _cleaner->setPower(CleanPower::ON);
                _state = RvcState::CLEANING;
            }
            return;

        case RvcState::AVOIDING_OBSTACLE: {
            // [변경] Side evaluation is advanced by tick after the front interrupt stops.
            SensorData data;
            data.is_front_blocked = true;
            data.is_left_blocked  = _left_sensor->detect();
            if (_nav_strategy->navigate(data) == Direction::LEFT) {
                _motor->move(Direction::LEFT);   // left open: turn and resume
                _state = RvcState::CLEANING;
            } else {
                _motor->move(Direction::RIGHT);  // left blocked: probe right side
                _state = RvcState::CHECKING_RIGHT;
            }
            return;
        }

        case RvcState::CHECKING_RIGHT:
            // [추가] Front sensor is reused after the robot rotates right.
            // Rotated right last tick, so the front sensor now faces the old right.
            if (_front_sensor->detect()) {
                _motor->move(Direction::LEFT);   // right blocked too: face back, escape
                _state = RvcState::ESCAPING;
            } else {
                _state = RvcState::CLEANING;      // right open: already facing it, resume
            }
            return;

        case RvcState::ESCAPING:
            // [변경] Escape backs up one cell, then re-evaluates sides on later ticks.
            _motor->move(Direction::BACKWARD);    // one cell back per tick
            _state = RvcState::AVOIDING_OBSTACLE; // re-evaluate sides after backing up
            return;

        case RvcState::CLEANING:
            if (_dust_sensor->detect()) {
                _intensify_ticks = INTENSIFY_DURATION;
                _state = RvcState::INTENSIFYING;
                _cleaner->setPower(CleanPower::POWER_UP);
            }
            _motor->move(Direction::FORWARD);
            return;
    }
}

bool RvcController::onFrontObstacleDetected() {
    // [변경] Accept front interrupts only while cruising. During avoidance,
    // FrontSensor is reused for Right Scan, so a rotation can create a false
    // rising edge that must be evaluated by onTick() instead.
    if (_state != RvcState::CLEANING && _state != RvcState::INTENSIFYING) {
        return false;
    }
    _motor->move(Direction::STOP);
    // [변경] Interrupt stops immediately; later ticks perform left/right evaluation.
    _state = RvcState::AVOIDING_OBSTACLE;
    return true;
}
