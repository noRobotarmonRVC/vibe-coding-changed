#pragma once

struct SensorData {
    bool is_front_blocked = false;
    bool is_left_blocked  = false;
    // [변경] This value now comes from FrontSensor Right Scan, not a dedicated RightSensor.
    bool is_right_blocked = false;
    bool has_dust         = false;
};
