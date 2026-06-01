#pragma once
#include <cstdint>

enum class RvcState : std::uint8_t {
    IDLE,
    CLEANING,
    AVOIDING_OBSTACLE,
    // [추가] Explicit state for FrontSensor-based right-side probing.
    CHECKING_RIGHT,
    ESCAPING,
    INTENSIFYING
};

inline const char* toString(RvcState s) {
    switch (s) {
        case RvcState::IDLE:              return "IDLE";
        case RvcState::CLEANING:          return "CLEANING";
        case RvcState::AVOIDING_OBSTACLE: return "AVOIDING_OBSTACLE";
        case RvcState::CHECKING_RIGHT:    return "CHECKING_RIGHT";
        case RvcState::ESCAPING:          return "ESCAPING";
        case RvcState::INTENSIFYING:      return "INTENSIFYING";
    }
    return "UNKNOWN";
}
