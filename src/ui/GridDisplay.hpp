#pragma once
#include <set>
#include <utility>
#include "domain/Position.hpp"
#include "domain/Heading.hpp"
#include "domain/Direction.hpp"
#include "domain/CleanPower.hpp"

class GridDisplay {
public:
    GridDisplay(int width, int height);

    void render(Position rvc, Heading heading,
                const std::set<std::pair<int,int>>& obstacles,
                const std::set<std::pair<int,int>>& dust_cells,
                Direction dir, CleanPower power, int tick) const;

private:
    int _width;
    int _height;
};
