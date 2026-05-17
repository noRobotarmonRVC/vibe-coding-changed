#pragma once
#include "domain/CleanPower.hpp"

class ICleanerController {
public:
    ICleanerController() = default;
    virtual ~ICleanerController() = default;
    ICleanerController(const ICleanerController&) = delete;
    ICleanerController& operator=(const ICleanerController&) = delete;
    ICleanerController(ICleanerController&&) = delete;
    ICleanerController& operator=(ICleanerController&&) = delete;

    virtual void setPower(CleanPower power) = 0;
};
