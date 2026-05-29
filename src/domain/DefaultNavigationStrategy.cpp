#include "domain/DefaultNavigationStrategy.hpp"

Direction DefaultNavigationStrategy::navigate(const SensorData& data) {
    // No right sensor: left open -> turn left; left blocked -> BACKWARD signals
    // the controller to probe right (rotate + front sensor) before escaping.
    if (data.is_front_blocked && data.is_left_blocked) {
        return Direction::BACKWARD;
    }
    if (data.is_front_blocked) {
        return Direction::LEFT;
    }
    return Direction::FORWARD;
}
