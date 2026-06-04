#include "simulator/Simulator.hpp"

Simulator::Simulator(int grid_width, int grid_height,
                     Position start, Heading start_heading)
    : _grid_width(grid_width)
    , _grid_height(grid_height)
    , _pos(start)
    , _heading(start_heading)
    , _controller(&_front, &_left, &_dust, &_motor, &_cleaner, &_nav) {}

void Simulator::start() {
    _controller.start();
    applyPendingMotorCommands();
}

void Simulator::stop() {
    _controller.stop();
    applyPendingMotorCommands();
}

void Simulator::tick() {
    // [삭제] No right sensor injection; heading-aware front readings drive right probing.
    // Auto-inject left sensor from the grid (right sensor no longer exists)
    _left.inject(isBlocked(adjacentCell(_pos, turnLeft(_heading))));

    // Front sensor is also polled: the controller reuses it to probe the right
    // side (after rotating right) since there is no dedicated right sensor.
    bool front_blocked = isBlocked(adjacentCell(_pos, _heading));
    _front.inject(front_blocked);

    // Auto-inject dust sensor if robot is on a dust cell, then consume it
    auto dust_key = std::make_pair(_pos.x, _pos.y);
    if (_dust_cells.count(dust_key) != 0U) {
        _dust.inject(true);
        _dust_cells.erase(dust_key);
    } else {
        _dust.inject(false);
    }

    // Front obstacle is an interrupt: fire only on the rising edge (clear ->
    // blocked). Multi-tick avoidance/escape then advances via onTick().
    // [추가] Edge-triggering prevents repeated STOP while the robot remains blocked.
    // [수정] interrupt 수용 정책은 controller가 소유한다(onFrontObstacleDetected가
    // 정상 주행 중일 때만 처리하고 true를 반환). controller가 처리하지 않으면(회피
    // 시퀀스 중 — Right Scan 회전이 만든 거짓 edge) onTick()으로 폴백해 평가를
    // 진행시킨다. 그래야 회전이 만든 rising edge가 CHECKING_RIGHT를 가로채지 못한다(F-10).
    bool handled = false;
    if (front_blocked && !_prev_front_blocked) {
        handled = _controller.onFrontObstacleDetected();
    }
    if (!handled) {
        _controller.onTick();
    }
    _prev_front_blocked = front_blocked;
    applyPendingMotorCommands();
}

void Simulator::triggerFrontObstacle() {
    _controller.onFrontObstacleDetected();
    applyPendingMotorCommands();
}

void Simulator::injectFront(bool reading) { _front.inject(reading); }
void Simulator::injectLeft(bool reading)  { _left.inject(reading); }
void Simulator::injectDust(bool reading)  { _dust.inject(reading); }

void Simulator::placeObstacle(int x, int y) { _obstacles.insert({x, y}); }
void Simulator::placeDust(int x, int y)     { _dust_cells.insert({x, y}); }

Direction  Simulator::lastDirection() const { return _motor.last(); }
CleanPower Simulator::lastPower()     const { return _cleaner.last(); }
Position   Simulator::pos()           const { return _pos; }
Heading    Simulator::heading()       const { return _heading; }
int        Simulator::gridWidth()     const { return _grid_width; }
int        Simulator::gridHeight()    const { return _grid_height; }

const std::set<std::pair<int,int>>& Simulator::obstacles()  const { return _obstacles; }
const std::set<std::pair<int,int>>& Simulator::dustCells() const { return _dust_cells; }
const std::vector<Direction>&       Simulator::motorLog()   const { return _motor.log(); }
const std::vector<CleanPower>&      Simulator::cleanerLog() const { return _cleaner.log(); }

// ── private helpers ────────────────────────────────────────────────────────────

bool Simulator::isBlocked(Position p) const {
    if (p.x < 0 || p.x >= _grid_width || p.y < 0 || p.y >= _grid_height) {
        return true;
    }
    return _obstacles.count({p.x, p.y}) > 0;
}

Position Simulator::adjacentCell(Position p, Heading h) {
    switch (h) {
        case Heading::NORTH: return {p.x,     p.y - 1};
        case Heading::EAST:  return {p.x + 1, p.y    };
        case Heading::SOUTH: return {p.x,     p.y + 1};
        case Heading::WEST:  return {p.x - 1, p.y    };
    }
    return p;
}

Heading Simulator::turnLeft(Heading h) {
    switch (h) {
        case Heading::NORTH: return Heading::WEST;
        case Heading::WEST:  return Heading::SOUTH;
        case Heading::SOUTH: return Heading::EAST;
        case Heading::EAST:  return Heading::NORTH;
    }
    return h;
}

Heading Simulator::turnRight(Heading h) {
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
                if (!isBlocked(next)) { _pos = next; }
                break;
            }
            case Direction::BACKWARD: {
                // opposite of current heading = two left-turns
                Heading opp  = turnLeft(turnLeft(_heading));
                Position next = adjacentCell(_pos, opp);
                if (!isBlocked(next)) { _pos = next; }
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
