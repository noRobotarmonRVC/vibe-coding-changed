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
    void injectDust(bool reading);

    // Environment setup
    void placeObstacle(int x, int y);
    void placeDust(int x, int y);

    // Observation
    [[nodiscard]] Direction  lastDirection() const;
    [[nodiscard]] CleanPower lastPower()     const;
    [[nodiscard]] Position   pos()           const;
    [[nodiscard]] Heading    heading()       const;
    [[nodiscard]] int        gridWidth()     const;
    [[nodiscard]] int        gridHeight()    const;

    [[nodiscard]] const std::set<std::pair<int,int>>& obstacles()  const;
    [[nodiscard]] const std::set<std::pair<int,int>>& dustCells() const;
    [[nodiscard]] const std::vector<Direction>&       motorLog()    const;
    [[nodiscard]] const std::vector<CleanPower>&      cleanerLog()  const;

private:
    [[nodiscard]] bool     isBlocked(Position p)               const;
    [[nodiscard]] static Position adjacentCell(Position p, Heading h);
    [[nodiscard]] static Heading  turnLeft(Heading h);
    [[nodiscard]] static Heading  turnRight(Heading h);
    void     applyPendingMotorCommands();

    int     _grid_width;
    int     _grid_height;
    Position _pos;
    Heading  _heading;
    size_t   _motor_log_applied = 0;
    // [추가] Tracks front obstacle rising edge for interrupt semantics.
    bool     _prev_front_blocked = false;
    std::set<std::pair<int,int>> _obstacles;
    std::set<std::pair<int,int>> _dust_cells;

    SimulatedSensor           _front;
    SimulatedSensor           _left;
    SimulatedSensor           _dust;
    SimulatedMotor            _motor;
    SimulatedCleaner          _cleaner;
    DefaultNavigationStrategy _nav;
    RvcController             _controller;
};
