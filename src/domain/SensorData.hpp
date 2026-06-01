#pragma once

struct SensorData {
    bool is_front_blocked = false;
    bool is_left_blocked  = false;
    // [삭제] No right-side field; right probing is represented by CHECKING_RIGHT.
    bool has_dust         = false;
};
