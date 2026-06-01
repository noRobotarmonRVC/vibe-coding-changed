#pragma once
#include "interfaces/ISensor.hpp"

// [삭제] Inactive legacy type. Right-side detection uses FrontSensor Right Scan.
class RightSensor : public ISensor {
public:
    bool detect() override;
};
