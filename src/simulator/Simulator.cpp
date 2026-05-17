#include "simulator/Simulator.hpp"

Simulator::Simulator(int grid_width, int grid_height,
                     Position start, Heading start_heading)
    : _grid_width(grid_width)
    , _grid_height(grid_height)
    , _pos(start)
    , _heading(start_heading)
    , _controller(&_front, &_left, &_right, &_dust, &_motor, &_cleaner, &_nav) {}

void Simulator::start() {
    _controller.start();
    applyPendingMotorCommands();
}

void Simulator::stop() {
    _controller.stop();
    applyPendingMotorCommands();
}

void Simulator::tick() {
    // Auto-inject left/right sensors from the grid before the control cycle
    _left.inject(isBlocked(adjacentCell(_pos, turnLeft(_heading))));
    _right.inject(isBlocked(adjacentCell(_pos, turnRight(_heading))));

    // Front sensor is interrupt-driven: trigger handler if blocked, else run tick
    if (isBlocked(adjacentCell(_pos, _heading))) {
        _controller.onFrontObstacleDetected();
    } else {
        _controller.onTick();
    }
    applyPendingMotorCommands();
}

void Simulator::triggerFrontObstacle() {
    _controller.onFrontObstacleDetected();
    applyPendingMotorCommands();
}

void Simulator::injectFront(bool reading) { _front.inject(reading); }
void Simulator::injectLeft(bool reading)  { _left.inject(reading); }
void Simulator::injectRight(bool reading) { _right.inject(reading); }
void Simulator::injectDust(bool reading)  { _dust.inject(reading); }

void Simulator::placeObstacle(int x, int y) { _obstacles.insert({x, y}); }

Direction  Simulator::lastDirection() const { return _motor.last(); }
CleanPower Simulator::lastPower()     const { return _cleaner.last(); }
Position   Simulator::pos()           const { return _pos; }
Heading    Simulator::heading()       const { return _heading; }
int        Simulator::gridWidth()     const { return _grid_width; }
int        Simulator::gridHeight()    const { return _grid_height; }

const std::set<std::pair<int,int>>& Simulator::obstacles()  const { return _obstacles; }
const std::vector<Direction>&       Simulator::motorLog()   const { return _motor.log(); }
const std::vector<CleanPower>&      Simulator::cleanerLog() const { return _cleaner.log(); }

// ── private helpers ────────────────────────────────────────────────────────────

bool Simulator::isBlocked(Position p) const {
    if (p.x < 0 || p.x >= _grid_width || p.y < 0 || p.y >= _grid_height)
        return true;
    return _obstacles.count({p.x, p.y}) > 0;
}

Position Simulator::adjacentCell(Position p, Heading h) const {
    switch (h) {
        case Heading::NORTH: return {p.x,     p.y - 1};
        case Heading::EAST:  return {p.x + 1, p.y    };
        case Heading::SOUTH: return {p.x,     p.y + 1};
        case Heading::WEST:  return {p.x - 1, p.y    };
    }
    return p;
}

Heading Simulator::turnLeft(Heading h) const {
    switch (h) {
        case Heading::NORTH: return Heading::WEST;
        case Heading::WEST:  return Heading::SOUTH;
        case Heading::SOUTH: return Heading::EAST;
        case Heading::EAST:  return Heading::NORTH;
    }
    return h;
}

Heading Simulator::turnRight(Heading h) const {
    switch (h) {
        case Heading::NORTH: return Heading::EAST;
        case Heading::EAST:  return Heading::SOUTH;
        case Heading::SOUTH: return Heading::WEST;
        case Heading::WEST:  return Heading::NORTH;
    }
    return h;
}

void Simulator::applyPendingMotorCommands() {
    const auto& log = _motor.log();
    for (size_t i = _motor_log_applied; i < log.size(); ++i) {
        switch (log[i]) {
            case Direction::FORWARD: {
                Position next = adjacentCell(_pos, _heading);
                if (!isBlocked(next)) _pos = next;
                break;
            }
            case Direction::BACKWARD: {
                // opposite of current heading = two left-turns
                Heading opp  = turnLeft(turnLeft(_heading));
                Position next = adjacentCell(_pos, opp);
                if (!isBlocked(next)) _pos = next;
                break;
            }
            case Direction::LEFT:
                _heading = turnLeft(_heading);
                break;
            case Direction::RIGHT:
                _heading = turnRight(_heading);
                break;
            case Direction::STOP:
                break;
        }
    }
    _motor_log_applied = log.size();
}
