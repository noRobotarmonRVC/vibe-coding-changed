#pragma once
#include "domain/Direction.hpp"

class IMotorController {
public:
    IMotorController() = default;
    virtual ~IMotorController() = default;
    IMotorController(const IMotorController&) = delete;
    IMotorController& operator=(const IMotorController&) = delete;
    IMotorController(IMotorController&&) = delete;
    IMotorController& operator=(IMotorController&&) = delete;

    virtual void move(Direction direction) = 0;
};
