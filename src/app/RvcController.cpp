#include "app/RvcController.hpp"
#include "domain/Direction.hpp"
#include "domain/CleanPower.hpp"

RvcController::RvcController(ISensor* front_sensor, ISensor* left_sensor,
                              ISensor* right_sensor, ISensor* dust_sensor,
                              IMotorController* motor, ICleanerController* cleaner,
                              INavigationStrategy* nav_strategy)
    : _front_sensor(front_sensor)
    , _left_sensor(left_sensor)
    , _right_sensor(right_sensor)
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
    if (_state == RvcState::IDLE) {
        return;
    }

    if (_state == RvcState::INTENSIFYING) {
        if (--_intensify_ticks <= 0) {
            _cleaner->setPower(CleanPower::ON);
            _state = RvcState::CLEANING;
        }
        return;
    }

    if (_state == RvcState::CLEANING && _dust_sensor->detect()) {
        _intensify_ticks = INTENSIFY_DURATION;
        _state = RvcState::INTENSIFYING;
        _cleaner->setPower(CleanPower::POWER_UP);
    }

    if (_state == RvcState::CLEANING || _state == RvcState::INTENSIFYING) {
        _motor->move(Direction::FORWARD);
    }
}

void RvcController::onFrontObstacleDetected() {
    if (_state == RvcState::IDLE) {
        return;
    }

    _motor->move(Direction::STOP);

    SensorData data;
    data.is_front_blocked = true;
    data.is_left_blocked  = _left_sensor->detect();
    data.is_right_blocked = _right_sensor->detect();

    Direction nav = _nav_strategy->navigate(data);

    if (nav == Direction::BACKWARD) {
        _state = RvcState::ESCAPING;
        _motor->move(Direction::BACKWARD);
        _motor->move(Direction::LEFT);  // default escape turn
    } else {
        _state = RvcState::AVOIDING_OBSTACLE;
        _motor->move(nav);
    }

    _motor->move(Direction::FORWARD);
    _state = RvcState::CLEANING;
}
