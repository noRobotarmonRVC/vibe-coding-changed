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
    // [수정] interrupt 수용 정책을 controller가 소유한다. 정상 주행 중이면 처리하고
    // true, 회피 시퀀스 중이면 무시하고 false를 반환한다(F-10).
    bool onFrontObstacleDetected();

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
