#pragma once
#include "interfaces/ISensor.hpp"
#include "interfaces/IMotorController.hpp"
#include "interfaces/ICleanerController.hpp"
#include "interfaces/INavigationStrategy.hpp"
#include "domain/RvcState.hpp"

class RvcController {
public:
    static constexpr int INTENSIFY_DURATION = 5;  // ticks

    RvcController(ISensor* front_sensor, ISensor* left_sensor,
                  ISensor* dust_sensor,
                  IMotorController* motor, ICleanerController* cleaner,
                  INavigationStrategy* nav_strategy);

    void start();
    void stop();
    void onTick();
    void onFrontObstacleDetected();

private:
    ISensor*             _front_sensor;
    ISensor*             _left_sensor;
    ISensor*             _dust_sensor;
    IMotorController*    _motor;
    ICleanerController*  _cleaner;
    INavigationStrategy* _nav_strategy;
    RvcState             _state           = RvcState::IDLE;
    int                  _intensify_ticks = 0;
};
