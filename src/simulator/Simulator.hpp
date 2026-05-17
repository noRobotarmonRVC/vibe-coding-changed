#pragma once
#include <set>
#include <utility>
#include "domain/Position.hpp"
#include "domain/Heading.hpp"
#include "simulator/SimulatedSensor.hpp"
#include "simulator/SimulatedMotor.hpp"
#include "simulator/SimulatedCleaner.hpp"
#include "domain/DefaultNavigationStrategy.hpp"
#include "app/RvcController.hpp"

class Simulator {
public:
    explicit Simulator(int grid_width  = 20,
                       int grid_height = 12,
                       Position start         = {1, 5},
                       Heading  start_heading = Heading::EAST);

    // Lifecycle
    void start();
    void stop();
    void tick();                  // auto-detects sensors from grid each cycle
    void triggerFrontObstacle();  // manual interrupt override

    // Manual sensor injection (overrides grid auto-detection for that tick)
    void injectFront(bool reading);
    void injectLeft(bool reading);
    void injectRight(bool reading);
    void injectDust(bool reading);

    // Environment setup
    void placeObstacle(int x, int y);

    // Observation
    Direction  lastDirection() const;
    CleanPower lastPower()     const;
    Position   pos()           const;
    Heading    heading()       const;
    int        gridWidth()     const;
    int        gridHeight()    const;

    const std::set<std::pair<int,int>>& obstacles()   const;
    const std::vector<Direction>&       motorLog()    const;
    const std::vector<CleanPower>&      cleanerLog()  const;

private:
    bool     isBlocked(Position p)             const;
    Position adjacentCell(Position p, Heading h) const;
    Heading  turnLeft(Heading h)               const;
    Heading  turnRight(Heading h)              const;
    void     applyPendingMotorCommands();

    int     _grid_width;
    int     _grid_height;
    Position _pos;
    Heading  _heading;
    size_t   _motor_log_applied = 0;
    std::set<std::pair<int,int>> _obstacles;

    SimulatedSensor           _front;
    SimulatedSensor           _left;
    SimulatedSensor           _right;
    SimulatedSensor           _dust;
    SimulatedMotor            _motor;
    SimulatedCleaner          _cleaner;
    DefaultNavigationStrategy _nav;
    RvcController             _controller;
};
